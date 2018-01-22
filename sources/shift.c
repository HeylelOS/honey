/*
	links.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <sys/stat.h>
#include <sys/dir.h>
#include <limits.h> /* NAME_MAX */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

enum hny_error hny_shift(const char *geist, const struct hny_geist *package) {
	enum hny_error error = HnyErrorNone;
	DIR *dirp;

	if(hny_check_geister(package, 1) != HnyErrorNone) {
		return HnyErrorInvalidArgs;
	}

	pthread_mutex_lock(&hive->mutex);

	if((dirp = opendir(hive->installdir)) != NULL) {
		struct stat st;
		char name[NAME_MAX];

		hny_fill_packagename(name, sizeof(name), package);

		/* We follow symlink, this is only to check if the file exists */
		if(fstatat(dirfd(dirp), name, &st, 0) == 0) {
			/* We check if the target exists, if it does, we unlink it */
			if((fstatat(dirfd(dirp), geist, &st, 0) == 0)
				&& (unlinkat(dirfd(dirp), geist, 0) == -1)) {
				error = hny_errno(errno);
			}

			if(error == HnyErrorNone) {
				if(symlinkat(name, dirfd(dirp), geist) == -1) {
					error = hny_errno(errno);
				}
			}
		} else {
			error = hny_errno(errno);
		}

		closedir(dirp);
	} else {
		error = hny_errno(errno);
	}

	pthread_mutex_unlock(&hive->mutex);

	return error;
}

