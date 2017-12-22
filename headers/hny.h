#ifndef _HNY_H
#define _HNY_H

#define HNY_OK			0
#define HNY_ERROR_INVALIDARGS	1
#define HNY_ERROR_UNAVAILABLE	2
#define HNY_ERROR_NONEXISTANT	3

#define HNY_CONNECT_WAIT	1 << 0

#define HNY_STANDARD	1 << 0
#define HNY_GEIST	1 << 1

#define _BSD_SOURCE
/* _BSD_SOURCE is for internal build */
#include <sys/types.h>

struct hny_link {
	char *target;
	char *linkname;
};

struct hny_geist {
	char *name;
	char *version;
};

struct hny_package {
	char *name;		/* Name of the package */
	char *version;		/* Version of the package */

	struct hny_geist geist;	/* Which geist it references too */

	time_t date;	/* Release date of package in provider, GMT+0 */

	struct hny_geist *dependencies;
	unsigned int dependencies_count;
};

/**
	hny_connect locks the honey semaphore associated to the current user
	it then connects to every providers given by the system configuration,
	therefore:
		/tmp/hny-$UID-sem : wait acquire if flags = HNY_CONNECT_WAIT
			returns HNY_ERROR_UNAVAILABLE if cannot acquire now
		/etc/hny/providers : reads and connects to every entry in
**/
int hny_connect(int flags);

/**
	Tries to fetch all packages identified with name, version if version is not NULL and flags
	returns all packages in packages, which must be free afterwards, returns the number of packages
	fetched
**/
size_t hny_fetch(const char *name, const char *version, struct hny_package **packages, int flags);

/**
	Comapre two versions together, usable in qsort
*/
int hny_compare_versions(const char **p1, const char **p2);

/**
	Frees all values of the library, release the semaphore and disconnects all connections
	from providers
**/
void hny_disconnect();

#endif
