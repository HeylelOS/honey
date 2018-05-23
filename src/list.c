/*
	list.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <stdlib.h>
#include <string.h>

enum hny_error
hny_list(hny_t hny,
	enum hny_listing listing,
	struct hny_geist **list,
	size_t *len) {
	enum hny_error error = HnyErrorNone;

	if(hny_lock(hny) == HnyErrorNone) {
		struct dirent *entry;
		size_t alloced = 0;

		*list = NULL;
		*len = 0;

		rewinddir(hny->dirp);
		while((entry = readdir(hny->dirp)) != NULL) {

			if(hny_check_package(entry->d_name) == HnyErrorNone) {
				char *stringp;

				if(*len == alloced) {
					alloced += 16;
					*list = realloc(*list, sizeof(**list) * alloced);
				}

				if(listing == HnyListPackages
					&& entry->d_type == DT_DIR) {

					stringp = strdup(entry->d_name);

					(*list)[*len].name = strsep(&stringp, "-");

					if(stringp != NULL) {
						(*list)[*len].version = stringp;

						(*len)++;
					} else {
						free(stringp);
					}
				} else if(listing == HnyListActive
					&& entry->d_type == DT_LNK
					&& strchr(entry->d_name, '-') == NULL) {

					(*list)[*len].name = strdup(entry->d_name);
					(*list)[*len].version = NULL;

					(*len)++;
				}
			}
		}

		*list = realloc(*list, sizeof(**list) * (*len));

		hny_unlock(hny);
	} else {
		error = HnyErrorUnavailable;
	}

	return error;
}

