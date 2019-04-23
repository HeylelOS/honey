/*
	verify.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE
*/
#include "internal.h"

#include <archive.h>
#include <archive_entry.h>

#include <stdlib.h>
#include <string.h>

#define HNY_ARCHIVE_HAS_HNY	(1 << 0)
#define HNY_ARCHIVE_HAS_SETUP	(1 << 1)
#define HNY_ARCHIVE_HAS_CLEAN	(1 << 2)
#define HNY_ARCHIVE_HAS_EULA	(1 << 3)

#define HNY_ARCHIVE_HAS_ALL	(HNY_ARCHIVE_HAS_HNY\
				| HNY_ARCHIVE_HAS_SETUP\
				| HNY_ARCHIVE_HAS_CLEAN\
				| HNY_ARCHIVE_HAS_EULA)

enum hny_error
hny_verify(const char *file,
	char **eula,
	hny_size_t *len) {
	enum hny_error retval = HNY_ERROR_NONE;
	struct archive *a;
	int aerr;
	char *alloc = NULL;
	size_t size;

	a = archive_read_new();
	archive_read_support_filter_xz(a);
	archive_read_support_format_cpio(a);

	aerr = archive_read_open_filename(a, file, 4096);

	if(aerr == ARCHIVE_OK) {
		struct archive_entry *entry;
		int archive_has = 0;

		while((aerr = archive_read_next_header(a, &entry)) == ARCHIVE_OK
			&& archive_has != HNY_ARCHIVE_HAS_EULA
			&& retval == HNY_ERROR_NONE) {
			const char *entry_name = archive_entry_pathname(entry);

			if(strncmp("hny", entry_name, 3) == 0) {
				if(strcmp("/eula", entry_name + 3) == 0) {
					const void *buff;
					int64_t offset;

					if(archive_read_data_block(a, &buff,
						&size, &offset) == ARCHIVE_OK
						&& size != 0) {

						alloc = malloc(size);
						if(alloc != NULL) {
							memcpy(alloc, buff, size);

							archive_has |= HNY_ARCHIVE_HAS_EULA;
						} else {
							retval = HNY_ERROR_UNAVAILABLE;
						}
					} else {
						retval = HNY_ERROR_MISSING;
					}
				} else {
					if(entry_name[3] == '\0') {
						archive_has |= HNY_ARCHIVE_HAS_HNY;
					} else if(strcmp("/setup", entry_name + 3) == 0) {
						archive_has |= HNY_ARCHIVE_HAS_SETUP;
					} else if(strcmp("/clean", entry_name + 3) == 0) {
						archive_has |= HNY_ARCHIVE_HAS_CLEAN;
					}

					archive_read_data_skip(a);
				}
			}
		}

		if(retval == HNY_ERROR_NONE
			&& (archive_has != HNY_ARCHIVE_HAS_ALL
				|| aerr != ARCHIVE_EOF)) {
			retval = HNY_ERROR_MISSING;
		}

		archive_read_close(a);
	} else {
		retval = HNY_ERROR_INVALID_ARGS;
	}

	archive_read_free(a);

	if(retval == HNY_ERROR_NONE) {
		*eula = alloc;
		*len = size;
	} else {
		free(alloc);
	}

	return retval;
}

