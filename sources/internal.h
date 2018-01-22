/*
	internal.h
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#ifndef _HNY_INTERNAL_H
#define _HNY_INTERNAL_H

#include <hny.h>

/* snprintf requires _BSD_SOURCE */
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>

struct {
	char sem_name[32];
	sem_t *semaphore;
	pthread_mutex_t mutex;

	char *installdir;
	char *prefixdir;
} *hive;

/* GIve the "corresponding" hny_error from the errno given */
enum hny_error hny_errno(int err);

/* Creates the package name in buf, every argument must be valid, no check done */
void hny_fill_packagename(char *buf, size_t bufsize, const struct hny_geist *geist);

/* _HNY_INTERNAL_H */
#endif
