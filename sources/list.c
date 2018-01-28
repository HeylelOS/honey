/*
	list.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <sys/dir.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

struct hny_geist *hny_list(enum hny_listing listing, size_t *listed) {
	struct hny_geist *list = NULL;
	size_t alloced = 0;
	DIR *dirp;
	struct dirent *entry;
	*listed = 0;

	pthread_mutex_lock(&hive->mutex);

	if((dirp = opendir(hive->installdir)) != NULL) {
		while((entry = readdir(dirp)) != NULL) {
			if(hny_check_name(entry->d_name) == HnyErrorNone) {
				char *stringp;

				if(*listed == alloced) {
					alloced += 16;
					list = realloc(list, sizeof(*list) * alloced);
				}

				if(listing == HnyListPackages
					&& entry->d_type == DT_DIR) {
					stringp = strdup(entry->d_name);

					list[*listed].name = strsep(&stringp, "-");
					if(stringp != NULL) {
						list[*listed].version = stringp;

						(*listed)++;
					} else {
						free(stringp);
					}
				} else if(listing == HnyListActive
					&& entry->d_type == DT_LNK
					&& strchr(entry->d_name, '-') == NULL) {
					list[*listed].name = strdup(entry->d_name);
					list[*listed].version = NULL;

					(*listed)++;
				}
			}
		}

		list = realloc(list, sizeof(*list) * (*listed));

		closedir(dirp);
	}

	pthread_mutex_unlock(&hive->mutex);

	return list;
}

