/* SPDX-License-Identifier: BSD-3-Clause */
#include "hny_prefix.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int
hny_shift(struct hny *hny, const char *geist, const char *target) {
	int errcode = 0;

	if (hny_type_of(geist) == HNY_TYPE_GEIST && hny_type_of(target) != HNY_TYPE_NONE) {

		if (faccessat(dirfd(hny->dirp), geist, F_OK, AT_SYMLINK_NOFOLLOW) == 0 && unlinkat(dirfd(hny->dirp), geist, 0) != 0) {
			errcode = errno;
		}

		if (errcode == 0 && symlinkat(target, dirfd(hny->dirp), geist) != 0) {
			errcode = errno;
		}
	} else {
		errcode = EINVAL;
	}

	return errcode;
}
