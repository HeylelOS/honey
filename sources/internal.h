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

enum hny_error hny_errno(int err);

/* _HNY_INTERNAL_H */
#endif
