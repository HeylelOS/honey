/* SPDX-License-Identifier: BSD-3-Clause */
#include "hny_internal.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <err.h>

int
hny_spawn(struct hny *hny, const char *entry, const char *path, pid_t *pid) {
	int errcode = 0;
	pid_t spawned;

	if(hny_type_of(entry) == HNY_TYPE_NONE) {
		return EINVAL;
	}

	if((spawned = fork()) == 0) {
		char *argv[2] = { strdup(path), NULL };

		if(*argv == NULL) {
			err(HNY_SPAWN_STATUS_ERROR, "Unable to create arguments list");
		}
		*argv = basename(*argv);

		if(setenv("HNY_PREFIX", hny->path, 1) == -1 || setenv("HNY_ENTRY", entry, 1) == -1) {
			err(HNY_SPAWN_STATUS_ERROR, "Unable to setup environment variables");
		}

		if(fchdir(dirfd(hny->dirp)) == -1 || chdir(entry) == -1) {
			err(HNY_SPAWN_STATUS_ERROR, "Unable to chdir %s/%s", hny->path, entry);
		}

		if(execv(path, argv) == -1) {
			err(HNY_SPAWN_STATUS_ERROR, "Unable to execv '%s' for %s", path, entry);
		}
	} else if(spawned > 0) {
		*pid = spawned;
	} else {
		errcode = errno;
	}

	return errcode;
}

