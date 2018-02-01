/*
	archive.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <sys/param.h> /* MAXPATHLEN */
#include <sys/dir.h>
#include <string.h>
#include <stdlib.h>

#include <archive.h>
#include <archive_entry.h>

static enum hny_error hny_archive_install(const char *file, const char *name) {
	enum hny_error error = HnyErrorNone;
	struct archive *a;
	int aerr;

	a = archive_read_new();
	archive_read_support_filter_xz(a);
	archive_read_support_format_tar(a);

	aerr = archive_read_open_filename(a, file, 4096);
	if(aerr == ARCHIVE_OK) {
		struct archive *aw;
		struct archive_entry *entry;
		int flags = ARCHIVE_EXTRACT_TIME
			| ARCHIVE_EXTRACT_PERM
			| ARCHIVE_EXTRACT_ACL
			| ARCHIVE_EXTRACT_FFLAGS
			| ARCHIVE_EXTRACT_NO_OVERWRITE;
		char path[MAXPATHLEN];
		int pathlength;

		aw = archive_write_disk_new();
		archive_write_disk_set_options(aw, flags);

		pathlength = snprintf(path, sizeof(path),
			"%s/%s/", hive->installdir, name);

		while((aerr = archive_read_next_header(a, &entry)) == ARCHIVE_OK
			&& error == HnyErrorNone) {
			path[pathlength] = '\0';
			strncat(path, archive_entry_pathname(entry), sizeof(path) - pathlength - 1);

			archive_entry_set_pathname(entry, path);

			aerr = archive_write_header(aw, entry);
			if(aerr == ARCHIVE_OK) {
				const void *buff;
				size_t size;
				int64_t offset;

				while(aerr == ARCHIVE_OK) {
					aerr = archive_read_data_block(a, &buff, &size, &offset);
					if(aerr == ARCHIVE_OK) {
						aerr = archive_write_data_block(aw, buff, size, offset);
					}
				}

				if(aerr != ARCHIVE_EOF) {
					error = HnyErrorNonExistant;
				}

				aerr = archive_write_finish_entry(aw);
				if(error == HnyErrorNone
					&& aerr != ARCHIVE_OK) {
					error = HnyErrorNonExistant;
				}
			} else {
				error = HnyErrorNonExistant;
			}
		}

		if(error != HnyErrorNone) {
			path[pathlength] = '\0';
			hny_rm_recur(path);
		}

		archive_write_close(aw);
		archive_write_free(aw);

		archive_read_close(a);
	} else {
		/* printf("libarchive error: %s\n", archive_error_string(a)); */
		error = HnyErrorInvalidArgs;
	}

	archive_read_free(a);

	return error;
}

#define HNY_ARCHIVE_HAS_HNY	(1 << 0)
#define HNY_ARCHIVE_HAS_SETUP	(1 << 1)
#define HNY_ARCHIVE_HAS_DRAIN	(1 << 2)
#define HNY_ARCHIVE_HAS_EULA	(1 << 3)

#define HNY_ARCHIVE_HAS_ALL	(HNY_ARCHIVE_HAS_HNY\
				| HNY_ARCHIVE_HAS_SETUP\
				| HNY_ARCHIVE_HAS_DRAIN\
				| HNY_ARCHIVE_HAS_EULA)

static enum hny_error hny_archive_verify(const char *file, _Bool (*eula)(const char *, size_t)) {
	enum hny_error error = HnyErrorNone;
	struct archive *a;
	int aerr;

	a = archive_read_new();
	archive_read_support_filter_xz(a);
	archive_read_support_format_tar(a);

	aerr = archive_read_open_filename(a, file, 4096);

	if(aerr == ARCHIVE_OK) {
		struct archive_entry *entry;
		int archive_has = 0;

		while((aerr = archive_read_next_header(a, &entry)) == ARCHIVE_OK
			&& archive_has != HNY_ARCHIVE_HAS_EULA
			&& error == HnyErrorNone) {
			const char *entry_name = archive_entry_pathname(entry);

			if(strcmp("hny/eula", entry_name) == 0) {
				const void *buff;
				size_t size;
				int64_t offset;

				if(archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK) {
					if(eula((const char *) buff, size)) {
						archive_has |= HNY_ARCHIVE_HAS_EULA;
					} else {
						error = HnyErrorUnauthorized;
					}
				} else {
					error = HnyErrorNonExistant;
				}
			} else {
				if(strcmp("hny/", entry_name) == 0) {
					archive_has |= HNY_ARCHIVE_HAS_HNY;
				} else if(strcmp("hny/setup", entry_name) == 0) {
					archive_has |= HNY_ARCHIVE_HAS_SETUP;
				} else if(strcmp("hny/drain", entry_name) == 0) {
					archive_has |= HNY_ARCHIVE_HAS_DRAIN;
				}

				archive_read_data_skip(a);
			}
		}

		if(error == HnyErrorNone
			&& (archive_has != HNY_ARCHIVE_HAS_ALL
				|| aerr != ARCHIVE_EOF)) {
			error = HnyErrorNonExistant;
		}

		archive_read_close(a);
	} else {
		error = HnyErrorInvalidArgs;
	}

	archive_read_free(a);

	return error;
}

enum hny_error hny_install(const char *file, _Bool (*eula)(const char *, size_t)) {
	enum hny_error error = HnyErrorNone;
	const char *fileext;

	pthread_mutex_lock(&hive->mutex);

	fileext = strrchr(file, '.');
	if(strncmp(fileext, ".hny", 5) == 0) {
		const char *filename = strrchr(file, '/');
		char *packagename;

		if(filename == NULL) {
			packagename = strdup(file);
		} else {
			packagename = strdup(++filename);
		}

		*(strrchr(packagename, '.')) = '\0';

		if(strchr(packagename, '-') != NULL
			&& hny_check_package(packagename) == HnyErrorNone) {
			DIR *dirp;

			if((dirp = opendir(hive->installdir)) != NULL) {
				error = hny_archive_verify(file, eula);

				if(error == HnyErrorNone) {
					error = hny_archive_install(file, packagename);
				}

				closedir(dirp);
			} else {
				error = HnyErrorUnauthorized;
			}
		} else {
			error = HnyErrorInvalidArgs;
		}

		free(packagename);
	} else {
		error = HnyErrorInvalidArgs;
	}

	pthread_mutex_unlock(&hive->mutex);

	return error;
}

