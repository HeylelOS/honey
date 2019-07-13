/*
	status.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE
*/
#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
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
	}
#ifdef HNY_VERBOSE
	else {
		perror("hny_status readlinkat");
	}
#endif

	return NULL;
}

enum hny_error
hny_status(struct hny *hny,
	const struct hny_geist *geist,
	struct hny_geist **target) {
	enum hny_error retval = HNY_ERROR_NONE;

	if(hny_check_geister(geist, 1) == HNY_ERROR_NONE) {

		if(hny_lock(hny) == HNY_ERROR_NONE) {
			char *buffer;

			if((buffer = malloc((NAME_MAX + 1) * 2)) != NULL
				&& hny_fillname(buffer, NAME_MAX + 1, geist) != -1) {
				char *name = hny_target(dirfd(hny->dirp), buffer,
					buffer + NAME_MAX + 1, NAME_MAX + 1);

				if(name != NULL) {
					*target = malloc(sizeof(**target));

					(*target)->name = strsep(&name, "-");
					(*target)->version = name;
				} else {
					retval = HNY_ERROR_MISSING;
				}

			}

			free(buffer);

			hny_unlock(hny);
		} else {
			retval = HNY_ERROR_UNAVAILABLE;
		}
	} else {
		retval = HNY_ERROR_INVALID_ARGS;
	}

	return retval;
}

