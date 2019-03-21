/*
	erase.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <errno.h>

static int
hny_remove_fn(const char *path,
	const struct stat *st,
	int type,
	struct FTW *ftw) {

	if(remove(path) == -1) {
#ifdef HNY_VERBOSE
		perror("hny_erase remove");
#endif
		return 1;
	}

	return 0;
}

static enum hny_error
hny_remove_recursive(const char *path) {
	enum hny_error retval = HNY_ERROR_NONE;
	struct rlimit rl;
	int fdlimit;

	if(getrlimit(RLIMIT_NOFILE, &rl) == 0) {
		fdlimit = rl.rlim_cur;
	} else {
		fdlimit = 1024; /* Arbitrary limit, better than panic */
#ifdef HNY_VERBOSE
		perror("hny_erase getrlimit");
#endif
	}

	if(nftw(path, hny_remove_fn, fdlimit, FTW_DEPTH | FTW_PHYS) != 0) {
		retval = hny_errno(errno);
	}

	return retval;
}

enum hny_error
hny_erase(hny_t *hny,
	const struct hny_geist *geist) {
	enum hny_error retval = HNY_ERROR_NONE;

	if(hny_check_geister(geist, 1) == HNY_ERROR_NONE) {

		if(hny_lock(hny) == HNY_ERROR_NONE) {
			if(geist->version != NULL) {
				const size_t size = strlen(hny->path) + 3
					+ strlen(geist->name) + strlen(geist->version);
				char *prefix = malloc(size);

				snprintf(prefix, size,
					"%s/%s-%s", hny->path,
					geist->name, geist->version);
				retval = hny_remove_recursive(prefix);

				free(prefix);
			} else {
				if(unlinkat(dirfd(hny->dirp), geist->name, 0) == -1) {
					retval = hny_errno(errno);
#ifdef HNY_VERBOSE
					perror("hny_erase unlinkat");
#endif
				}
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

