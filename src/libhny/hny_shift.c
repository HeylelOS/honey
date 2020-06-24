/*
	hny_shift.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE
*/
#include "hny_internal.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int
hny_shift(struct hny *hny,
	const char *geist,
	const char *target) {
	int errcode = 0;

	if(hny_type_of(geist) == HNY_TYPE_GEIST
		&& hny_type_of(target) != HNY_TYPE_NONE) {

		if(faccessat(dirfd(hny->dirp), geist, F_OK, AT_SYMLINK_NOFOLLOW) == 0
			&& unlinkat(dirfd(hny->dirp), geist, 0) == -1) {
			errcode = errno;
		}

		if(errcode == 0
			&& symlinkat(target, dirfd(hny->dirp), geist) == -1) {
			errcode = errno;
		}
	} else {
		errcode = EINVAL;
	}

	return errcode;
}

