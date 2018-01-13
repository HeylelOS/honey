#ifndef _HNY_H
#define _HNY_H

#define HNY_OK			0
#define HNY_ERROR_INVALIDARGS	1
#define HNY_ERROR_UNAVAILABLE	2
#define HNY_ERROR_NONEXISTANT	3

#define HNY_CONNECT_WAIT	1 << 0

#define HNY_STANDARD	1 << 0
#define HNY_GEIST		1 << 1

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
	HNY_REPAIR			/* Repair package installation (cleanup and file check) */
};

enum hny_synchronicity {
	HNY_ASYNC = 0,
	HNY_SYNC = 1
};

struct hny_callback {
	union {
		void (*fetch_list)(const hny_info_fetch_t);
		void (*fetch)(const hny_info_fetch_t);
	} handler;
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
	it then connects to every providers given by the system configuration,
	therefore:
		hny-$UID-sem : wait acquire if flags = HNY_CONNECT_WAIT
			returns HNY_ERROR_UNAVAILABLE if cannot acquire
**/
int hny_connect(int flags);

/**
	Frees all values of the library, release the semaphore and disconnects all connections
	from providers
**/
void hny_disconnect();

/**
	Adds provider url to the list of current providers
	returns 1 if added, 0 elsewhere
**/
int hny_provider_add(const char *url, int flags);

/********************
  OPERATIONS SECTION
*********************/

/**
	Tries to fetch all packages referencing geist with flags
	returns all packages in packages, which is a pointer to a valid realloc pointer
	and must be free afterwards,
	returns the number of packages fetched in fetched
**/
int hny_list_provider(struct hny_geist geist, struct hny_geist **packages, size_t *fetched, int flags);

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
