/*
	erase.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <stdio.h> /* remove() */
#include <unistd.h> /* unlinkat() */
#include <sys/param.h> /* MAXPATHLEN, NAME_MAX */
#include <sys/resource.h> /* getrlimit() */
#include <string.h>
#include <alloca.h>
#include <errno.h>
#include <ftw.h>

static int
hny_remove_fn(const char *path,
	const struct stat *st,
	int type,
	struct FTW *ftw) {

	if(remove(path) == -1) {
#ifdef HNY_VERBOSE
		perror("hny remove package");
#endif
		return 1;
	}

	return 0;
}

static enum hny_error
hny_remove_recursive(const char *path) {
	enum hny_error error = HnyErrorNone;
	struct rlimit rl;
	int fdlimit;

	if(getrlimit(RLIMIT_NOFILE, &rl) == 0) {
		fdlimit = rl.rlim_cur;
	} else {
#ifdef HNY_VERBOSE
		perror("hny erase getrlimit");
#endif
		fdlimit = 1024; /* Arbitrary limit, better than panic */
	}

	if(nftw(path, hny_remove_fn, fdlimit, FTW_DEPTH | FTW_PHYS) != 0) {
		error = hny_errno(errno);
	}

	return error;
}

enum hny_error
hny_erase(hny_t hny,
	const struct hny_geist *geist) {
	enum hny_error error = HnyErrorNone;

	if(hny_check_geister(geist, 1) == HnyErrorNone) {

		if(hny_lock(hny) == HnyErrorNone) {
			if(geist->version != NULL) {
				const size_t length = strlen(hny->path) + 2
					+ strlen(geist->name) + strlen(geist->version);
				char *prefix = alloca(length);

				snprintf(prefix, MAXPATHLEN,
					"%s/%s-%s", hny->path,
					geist->name, geist->version);
				error = hny_remove_recursive(prefix);
			} else {
				if(unlinkat(dirfd(hny->dirp), geist->name, 0) == -1) {
					error = hny_errno(errno);
				}
			}

			hny_unlock(hny);
		} else {
			error = HnyErrorUnavailable;
		}
	} else {
		error = HnyErrorInvalidArgs;
	}

	return error;
}

