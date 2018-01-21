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

		if(package->version != NULL) {
			snprintf(name, NAME_MAX,
				"%s-%s", package->name, package->version);
		} else {
			strcpy(name, package->name);
		}

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
	}

	pthread_mutex_unlock(&hive->mutex);

	return error;
}

enum hny_error hny_status(struct hny_geist *target, const struct hny_geist *geist) {
	if(hny_check_geister(geist, 1) != HnyErrorNone) {
		return HnyErrorInvalidArgs;
	}

	pthread_mutex_lock(&hive->mutex);
	pthread_mutex_unlock(&hive->mutex);

	return HnyErrorNone;
}

