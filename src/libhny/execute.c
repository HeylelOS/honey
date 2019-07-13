/*
	execute.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE
*/
#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>

static enum hny_error
hny_spawn(struct hny *hny,
	const struct hny_geist *geist,
	char *name) {
	enum hny_error retval = HNY_ERROR_NONE;
	pid_t pid = fork();

	if(pid == 0) {
		extern char **environ;
		char *argv[2] = { NULL };
		char path[PATH_MAX];
		size_t prefixsize = stpncpy(path, hny->path, sizeof(path)) - path + 1;
		ssize_t filled = hny_fillname(path + prefixsize,
			PATH_MAX - prefixsize, geist);

		if(filled == -1
			|| setenv("HNY_PREFIX", hny->path, 1) != 0
			|| putenv("HNY_ERROR_NONE=0") != 0
			|| putenv("HNY_ERROR_INVALID_ARGS=1") != 0
			|| putenv("HNY_ERROR_UNAVAILABLE=2") != 0
			|| putenv("HNY_ERROR_MISSING=3") != 0
			|| putenv("HNY_ERROR_UNAUTHORIZED=4") != 0) {
			_Exit(HNY_ERROR_UNAVAILABLE);
		}

		argv[0] = name;

		path[prefixsize-1] = '/';

		if(chdir(hny->path) == 0) {
			snprintf(path + prefixsize + filled,
				PATH_MAX - prefixsize - filled,
				"/hny/%s", name);

			execve(path, argv, environ);

#ifdef HNY_VERBOSE
			perror("hny_execute execve");
#endif
		}
#ifdef HNY_VERBOSE
		else {
			perror("hny_execute chdir");
		}
#endif

		_Exit(HNY_ERROR_UNAVAILABLE);
	} else {
		int status;

		if(waitpid(pid, &status, 0) != -1
			&& WIFEXITED(status)) {
			retval = WEXITSTATUS(status);
		} else {
			retval = HNY_ERROR_UNAVAILABLE;
		}
	}

	return retval;
}

enum hny_error
hny_execute(struct hny *hny,
	enum hny_action action,
	const struct hny_geist *geist) {
	enum hny_error retval = HNY_ERROR_NONE;

	if(hny_check_geister(geist, 1) == HNY_ERROR_NONE) {

		if(hny_lock(hny) == HNY_ERROR_NONE) {
			char *straction;

			switch(action) {
			case HNY_ACTION_SETUP:
				straction = "setup";
				break;
			case HNY_ACTION_CLEAN:
				straction = "clean";
				break;
			case HNY_ACTION_RESET:
				straction = "reset";
				break;
			case HNY_ACTION_CHECK:
				straction = "check";
				break;
			case HNY_ACTION_PURGE:
				straction = "purge";
				break;
			default:
				straction = NULL;
				break;
			}

			if(straction != NULL) {
				retval = hny_spawn(hny, geist, straction);
			} else {
				retval = HNY_ERROR_INVALID_ARGS;
			}

			hny_unlock(hny);
		} else {
			retval = HNY_ERROR_UNAVAILABLE;
		}
	} else {
		retval = HNY_ERROR_INVALID_ARGS;
	}

	return retval;
}

