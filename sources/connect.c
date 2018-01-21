/*
	connect.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <sys/param.h> /* MAXPATHLEN */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

enum hny_error hny_connect(int flags) {
	int wait_ret;

	hny_disconnect();

	hive = malloc(sizeof(*hive));
	if(hive == NULL) {
		return HnyErrorUnavailable;
	}

	snprintf(hive->sem_name, sizeof(hive->sem_name),
		"hny-%u-sem", geteuid());
	/* effective user id because the semaphore gets ownership from
		effective user id */

	hive->semaphore = sem_open(hive->sem_name, O_CREAT, S_IRUSR | S_IWUSR, 1);
	if(hive->semaphore == SEM_FAILED) {
		goto lbl_hny_connect_err1;
	}

	if(flags & HNY_CONNECT_WAIT) {
		wait_ret = sem_wait(hive->semaphore);
	} else {
		wait_ret = sem_trywait(hive->semaphore);
	}

	if(wait_ret == -1) {
		if(errno != EAGAIN) {
			perror("hny semaphore");
		}
		goto lbl_hny_connect_err1;
	}

	hive->installdir = malloc(MAXPATHLEN);
	hive->installdir = getcwd(hive->installdir, MAXPATHLEN);
	snprintf(hive->installdir, MAXPATHLEN,
		"%s/%s", hive->installdir, "packages/");
	hive->installdir = realloc(hive->installdir, strlen(hive->installdir));

	hive->prefixdir = malloc(MAXPATHLEN);
	hive->prefixdir = getcwd(hive->prefixdir, MAXPATHLEN);
	snprintf(hive->prefixdir, MAXPATHLEN,
		"%s/%s", hive->prefixdir, "prefix/");
	hive->prefixdir = realloc(hive->prefixdir, strlen(hive->prefixdir));

	pthread_mutex_init(&hive->mutex, NULL);

	return HnyErrorNone;

lbl_hny_connect_err1:
	free(hive);
	hive = NULL;

	return HnyErrorUnavailable;
}

void hny_disconnect() {
	if(hive != NULL) {
		pthread_mutex_destroy(&hive->mutex);

		sem_post(hive->semaphore);
		sem_close(hive->semaphore);

		free(hive->installdir);
		free(hive);
		hive = NULL;
	}
}

