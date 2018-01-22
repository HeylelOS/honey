/*
	links.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <sys/dir.h>
#include <limits.h> /* NAME_MAX */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct hny_geist *hny_status(const struct hny_geist *geist) {
	struct hny_geist *target = NULL;
	DIR *dirp;

	if(hny_check_geister(geist, 1) != HnyErrorNone) {
		return NULL;
	}

	pthread_mutex_lock(&hive->mutex);

	if((dirp = opendir(hive->installdir)) != NULL) {
		/* NAME_MAX because of the package dir format */
		char *name1, *name2, *name;

		name1 = malloc(NAME_MAX);
		name2 = malloc(NAME_MAX);

		hny_fill_packagename(name2, NAME_MAX, geist);

		name = hny_target(dirfd(dirp), name2, name1, NAME_MAX);
		if(name != NULL) {
			target = malloc(sizeof(*target));

			target->name = strsep(&name, "-");
			target->version = name;
		}

		free(name1);
		free(name2);
		closedir(dirp);
	}

	pthread_mutex_unlock(&hive->mutex);

	return target;
}

char *hny_target(int dirfd, char *orig, char *target, size_t bufsize) {
	size_t length = bufsize - 1;
	char *swap;

	do {
		orig[length] = '\0';
		swap = target;
		target = orig;
		orig = swap;

		length = readlinkat(dirfd, target,
					orig, bufsize);
	} while(length != -1);

	if(errno == EINVAL) {
		return strdup(target);
	} else if(errno == ENOENT) {
		/* broken symlink */
		unlinkat(dirfd, orig, 0);
	}

	return NULL;
}

