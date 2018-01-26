/*
	remove.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <sys/param.h> /* MAXPATHLEN */
#include <sys/dir.h>
#include <unistd.h>
#include <stdio.h> /* remove() */
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <errno.h>

static int hny_remove_fn(const char *path, const struct stat *st, int type, struct FTW* ftw) {
	if(remove(path) == -1) {
		perror("hny remove package");
	}

	return 0;
}

enum hny_error hny_remove(const struct hny_geist *geist) {
	enum hny_error error = HnyErrorNone;

	if(hny_check_geister(geist, 1) != HnyErrorNone) {
		return HnyErrorInvalidArgs;
	}

	pthread_mutex_lock(&hive->mutex);

	if(geist->version != NULL) {
		DIR *dirp = opendir(hive->installdir);

		if(dirp != NULL) {
			struct dirent *entry;
			char *name1 = malloc(NAME_MAX);
			char *name2 = malloc(NAME_MAX);

			while((entry = readdir(dirp)) != NULL) {
				if(entry->d_type == DT_LNK
					&& hny_check_name(entry->d_name) == HnyErrorNone) {
					char *targetname;
					strncpy(name2, entry->d_name, NAME_MAX);

					targetname = hny_target(dirfd(dirp), name2, name1, NAME_MAX);
					if(targetname != NULL) {
						struct hny_geist target;

						target.name = strsep(&targetname, "-");
						target.version = targetname;

						if(hny_equals_geister(&target, geist)
							&& unlinkat(dirfd(dirp), entry->d_name, 0) == -1) {
							perror("hny remove link");
						}

						free(target.name);
					}
				}
			}

			/* Different use variable, I know this is evil */
			name1 = realloc(name1, MAXPATHLEN);
			free(name2);

			snprintf(name1, MAXPATHLEN,
				"%s/%s-%s", hive->installdir,
				geist->name, geist->version);

			/* indeed, with the current hny_remove_fn, errors are not checked */
			if(nftw(name1, hny_remove_fn, 1, FTW_DEPTH | FTW_PHYS) != 0) {
				error = hny_errno(errno);
			}

			free(name1);
			closedir(dirp);
		} else {
			error = HnyErrorUnavailable;
		}
	} else {
		error = HnyErrorInvalidArgs;
	}

	pthread_mutex_unlock(&hive->mutex);

	return error;
}

