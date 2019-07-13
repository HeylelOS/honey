/*
	list.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE
*/
#include "internal.h"

#include <stdlib.h>
#include <string.h>

enum hny_error
hny_list(struct hny *hny,
	enum hny_listing listing,
	struct hny_geist **list,
	hny_size_t *len) {
	enum hny_error retval = HNY_ERROR_NONE;

	if(hny_lock(hny) == HNY_ERROR_NONE) {
		struct dirent *entry;
		size_t capacity = 0;

		*list = NULL;
		*len = 0;

		rewinddir(hny->dirp);
		while((entry = readdir(hny->dirp)) != NULL) {

			if(hny_check_package(entry->d_name) == HNY_ERROR_NONE) {
				char *stringp;

				if(*len == capacity) {
					capacity += 16;
					*list = realloc(*list, sizeof(**list) * capacity);
				}

				if(listing == HNY_LIST_PACKAGES
					&& entry->d_type == DT_DIR) {

					stringp = strdup(entry->d_name);

					(*list)[*len].name = strsep(&stringp, "-");

					if(stringp != NULL) {
						(*list)[*len].version = stringp;

						++(*len);
					} else {
						free(stringp);
					}
				} else if(listing == HNY_LIST_ACTIVE
					&& entry->d_type == DT_LNK
					&& strchr(entry->d_name, '-') == NULL) {

					(*list)[*len].name = strdup(entry->d_name);
					(*list)[*len].version = NULL;

					++(*len);
				}
			}
		}

		*list = realloc(*list, sizeof(**list) * (*len));

		hny_unlock(hny);
	} else {
		retval = HNY_ERROR_UNAVAILABLE;
	}

	return retval;
}

