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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static enum hny_error
hny_spawn(hny_t hny,
	const struct hny_geist *geist,
	char *name) {
	enum hny_error error = HnyErrorNone;
	pid_t pid = fork();

	if(pid == 0) {
		char *argv[2] = { NULL };
		extern char **environ;
		size_t length = strlen(hny->path);

		if(setenv("HNY_PREFIX", hny->path, 1) != 0
			|| putenv("HNY_ERROR_NONE=0") != 0
			|| putenv("HNY_ERROR_INVALID_ARGS=1") != 0
			|| putenv("HNY_ERROR_UNAVAILABLE=2") != 0
			|| putenv("HNY_ERROR_NON_EXISTANT=3") != 0
			|| putenv("HNY_ERROR_UNAUTHORIZED=4") != 0) {
			_Exit(HnyErrorUnavailable);
		}

		argv[0] = name;

		hny->path[length] = '/';
		length++;
		length += hny_fill_packagename(&hny->path[length],
			MAXPATHLEN - length, geist);

		if(length > 0
			&& chdir(hny->path) == 0) {

			snprintf(&hny->path[length], MAXPATHLEN - length,
				"/hny/%s", name);

			execve(hny->path, argv, environ);
		}

		/* perror("hny"); */
		_Exit(HnyErrorUnavailable);
	} else {
		int status;

		if(waitpid(pid, &status, 0) != -1
			&& WIFEXITED(status)) {
			error = WEXITSTATUS(status);
		} else {
			error = HnyErrorUnavailable;
		}
	}

	return error;
}

enum hny_error
hny_execute(hny_t hny,
	enum hny_action action,
	const struct hny_geist *geist) {
	enum hny_error error = HnyErrorNone;

	if(hny_check_geister(geist, 1) == HnyErrorNone) {

		if(hny_lock(hny)) {
			char *straction;

			switch(action) {
			case HnyActionSetup:
				straction = "setup";
				break;
			case HnyActionClean:
				straction = "clean";
				break;
			case HnyActionReset:
				straction = "reset";
				break;
			case HnyActionCheck:
				straction = "check";
				break;
			case HnyActionPurge:
				straction = "purge";
				break;
			default:
				straction = NULL;
				break;
			}

			if(straction != NULL) {
				error = hny_spawn(hny, geist, straction);
			} else {
				error = HnyErrorInvalidArgs;
			}

			hny_unlock(hny);
		} else {
			error = HnyErrorUnavailable;
		}
	} else {
		error = HnyErrorInvalidArgs;
	}

	return error;
}

