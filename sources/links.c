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
		ssize_t length = NAME_MAX - 1;

		path1 = malloc(NAME_MAX);
		path2 = malloc(NAME_MAX);

		if(geist->version == NULL) {
			path2 = strncpy(path2, geist->name, NAME_MAX);
		} else {
			snprintf(path2, NAME_MAX,
				"%s-%s", geist->name, geist->version);
		}

		do {
			path2[length] = '\0';
			swap = path1;
			path1 = path2;
			path2 = swap;

			length = readlinkat(dirfd(dirp), path1,
						path2, NAME_MAX);
		} while(length != -1);

		if(errno == EINVAL) {
			path1 = realloc(path1, strlen(path1));
			target = malloc(sizeof(*target));

			target->name = strsep(&path1, "-");
			target->version = path1;
		} else {
			if(errno == ENOENT) {
				/* Clean broken symlink */
				unlinkat(dirfd(dirp), path2, 0);
			}
			free(path1);
		}

		free(path2);
		closedir(dirp);
	}

	pthread_mutex_unlock(&hive->mutex);

	return target;
}

