#include "hny_internal.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define HNY_REMOVE_RECURSION_PATH_DEFAULT_SIZE 128
#define HNY_REMOVE_RECURSION_LOCATIONS_DEFAULT_SIZE 16

struct hny_removal {
	DIR *dirp;

	long *locations;
	size_t count;
	size_t capacity;

	char *path;
	char *name;
	char *pathend;
};

static inline bool
hny_remove_is_dot_or_dot_dot(const char *name) {

	return name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'));
}

static DIR *
hny_remove_opendirat(int dirfd, const char *path, int *errcode) {
	int newdirfd = openat(dirfd, path, O_DIRECTORY | O_NOFOLLOW);
	DIR *newdirp = NULL;

	if(newdirfd >= 0) {
		newdirp = fdopendir(newdirfd);

		if(newdirp == NULL) {
			*errcode = errno;
			close(newdirfd);
		}
	} else {
		*errcode = errno;
	}

	return newdirp;
}

static int
hny_removal_init(struct hny_removal *removal,
	struct hny *hny, const char *package, int *errcode) {

	if((removal->dirp = hny_remove_opendirat(dirfd(hny->dirp), package, errcode)) == NULL) {
		goto hny_removal_init_err0;
	}

	removal->count = 0;
	removal->capacity = HNY_REMOVE_RECURSION_LOCATIONS_DEFAULT_SIZE;
	if((removal->locations = malloc(sizeof(*removal->locations) * removal->capacity)) == NULL) {
		goto hny_removal_init_err1;
	}
	*removal->locations = 0;

	if((removal->path = malloc(sizeof(*removal->path) * HNY_REMOVE_RECURSION_PATH_DEFAULT_SIZE)) == NULL) {
		goto hny_removal_init_err2;
	}
	*removal->path = '/';
	removal->name = removal->path + 1;
	removal->pathend = removal->path + HNY_REMOVE_RECURSION_PATH_DEFAULT_SIZE;

	return 0;
hny_removal_init_err2:
	free(removal->locations);
hny_removal_init_err1:
	closedir(removal->dirp);
hny_removal_init_err0:
	return -1;
}

static void
hny_removal_deinit(struct hny_removal *removal) {

	free(removal->path);
	free(removal->locations);
	closedir(removal->dirp);
}

static void
hny_removal_push(struct hny_removal *removal, const char *name, int *errcode) {
	char *namenul;
	DIR *dirp;

	while((namenul = stpncpy(removal->name, name,
		removal->pathend - removal->name)) >= removal->pathend - 1) {
		size_t length = removal->name - removal->path,
			capacity = removal->pathend - removal->path;
		char *newpath = realloc(removal->path, capacity * 2);

		if(newpath != NULL) {
			removal->path = newpath;
			removal->name = newpath + length;
			removal->pathend = newpath + capacity * 2;
		} else {
			*errcode = errno;
			removal->locations[removal->count]++;
			return;
		}
	}

	if(removal->count == removal->capacity) {
		long *newlocations = realloc(removal->locations,
			sizeof(*removal->locations) * removal->capacity * 2);

		if(newlocations != NULL) {
			removal->locations = newlocations;
			removal->capacity *= 2;
		} else {
			*errcode = errno;
			removal->locations[removal->count]++;
			return;
		}
	}

	if((dirp = hny_remove_opendirat(dirfd(removal->dirp), removal->name, errcode)) == NULL) {
		*errcode = errno;
		removal->locations[removal->count]++;
		return;
	}

	closedir(removal->dirp);
	removal->dirp = dirp;
	*namenul = '/';
	removal->name = namenul + 1;
	removal->count++;
}

static int
hny_removal_pop(struct hny_removal *removal, int *errcode) {
	DIR *dirp = hny_remove_opendirat(dirfd(removal->dirp), "..", errcode);

	if(dirp == NULL) {
		*errcode = errno;
		return -1;
	}

	do {
		removal->name--;
	} while(removal->name[-1] != '/');

	removal->count--;
	if(unlinkat(dirfd(dirp), removal->name, AT_REMOVEDIR) == -1) {
		*errcode = errno;
		removal->locations[removal->count]++;
	}

	for(long i = 0; i < removal->locations[removal->count]
		&& (errno = 0, readdir(dirp)) != NULL; i++);

	if(errno != 0) {
		*errcode = errno;
		return -1;
	}

	closedir(removal->dirp);
	removal->dirp = dirp;

	return 0;
}

static int
hny_removal_remove(struct hny_removal *removal, int *errcode) {
	struct dirent *entry = (errno = 0, readdir(removal->dirp));

	if(entry == NULL) {
		if(errno != 0) {
			*errcode = errno;
		}

		if(removal->count == 0) {
			return -1;
		}

		return hny_removal_pop(removal, errcode);
	} else if(entry->d_type == DT_DIR) {
		if(hny_remove_is_dot_or_dot_dot(entry->d_name)) {
			removal->locations[removal->count]++;
		} else {
			hny_removal_push(removal, entry->d_name, errcode);
		}
	} else if(unlinkat(dirfd(removal->dirp), entry->d_name, 0) == -1) {
		*errcode = errno;
		removal->locations[removal->count]++;
	}

	return 0;
}

static int
hny_remove_package(struct hny *hny, const char *package) {
	struct hny_removal removal;
	int errcode = 0;

	if(hny_removal_init(&removal, hny, package, &errcode) == 0) {

		while(hny_removal_remove(&removal, &errcode) == 0);

		if((errcode = hny_lock(hny)) == 0) {
			if(unlinkat(dirfd(hny->dirp), package, AT_REMOVEDIR) == -1) {
				errcode = errno;
			}
			hny_unlock(hny);
		}

		hny_removal_deinit(&removal);
	}

	return errcode;
}

int
hny_remove(struct hny *hny,
	const char *entry) {
	int errcode = 0;

	switch(hny_type_of(entry)) {
	case HNY_TYPE_PACKAGE:
		errcode = hny_remove_package(hny, entry);
		break;
	case HNY_TYPE_GEIST:
		if((errcode = hny_lock(hny)) == 0) {
			if(unlinkat(dirfd(hny->dirp), entry, 0) == -1) {
				errcode = errno;
			}
			hny_unlock(hny);
		}
		break;
	default: /* HNY_TYPE_NONE */
		errcode = EINVAL;
		break;
	}

	return errcode;
}

