#include "hny_internal.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int
hny_shift(struct hny *hny,
	const char *geist,
	const char *target) {
	int errcode = EINVAL;

	if(hny_type_of(geist) == HNY_TYPE_GEIST
		&& hny_type_of(target) != HNY_TYPE_NONE
		&& (errcode = hny_lock(hny)) == 0) {

		if(faccessat(dirfd(hny->dirp), geist, F_OK, AT_SYMLINK_NOFOLLOW) == 0
			&& unlinkat(dirfd(hny->dirp), geist, 0) == -1) {
			errcode = errno;
		}

		if(errcode == 0
			&& symlinkat(target, dirfd(hny->dirp), geist) == -1) {
			errcode = errno;
		}

		hny_unlock(hny);
	}

	return errcode;
}

