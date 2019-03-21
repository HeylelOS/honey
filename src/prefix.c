/*
	prefix.c
	Copyright (c) 2019, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

enum hny_error
hny_open(const char *path,
	int flags,
	hny_t **hnyp) {
	enum hny_error retval = HNY_ERROR_NONE;
	DIR *dirp;

	errno = ENOTDIR; /* If not absolute path */
	if(*path == '/'
		&& (dirp = opendir(path)) != NULL) {
		size_t pathsize = strlen(path) + 1;
		char *ptr = malloc(sizeof(**hnyp) + pathsize);

		if(ptr != NULL) {
			*hnyp = (hny_t *)ptr;

			(*hnyp)->dirp = dirp;

			ptr += sizeof(**hnyp);
			strncpy(ptr, path, pathsize);
			(*hnyp)->path = ptr;

			(*hnyp)->flags = flags;
		} else {
			retval = HNY_ERROR_UNAVAILABLE;
#ifdef HNY_VERBOSE
			perror("hny_open malloc");
#endif
			closedir(dirp);
		}
	} else {
		switch(errno) {
		case EACCES:
			retval = HNY_ERROR_UNAUTHORIZED;
			break;
		case ENOENT:
			retval = HNY_ERROR_MISSING;
			break;
		case ENOTDIR:
			retval = HNY_ERROR_INVALID_ARGS;
			break;
		default:
			retval = HNY_ERROR_UNAVAILABLE;
			break;
		}
#ifdef HNY_VERBOSE
		perror("hny_open opendir");
#endif
	}

	return retval;
}

void
hny_close(hny_t *hny) {

	hny_lock(hny);
	hny_unlock(hny);
	closedir(hny->dirp);
	free(hny);
}

int
hny_flags(hny_t *hny,
	int flags) {
	int oldflags = hny->flags;

	hny_lock(hny);
	hny->flags = flags;
	hny_unlock(hny);

	return oldflags;
}

