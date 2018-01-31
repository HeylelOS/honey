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
} *hive;

/* Gives the "corresponding" hny_error from the errno given */
enum hny_error hny_errno(int err);

/* Creates the package name in buf, every argument must be valid, no check done */
void hny_fill_packagename(char *buf, size_t bufsize, const struct hny_geist *geist);

/*
 climbs symlink given in orig in dir dirfd and returns final link
 if it finds one (a duplicate of name, should be freed) or NULL otherwise,
 if it encounters a dead symlink, it will delete it orig and target are also
 buffers of the same, bufsize, size for the function and there content
 should be consider invalidated when the function returns

 found in status.c
*/
char *hny_target(int dirfd, char *orig, char *target, size_t bufsize);

/*
 runs the script located at path/name and returns
 hny-like error

 found in execute.c
*/
enum hny_error hny_run(const struct hny_geist *geist, char *name);

/*
 recursivly delete the files located at path

 found in remove.c
*/
enum hny_error hny_rm_recur(const char *path);

/* _HNY_INTERNAL_H */
#endif
