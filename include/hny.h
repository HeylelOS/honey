/*
	hny.h
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#ifndef _HNY_H
#define _HNY_H

enum hny_error {
	HnyErrorNone = 0,
	HnyErrorInvalidArgs = 1,
	HnyErrorUnavailable = 2,
	HnyErrorNonExistant = 3,
	HnyErrorUnauthorized = 4
};

#ifdef __linux__
#define _XOPEN_SOURCE 500
#endif
#define _BSD_SOURCE
/* Above are for internal build */
#include <sys/types.h>

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
enum hny_error hny_connect(int flags);

/**
	Frees all values of the library, and releases the semaphore
**/
void hny_disconnect();

/********************
  OPERATIONS SECTION
*********************/

/**
	Tries to install file package in the user package
	directory
**/
enum hny_error hny_install(const char *file);

/**
	Shift replaces the geist named geist
	by the package or geist described in package
**/
enum hny_error hny_shift(const char *geist, const struct hny_geist *package);

/**
	Lists all geister which follow the listing
	condition
**/
enum hny_listing {
	HnyListPackages,
	HnyListActive
};
struct hny_geist *hny_list(enum hny_listing listing, size_t *listed);

/**
**/
enum hny_removal {
	HnyRemovePackage,
	HnyRemoveData
};
enum hny_error hny_remove(enum hny_removal removal, const struct hny_geist *geist);

/**
	Status follows the symlinks and/or file in the
	package dir until it finds a problem or a package
	directory, if it finds a broken symlink it will unlink
	it, if an error occurs, it returns NULL, otherwise
	a pointer to the final geist, which can be freed with
	hny_free_geister
**/
struct hny_geist *hny_status(const struct hny_geist *geist);

/**
**/
#define HNY_REPAIR_CLEAN	1 << 0
#define HNY_REPAIR_CHECK	1 << 1
#define HNY_REPAIR_CONFIG	1 << 2
enum hny_error hny_repair(const struct hny_geist *geist, int flags);

/********************
  UTILITIES SECTION
*********************/

/**
	Free all geister values (names and optionnaly version)
	then frees the vector
**/
void hny_free_geister(struct hny_geist *geister, size_t count);

/**
	Checks if two geister are equals
**/
_Bool hny_equals_geister(const struct hny_geist *g1, const struct hny_geist *g2);

/**
	The following checks for errors in the structures
	like if a geist name is invalid
**/
enum hny_error hny_check_geister(const struct hny_geist *geister, size_t n);

/**
	Compare two versions together, qsort compatible
**/
#define HNY_CMP_QSORT(x)	((int (*)(const void *, const void *))(x))
int hny_compare_versions(const char **p1, const char **p2);

/* _HNY_H */
#endif
