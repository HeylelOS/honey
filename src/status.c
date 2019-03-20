/*
	status.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <sys/param.h> /* NAME_MAX */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static char *
hny_target(int dirfd,
	char *orig,
	char *target,
	size_t bufsize) {
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
		/* reached a non-symlink */
		return strdup(target);
	} else if(errno == ENOENT) {
		/* broken symlink */
		unlinkat(dirfd, orig, 0);
	}

	return NULL;
}

enum hny_error
hny_status(hny_t *hny,
	const struct hny_geist *geist,
	struct hny_geist **target) {
	enum hny_error retval = HNY_ERROR_NONE;

	if(hny_check_geister(geist, 1) == HNY_ERROR_NONE) {

		if(hny_lock(hny) == HNY_ERROR_NONE) {
			/* NAME_MAX because of the package dir format */
			char *name1, *name2, *name;

			name1 = malloc(NAME_MAX);
			name2 = malloc(NAME_MAX);

			hny_fillname(name2, NAME_MAX, geist);

			name = hny_target(dirfd(hny->dirp), name2, name1, NAME_MAX);
			if(name != NULL) {

				*target = malloc(sizeof(**target));

				(*target)->name = strsep(&name, "-");
				(*target)->version = name;
			} else {
				retval = HNY_ERROR_MISSING;
			}

			free(name1);
			free(name2);

			hny_unlock(hny);
		} else {
			retval = HNY_ERROR_UNAVAILABLE;
		}
	} else {
		retval = HNY_ERROR_INVALID_ARGS;
	}

	return retval;
}

