/*
	internal.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE
*/
#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <errno.h>

enum hny_error
hny_lock(hny_t *hny) {

	if(flock(dirfd(hny->dirp),
		LOCK_EX | ((hny->flags & HNY_FLAGS_BLOCK) == 0 ? LOCK_NB : 0))
			== -1) {
#ifdef HNY_VERBOSE
		if(errno != EWOULDBLOCK) {
			perror("hny_lock flock");
		}
#endif
		return HNY_ERROR_UNAVAILABLE;
	}

	return HNY_ERROR_NONE;
}

void
hny_unlock(hny_t *hny) {

	flock(dirfd(hny->dirp), LOCK_UN);
}

enum hny_error
hny_errno(int err) {

	switch(err) {
		case EBADF:
		case ENOENT:
			return HNY_ERROR_MISSING;
		case EAGAIN:
		case ENFILE:
		case ENOMEM:
			return HNY_ERROR_UNAVAILABLE;
		case ELOOP:
		case ENAMETOOLONG:
		case ENOTEMPTY:
			return HNY_ERROR_INVALID_ARGS;
		case EACCES:
		case EPERM:
			return HNY_ERROR_UNAUTHORIZED;
		default:
			break;
	}

	return HNY_ERROR_NONE;
}

ssize_t
hny_fillname(char *buf,
	size_t bufsize,
	const struct hny_geist *geist) {
	ssize_t written = -1;

	if(geist->version != NULL) {
		int end = snprintf(buf, bufsize,
			"%s-%s", geist->name, geist->version);

		if(end >= 0
			&& end < bufsize) {
			written = (ssize_t) end;
		}
	} else {
		char *end = stpncpy(buf,
			geist->name, bufsize);

		if(end < buf + bufsize) {
			written = (ssize_t) (end - buf);
		}
	}

	return written;
}

