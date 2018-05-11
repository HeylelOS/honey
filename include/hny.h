/*
	hny.h
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#ifndef HNY_H
#define HNY_H

#include <sys/types.h>

enum hny_error {
	HnyErrorNone = 0,
	HnyErrorInvalidArgs = 1,
	HnyErrorUnavailable = 2,
	HnyErrorNonExistant = 3,
	HnyErrorUnauthorized = 4
};

struct hny_geist {
	char *name;
	char *version;
};

/********************
    PREFIX SECTION
*********************/

typedef struct hny *hny_t;

hny_t
hny_create(const char *prefix);

void
hny_destroy(hny_t hny);

/********************
  OPERATIONS SECTION
*********************/

/**
	Checks archive integrity
	returns:
	error if there was an error,
	eula and len untouched else
	eula and its size if found,
	eula should be freed
**/
enum hny_error
hny_verify(hny_t hny,
	const char *file,
	const char **eula,
	size_t *len);

/**
	Tries to deflate file package
	in the hny prefix
**/
enum hny_error
hny_export(hny_t hny,
	const char *file,
	const struct hny_geist *package);

/**
	Shift replaces the geist named geist
	by the package or geist described in package
**/
enum hny_error
hny_shift(hny_t hny,
	const char *geist,
	const struct hny_geist *package);

/**
	Lists all geister which follow the listing
	condition
**/
enum hny_listing {
	HnyListPackages,
	HnyListActive
};

enum hny_error
hny_list(hny_t hny,
	enum hny_listing listing,
	struct hny_geist **list,
	size_t *len);

/**
	Erases the geist of prefix, which
	deactivates it if its a symlink,
	and erases all files if its a directory
**/
enum hny_error
hny_erase(hny_t hny,
	const struct hny_geist *geist);

/**
	Status follows the symlinks and/or file in the
	prefix until it finds a problem or a package
	directory, if it finds a broken symlink it will unlink
	it, if an error occurs, it returns error, otherwise
	a pointer to the target, which can be freed with
	hny_free_geister
**/
enum hny_error
hny_status(hny_t hny,
	const struct hny_geist *geist,
	struct hny_geist **target);

/**
**/
enum hny_action {
	HnyActionSetup,
	HnyActionClean,
	HnyActionReset,
	HnyActionCheck,
	HnyActionPurge
};

enum hny_error
hny_execute(hny_t hny,
	enum hny_action action,
	const struct hny_geist *geist);

/********************
  UTILITIES SECTION
*********************/

/**
	Alloc geister for all names, or none
	if one isn't valid
**/
struct hny_geist *
hny_alloc_geister(const char **names,
	size_t count);

/**
	Free all geister values (names and optionnaly version)
	then frees the vector
**/
void
hny_free_geister(struct hny_geist *geister,
	size_t count);

/**
	Checks if two geister are equals
	returns 0 if equals, -1 else
**/
int
hny_equals_geister(const struct hny_geist *g1,
	const struct hny_geist *g2);

enum hny_error
hny_check_name(const char *name);
enum hny_error
hny_check_version(const char *version);
enum hny_error
hny_check_package(const char *packagename);
enum hny_error
hny_check_geister(const struct hny_geist *geister,
	size_t n);

/**
	Compare two versions together, qsort compatible
**/
#define HNY_CMP_QSORT(x)	((int (*)(const void *, const void *))(x))
int
hny_compare_versions(const char **p1,
	const char **p2);

/* HNY_H */
#endif
