#ifndef _HNY_H
#define _HNY_H

#define HNY_OK			0
#define HNY_ERROR_INVALIDARGS	1
#define HNY_ERROR_UNAVAILABLE	2
#define HNY_ERROR_NONEXISTANT	3
#define HNY_ERROR_UNAUTHORIZED	4

#define _BSD_SOURCE
/* _BSD_SOURCE is for internal build */
#include <sys/types.h>

enum hny_action {
	HNY_FETCH,			/* Fetch the latest or specified package */
	HNY_INSTALL,		/* Install the archive */
	HNY_LIST_PROVIDER,	/* List all packages following condition from provider */
	HNY_LIST_ALL,		/* List all packages installed */
	HNY_LIST_ACTIVE,	/* List all active packages */
	HNY_REMOVE_TOTAL,	/* Total removal of packages' data */
	HNY_REMOVE_PARTIAL,	/* Partial removal of packages' data */
	HNY_STATUS,			/* Status of package (installed, active, latest) */
	HNY_REPAIR_ALL,		/* Repair package installation (cleanup, files check and config) */
	HNY_REPAIR_CLEAN,	/* Repair package installation (only clean) */
	HNY_REPAIR_CHECK,	/* Repair package installation (only files check) */
	HNY_REPAIR_CONFIG	/* Repair package installation (only config) */
};

enum hny_synchronicity {
	HNY_SYNC = 0,
	HNY_ASYNC = 1
};

struct hny_geist {
	char *name;
	char *version;
};

/********************
  CONNECTION SECTION
*********************/

/**
	hny_connect locks the honey semaphore associated to the current user
	therefore:
		hny-$UID-sem : wait acquire if flags = HNY_CONNECT_WAIT
			returns HNY_ERROR_UNAVAILABLE if cannot acquire
**/
#define HNY_CONNECT_WAIT	1 << 0
int hny_connect(int flags);

/**
	Frees all values of the library, release the semaphore and disconnects all connections
	from providers
**/
void hny_disconnect();

/********************
  OPERATIONS SECTION
*********************/

/**
	Tries to fetch all packages referencing geist
	with flags, from provider
	returns the number of packages fetched in fetched
		and the list, NULL if empty, list must be freed afterward
**/
#define HNY_GEISTER			1 << 0
#define HNY_PACKAGES		1 << 1
#define HNY_DEPENDENCIES	1 << 2
struct hny_geist *hny_fetch(const struct hny_geist *geist, const char *provider, size_t *fetched, int flags);

/**
	Tries to install file package
	with flags, stealth means it only installs the package, it
	doesn't replace the current version
	return HNY_OK on success,
		HNY_NONEXISTANT if one of the file doesn't exist
		HNY_UNAUTHORIZED if a permission stops the install
**/
#define HNY_STEALTH		1 << 0
int hny_install(const char *file, const char *directory, int flags);

/********************
  UTILITIES SECTION
*********************/

/**
	Both following check for errors in the structures
	like if a geist has a name set at NULL
**/
int hny_check_geister(struct hny_geist *geister, size_t n);

/**
	Compare two versions together, qsort compatible
**/
#define HNY_CMP_QSORT(x)	((int (*)(const void *, const void *))(x))
int hny_compare_versions(const char **p1, const char **p2);

#endif
