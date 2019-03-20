/*
	internal.h
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#ifndef INTERNAL_H
#define INTERNAL_H

#ifdef __linux__
#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE
#endif

#include <hny.h>

#include <dirent.h> /* DIR */
#include <sys/types.h> /* ssize_t size_t */

struct hny {
	DIR *dirp;  /* export prefix */
	char *path; /* export prefix absolute path */
	int flags;  /* blocking behavior */
};

enum hny_error
hny_lock(hny_t *hny);

void
hny_unlock(hny_t *hny);

enum hny_error
hny_errno(int err);

ssize_t
hny_fillname(char *buf,
	size_t bufsize,
	const struct hny_geist *geist);

/* INTERNAL_H */
#endif
