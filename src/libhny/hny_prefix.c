/* SPDX-License-Identifier: BSD-3-Clause */
#define _XOPEN_SOURCE 700
#include "hny_prefix.h"

#include <stdlib.h>
#include <sys/file.h>
#include <dirent.h>
#include <errno.h>

int
hny_open(struct hny **hnyp, const char *path, int flags) {
	struct hny * const hny = malloc(sizeof (*hny));
	int errcode;

	if (hny == NULL) {
		errcode = errno;
		goto hny_open_err0;
	}

	hny->path = realpath(path, NULL);
	if (hny->path == NULL) {
		errcode = errno;
		goto hny_open_err1;
	}

	hny->dirp = opendir(hny->path);
	if (hny->dirp == NULL) {
		errcode = errno;
		goto hny_open_err2;
	}

	*hnyp = hny;

	return 0;
hny_open_err2:
	free(hny->path);
hny_open_err1:
	free(hny);
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
hny_flags(struct hny *hny, int flags) {
	const int oldflags = hny->flags;

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

	if (!(hny->flags & HNY_FLAGS_BLOCK)) {
		flags |= LOCK_NB;
	}

	if (flock(dirfd(hny->dirp), flags) != 0) {
		return errno;
	}

	return 0;
}

void
hny_unlock(struct hny *hny) {
	flock(dirfd(hny->dirp), LOCK_UN);
}

