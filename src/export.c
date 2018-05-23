/*
	export.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <sys/param.h> /* MAXPATHLEN */
#include <string.h>

#include <archive.h>
#include <archive_entry.h>

enum hny_error
hny_export(hny_t hny,
	const char *file,
	const struct hny_geist *package) {
	enum hny_error error = HnyErrorNone;
	struct archive *a;
	int aerr;

	a = archive_read_new();
	archive_read_support_filter_xz(a);
	archive_read_support_format_tar(a);

	aerr = archive_read_open_filename(a, file, 4096);
	if(aerr == ARCHIVE_OK
		&& package->version != NULL
		&& hny_check_geister(package, 1) == HnyErrorNone) {

		if(hny_lock(hny) == HnyErrorNone) {
			char path[MAXPATHLEN];
			ssize_t length;
			char *end = stpncpy(path, hny->path, MAXPATHLEN);

			length = hny_fill_packagename(&end[1],
				&path[MAXPATHLEN] - end - 1, package);
			if(length > 0) {
				struct archive *aw;
				struct archive_entry *entry;
				int flags = ARCHIVE_EXTRACT_TIME
					| ARCHIVE_EXTRACT_PERM
					| ARCHIVE_EXTRACT_ACL
					| ARCHIVE_EXTRACT_FFLAGS
					| ARCHIVE_EXTRACT_NO_OVERWRITE;

				if(geteuid() == 0) {
					flags |= ARCHIVE_EXTRACT_OWNER;
				}

				*end = '/';
				length += end - path + 2;
				path[length - 1] = '/';

				aw = archive_write_disk_new();
				archive_write_disk_set_options(aw, flags);

				while((aerr = archive_read_next_header(a, &entry)) == ARCHIVE_OK
					&& error == HnyErrorNone) {
					path[length] = '\0';
					strncat(path, archive_entry_pathname(entry),
						sizeof(path) - length + 1);

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

				archive_write_close(aw);
				archive_write_free(aw);
			} else {
				error = HnyErrorInvalidArgs;
			}

			hny_unlock(hny);
		} else {
			error = HnyErrorUnavailable;
		}

		archive_read_close(a);
	} else {
		/* fprintf(stderr, "libarchive error: %s\n", archive_error_string(a)); */
		error = HnyErrorInvalidArgs;
	}

	archive_read_free(a);

	return error;
}

