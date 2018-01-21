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

#define _BSD_SOURCE
/* _BSD_SOURCE is for internal build */
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
	Frees all values of the library, release the semaphore and disconnects all connections
	from providers
**/
void hny_disconnect();

/********************
  OPERATIONS SECTION
*********************/

/**
	Tries to install file package in the user package
	directory
	return HNY_OK on success,
		HNY_NONEXISTANT if file doesn't exist
		HNY_UNAVAILABLE if a package of the same name already exists
		HNY_UNAUTHORIZED if a permission stops the install
**/
enum hny_error hny_install(const char *file);

/**
	Shift replaces the geist named geist
	by the package or geist described in packages
**/
enum hny_error hny_shift(const char *geist, const struct hny_geist *package);

/**
**/
enum hny_listing {
	HnyListAll,
	HnyListActive,
	HnyListLinks
};
struct hny_geist *hny_list(enum hny_listing listing, size_t *listed);

/**
**/
enum hny_removal {
	HnyRemoveTotal,
	HnyRemoveData,
	HnyRemoveLinks
};
enum hny_error hny_remove(enum hny_removal removal, const struct hny_geist *geist);

/**
**/
enum hny_error hny_status(struct hny_geist *target, const struct hny_geist *geist);

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
