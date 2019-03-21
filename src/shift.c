/*
	shift.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>

enum hny_error
hny_shift(hny_t *hny,
	const char *geist,
	const struct hny_geist *package) {
	enum hny_error retval = HNY_ERROR_NONE;

	if(hny_check_name(geist) == HNY_ERROR_NONE
		&& hny_check_geister(package, 1) == HNY_ERROR_NONE) {

		if(hny_lock(hny) == HNY_ERROR_NONE) {
			struct stat st;
			char name[NAME_MAX + 1]; /* NAME_MAX doesn't include nul termination */

			hny_fillname(name, sizeof(name), package);

			/* We follow symlink, this is only to check if the file exists */
			if(fstatat(dirfd(hny->dirp), name, &st, 0) == 0) {
				/* We check if the target exists, if it does, we unlink it */
				if((fstatat(dirfd(hny->dirp), geist, &st, 0) == 0)
					&& (unlinkat(dirfd(hny->dirp), geist, 0) == -1)) {
					retval = hny_errno(errno);
#ifdef HNY_VERBOSE
					perror("hny_shift unlinkat");
#endif
				}

				if(retval == HNY_ERROR_NONE) {
					if(symlinkat(name, dirfd(hny->dirp), geist) == -1) {
						retval = hny_errno(errno);
#ifdef HNY_VERBOSE
						perror("hny_shift symlinkat");
#endif
					}
				}
			} else {
				retval = hny_errno(errno);
#ifdef HNY_VERBOSE
				perror("hny_shift fstatat");
#endif
			}

			hny_unlock(hny);
		} else {
			retval = HNY_ERROR_UNAVAILABLE;
		}

	} else {
		retval = HNY_ERROR_INVALID_ARGS;
	}

	return retval;
}

