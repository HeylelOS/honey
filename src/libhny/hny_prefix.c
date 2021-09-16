/* SPDX-License-Identifier: BSD-3-Clause */
#include "hny_internal.h"

#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <errno.h>

int
hny_open(struct hny **hnyp,
	const char *path,
	int flags) {
	int errcode;

	if(*path != '/') {
		errcode = EINVAL;
		goto hny_open_err0;
	}

	DIR *dirp = opendir(path);
	if(dirp == NULL) {
		errcode = errno;
		goto hny_open_err0;
	}

	char *newpath = strdup(path);
	if(newpath == NULL) {
		errcode = errno;
		goto hny_open_err1;
	}

	struct hny *hny = malloc(sizeof(*hny));
	if(hny == NULL) {
		errcode = errno;
		goto hny_open_err2;
	}

	hny->dirp = dirp;
	hny->path = newpath;
	hny->flags = flags;

	*hnyp = hny;

	return 0;
hny_open_err2:
	free(newpath);
hny_open_err1:
	closedir(dirp);
hny_open_err0:
	return errcode;
}

void
hny_close(struct hny *hny) {

	closedir(hny->dirp);
	free(hny->path);
	free(hny);
}

int
hny_flags(struct hny *hny,
	int flags) {
	int oldflags = hny->flags;

	hny->flags = flags;

	return oldflags;
}

const char *
hny_path(struct hny *hny) {

	return hny->path;
}

int
hny_lock(struct hny *hny) {
	int flags = LOCK_EX;

	if((hny->flags & HNY_FLAGS_BLOCK) == 0) {
		flags |= LOCK_NB;
	}

	if(flock(dirfd(hny->dirp), flags) == -1) {
		return errno;
	}

	return 0;
}

void
hny_unlock(struct hny *hny) {

	flock(dirfd(hny->dirp), LOCK_UN);
}

