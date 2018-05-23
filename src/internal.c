/*
	internal.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <sys/file.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef __linux__
#include <unistd.h>
#endif

enum hny_error
hny_lock(hny_t hny) {

	pthread_mutex_lock(&hny->mutex);

	if(flock(dirfd(hny->dirp),
		LOCK_EX | (hny->block == HNY_NONBLOCK ? LOCK_NB : 0))
			== -1) {
#ifdef HNY_VERBOSE
		if(hny->block != HNY_NONBLOCK
			&& errno != EWOULDBLOCK) {
			perror("hny flock");
		}
#endif
		return HnyErrorUnavailable;
	}

	return HnyErrorNone;
}

void
hny_unlock(hny_t hny) {

	flock(dirfd(hny->dirp), LOCK_UN);
	pthread_mutex_unlock(&hny->mutex);
}

enum hny_error
hny_errno(int err) {

	switch(err) {
		case EBADF:
		case ENOENT:
			return HnyErrorNonExistant;
		case EAGAIN:
		case ENFILE:
		case ENOMEM:
			return HnyErrorUnavailable;
		case ELOOP:
		case ENAMETOOLONG:
		case ENOTEMPTY:
			return HnyErrorInvalidArgs;
		case EACCES:
		case EPERM:
			return HnyErrorUnauthorized;
		default:
			break;
	}

	return HnyErrorNone;
}

ssize_t
hny_fill_packagename(char *buf,
	size_t bufsize,
	const struct hny_geist *geist) {
	ssize_t written;

	if(geist->version != NULL) {
		int end = snprintf(buf, bufsize,
			"%s-%s", geist->name, geist->version);

		if(end < 0
			|| &buf[end] >= &buf[bufsize]) {
			return -1;
		}

		written = (ssize_t) end;
	} else {
		char *end = stpncpy(buf,
			geist->name, bufsize);

		if(end >= &buf[bufsize]) {
			return -1;
		}

		written = (ssize_t) (end - buf);
	}

	return written;
}

char *
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

