/*
	shift.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <sys/stat.h>
#include <sys/param.h> /* NAME_MAX */
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

enum hny_error
hny_shift(hny_t hny,
	const char *geist,
	const struct hny_geist *package) {
	enum hny_error error = HnyErrorNone;

	if(hny_check_name(geist) == HnyErrorNone
		&& hny_check_geister(package, 1) == HnyErrorNone) {

		if(hny_lock(hny) == HnyErrorNone) {
			struct stat st;
			char name[NAME_MAX];

			hny_fill_packagename(name, sizeof(name), package);

			/* We follow symlink, this is only to check if the file exists */
			if(fstatat(dirfd(hny->dirp), name, &st, 0) == 0) {
				/* We check if the target exists, if it does, we unlink it */
				if((fstatat(dirfd(hny->dirp), geist, &st, 0) == 0)
					&& (unlinkat(dirfd(hny->dirp), geist, 0) == -1)) {

					error = hny_errno(errno);
				}

				if(error == HnyErrorNone) {
					if(symlinkat(name, dirfd(hny->dirp), geist) == -1) {
						error = hny_errno(errno);
					}
				}
			} else {
				error = hny_errno(errno);
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

