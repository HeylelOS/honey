/*
	execute.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <sys/param.h> /* MAXPATHLEN */
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

enum hny_error hny_execute(enum hny_action action, const struct hny_geist *geist) {
	enum hny_error error = HnyErrorNone;

	pthread_mutex_lock(&hive->mutex);

	if(hny_check_geister(geist, 1) == HnyErrorNone) {
		switch(action) {
			case HnyActionSetup:
				error = hny_run(geist, "hny/setup");
				break;
			case HnyActionDrain:
				error = hny_run(geist, "hny/drain");
				break;
			case HnyActionReset:
				error = hny_run(geist, "hny/reset");
				break;
			case HnyActionCheck:
				error = hny_run(geist, "hny/check");
				break;
			case HnyActionClean:
				error = hny_run(geist, "hny/clean");
				break;
			default:
				error = HnyErrorInvalidArgs;
				break;
		}
	} else {
		error = HnyErrorInvalidArgs;
	}

	pthread_mutex_unlock(&hive->mutex);

	return error;
}

enum hny_error hny_run(const struct hny_geist *geist, char *name) {
	enum hny_error error = HnyErrorNone;
	pid_t pid = fork();

	if(pid == 0) {
		char path[MAXPATHLEN];
		char *argv[2] = { NULL };
		extern char **environ;

		argv[0] = name;

		if(geist->version == NULL) {
			snprintf(path, MAXPATHLEN,
				"%s/%s/", hive->installdir,
				geist->name);
		} else {
			snprintf(path, MAXPATHLEN,
				"%s/%s-%s/", hive->installdir,
				geist->name, geist->version);
		}

		if(chdir(path) == 0) {
			strncat(path, name, MAXPATHLEN);
			execve(path, argv, environ);
		}

		perror("hny");
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

	return error;
}

