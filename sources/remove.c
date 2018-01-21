/*
	remove.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <sys/param.h> /* MAXPATHLEN */
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h> /* remove() */
#include <stdlib.h>
#include <ftw.h>
#include <errno.h>

static int hny_remove_fn(const char *path, const struct stat *st, int type, struct FTW* ftw) {
	if(remove(path) == -1) {
		perror("hny remove package");
	}

	return 0;
}

enum hny_error hny_remove(enum hny_removal removal, const struct hny_geist *geist) {
	enum hny_error error = HnyErrorNone;

	if(hny_check_geister(geist, 1) != HnyErrorNone) {
		return HnyErrorInvalidArgs;
	}

	pthread_mutex_lock(&hive->mutex);

	if(removal == HnyRemovePackage
		&& geist->version != NULL) {
		struct hny_geist *active;
		size_t count, i;
		char path[MAXPATHLEN];

		pthread_mutex_unlock(&hive->mutex);

		/* REMOVE LINKS HERE */

		active = hny_list(HnyListActive, &count);

		pthread_mutex_lock(&hive->mutex);

		for(i = 0; i < count; i++) {
			struct hny_geist *target;

			pthread_mutex_unlock(&hive->mutex);

			target = hny_status(&active[i]);

			pthread_mutex_lock(&hive->mutex);

			if(target != NULL) {
				if(hny_equals_geister(geist, target)) {
					snprintf(path, MAXPATHLEN,
						"%s/%s", hive->installdir,
						active[i].name);

					if(unlink(path) == -1) {
						perror("hny remove of active");
					}
				}

				hny_free_geister(target, 1);
			}
		}

		snprintf(path, MAXPATHLEN,
			"%s/%s-%s", hive->installdir,
			geist->name, geist->version);

		/* indeed, with the current hny_remove_fn, errors are not checked */
		if(nftw(path, hny_remove_fn, 1, FTW_DEPTH | FTW_PHYS) != 0) {
			error = hny_errno(errno);
		}

		hny_free_geister(active, count);
	} else if(removal == HnyRemoveData) {
		pid_t pid = fork();

		if(pid == 0) {
			char filename[MAXPATHLEN];
			char *argv[] = { "rmdata", NULL };
			extern char **environ;

			if(geist->version == NULL) {
				snprintf(filename, MAXPATHLEN,
					"%s/%s/hny/rmdata", hive->installdir,
					geist->name);
			} else {
				snprintf(filename, MAXPATHLEN,
					"%s/%s-%s/hny/rmdata", hive->installdir,
					geist->name, geist->version);
			}

			execve(filename, argv, environ);

			exit(EXIT_FAILURE);
		} else {
			int status;

			if(waitpid(pid, &status, 0) == -1) {
				error = HnyErrorUnavailable;
			} else if(!WIFEXITED(status)
				|| WEXITSTATUS(status) != 0) {
				error = HnyErrorUnavailable;
			}
		}
	} else {
		error = HnyErrorInvalidArgs;
	}

	pthread_mutex_unlock(&hive->mutex);

	return error;
}

