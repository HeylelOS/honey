/*
	erase.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <sys/param.h> /* MAXPATHLEN, NAME_MAX */
#include <unistd.h> /* unlinkat() */
#include <stdio.h> /* remove() */
#include <stdlib.h>
#include <string.h>
#include <errno.h>

enum hny_error
hny_erase(hny_t hny,
	const struct hny_geist *geist) {
	enum hny_error error = HnyErrorNone;

	if(hny_check_geister(geist, 1) == HnyErrorNone
		&& geist->version != NULL) {

		if(hny_lock(hny)) {
			struct dirent *entry;
			char *name1 = malloc(MAXPATHLEN);
			char *name2 = malloc(NAME_MAX);

			/* First, delete all geister linked after me */
			rewinddir(hny->dirp);
			while((entry = readdir(hny->dirp)) != NULL) {
				if(entry->d_type == DT_LNK
					&& hny_check_name(entry->d_name) == HnyErrorNone) {
					char *targetname;

					strncpy(name2, entry->d_name, NAME_MAX);

					targetname = hny_target(dirfd(hny->dirp),
						name2, name1, NAME_MAX);
					if(targetname != NULL) {
						struct hny_geist target;

						target.name = strsep(&targetname, "-");
						target.version = targetname;

						if(hny_equals_geister(&target, geist) == 0
							&& unlinkat(dirfd(hny->dirp),
								entry->d_name, 0) == -1) {
							/* perror("hny remove link"); */
						}

						free(target.name);
					}
				}
			}
			free(name2);

			/* Then delete the geist */
			snprintf(name1, MAXPATHLEN,
				"%s/%s-%s", hny->path,
				geist->name, geist->version);
			error = hny_remove_recursive(name1);
			free(name1);

			hny_unlock(hny);
		} else {
			error = HnyErrorUnavailable;
		}
	} else {
		error = HnyErrorInvalidArgs;
	}

	return error;
}

