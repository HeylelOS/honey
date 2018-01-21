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

		if(package->version != NULL) {
			snprintf(name, NAME_MAX,
				"%s-%s", package->name, package->version);
		} else {
			strncpy(name, package->name, NAME_MAX);
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
	} else {
		error = hny_errno(errno);
	}

	pthread_mutex_unlock(&hive->mutex);

	return error;
}

struct hny_geist *hny_status(const struct hny_geist *geist) {
	struct hny_geist *target = NULL;
	DIR *dirp;

	if(hny_check_geister(geist, 1) != HnyErrorNone) {
		return NULL;
	}

	pthread_mutex_lock(&hive->mutex);

	if((dirp = opendir(hive->installdir)) != NULL) {
		/* NAME_MAX because of the package dir format */
		char *path1, *path2, *swap;
		ssize_t length;

		path1 = malloc(NAME_MAX);
		path2 = malloc(NAME_MAX);

		if(geist->version == NULL) {
			path1 = strncpy(path1, geist->name, NAME_MAX);
		} else {
			snprintf(path1, NAME_MAX,
				"%s-%s", geist->name, geist->version);
		}

		do {
			length = readlinkat(dirfd(dirp), path1,
						path2, NAME_MAX);
			swap = path1;
			path1 = path2;
			path2 = swap;
		} while(length != -1);

		if(errno == EINVAL) {
			path2 = realloc(path2, strlen(path2));
			target = malloc(sizeof(*target));

			target->name = strsep(&path2, "-");
			target->version = path2;
		} else {
			free(path2);
		}

		free(path1);
		closedir(dirp);
	}

	pthread_mutex_unlock(&hive->mutex);

	return target;
}

