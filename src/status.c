/*
	status.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <sys/param.h> /* NAME_MAX */
#include <stdlib.h>
#include <string.h>

enum hny_error
hny_status(hny_t hny,
	const struct hny_geist *geist,
	struct hny_geist **target) {
	enum hny_error error = HnyErrorNone;

	if(hny_check_geister(geist, 1) == HnyErrorNone) {

		if(hny_lock(hny) == HnyErrorNone) {
			/* NAME_MAX because of the package dir format */
			char *name1, *name2, *name;

			name1 = malloc(NAME_MAX);
			name2 = malloc(NAME_MAX);

			hny_fill_packagename(name2, NAME_MAX, geist);

			name = hny_target(dirfd(hny->dirp), name2, name1, NAME_MAX);
			if(name != NULL) {

				*target = malloc(sizeof(**target));

				(*target)->name = strsep(&name, "-");
				(*target)->version = name;
			} else {
				error = HnyErrorNonExistant;
			}

			free(name1);
			free(name2);

			hny_unlock(hny);
		} else {
			error = HnyErrorUnavailable;
		}
	} else {
		error = HnyErrorInvalidArgs;
	}

	return error;
}

