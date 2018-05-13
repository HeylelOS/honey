/*
	internal.h
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#ifndef INTERNAL_H
#define INTERNAL_H

#include <hny.h>

#include <dirent.h>
#include <pthread.h>

struct hny {
	DIR *dirp;				/* export prefix */
	char *path;				/* export prefix absolute path */

	pthread_mutex_t mutex;
};

_Bool
hny_lock(hny_t hny);

void
hny_unlock(hny_t hny);

enum hny_error
hny_errno(int err);

ssize_t
hny_fill_packagename(char *buf,
	size_t bufsize,
	const struct hny_geist *geist);

enum hny_error
hny_remove_recursive(const char *path);

char *
hny_target(int dirfd,
	char *orig,
	char *target,
	size_t bufsize);

/* INTERNAL_H */
#endif
