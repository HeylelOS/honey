/*
	internal.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <stdbool.h>
#include <sys/file.h>
#include <stdio.h>
#include <string.h>
#include <ftw.h>
#include <errno.h>
#ifdef __linux__
#include <dirent.h>
#include <unistd.h>
#endif

_Bool
hny_lock(hny_t hny) {

	pthread_mutex_lock(&hny->mutex);

	if(flock(dirfd(hny->dirp), LOCK_EX) == -1) {
		/* perror("flock"); */
		return false;
	}

	return true;
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

static int
hny_remove_fn(const char *path,
	const struct stat *st,
	int type,
	struct FTW* ftw) {

	if(remove(path) == -1) {
		/* perror("hny remove package"); */
	}

	return 0;
}

enum hny_error
hny_remove_recursive(const char *path) {
	enum hny_error error = HnyErrorNone;

	/*
		indeed, with the current hny_remove_fn,
		errors are not checked
	*/
	if(nftw(path, hny_remove_fn, 1, FTW_DEPTH | FTW_PHYS) != 0) {
		error = hny_errno(errno);
	}

	return error;
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

