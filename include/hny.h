/*
	hny.h
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#ifndef HNY_H
#define HNY_H

#ifdef __linux__
#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE
#endif
#include <sys/types.h>

/**
 * @mainpage Honey package manager
 * Honey is intended to be a unix-like OS package manager,
 * archive structure format for packages and http/https remote
 * package database guideline. It is distributed under BSD 3-Clause
 * license.
 *
 * It is meant to be composed of an ansi C library and
 * a command line utility for now, plus a guideline
 * for future providers implementations.
 *
 * It keeps in mind the desire to stay embeddable without
 * any compromise on its capabilities. However it isn't
 * meant to be used directly, you should build your
 * providers around it. The command line utility is meant for
 * shell script providers or advanced users.
 */

/**
 * Opaque data type to represent a Honey prefix
 * thread-safe through the library
 */
typedef struct hny *hny_t;

/**
 * enum type to represent errors, numeric values
 * are used as error codes for Honey packages scripts
 */
enum hny_error {
	HnyErrorNone = 0,
	HnyErrorInvalidArgs = 1,
	HnyErrorUnavailable = 2,
	HnyErrorNonExistant = 3,
	HnyErrorUnauthorized = 4
};

/**
 * Internal representation of a geist,
 * a reference geist has a NULL version,
 * a package has a name and a version.
 * Both must be valid values
 * @see hny_check_geister
 */
struct hny_geist {
	/**
	 * name of the geist
	 * @see hny_check_name
	 * */
	char *name;
	/**
	 * version of the geist
	 * @see hny_check_version
	 * */
	char *version;
};

/**
 * Listing conditions. HnyListPackages list
 * all packages, even unactive ones. HnyListActive
 * lists all referenced packages.
 * @see hny_list
 */
enum hny_listing {
	HnyListPackages,
	HnyListActive
};

/**
 * List of valid Honey package scripts,
 * see the package file format for further informations
 * @see hny_execute
 */
enum hny_action {
	HnyActionSetup,
	HnyActionClean,
	HnyActionReset,
	HnyActionCheck,
	HnyActionPurge
};

/********************
    PREFIX SECTION
*********************/

/**
 * Hook on a Honey prefix
 * @param prefix prefix directory
 * @return NULL on error. A valid Honey prefix otherwise, which MUST be closed with hny_destroy
 */
hny_t
hny_create(const char *prefix);

/**
 * Unhook a Honey prefix
 * @param hny Honey prefix to close
 */
void
hny_destroy(hny_t hny);

/********************
  OPERATIONS SECTION
*********************/

/**
 * Checks archive integrity
 * @param hny Honey prefix
 * @param file file to verify
 * @param eula pointer for returning a buffer to the end user license agreement, should be free()'d if no error
 * @param len pointer for returning eula buffer size
 * @return error code
 */
enum hny_error
hny_verify(hny_t hny,
	const char *file,
	char **eula,
	size_t *len);

/**
 * Deflates the package in the associated prefix
 * @param hny Honey prefix
 * @param file archive to deflate, the user should have accepted the eula
 * @param package the package installed, name and version provided
 * @return error code
 */
enum hny_error
hny_export(hny_t hny,
	const char *file,
	const struct hny_geist *package);

/**
 * Replaces a geist by a package
 * @param hny Honey prefix
 * @param geist the geist to replace
 * @param package the package portraying the geist
 * @return error code
 */
enum hny_error
hny_shift(hny_t hny,
	const char *geist,
	const struct hny_geist *package);

/**
 * Lists all packages following condition
 * @param hny Honey prefix
 * @param listing the listing condition
 * @param list pointer for returning the list, should be hny_free_geister()'d if no error
 * @param len pointer for returning list length
 * @return error code
 */
enum hny_error
hny_list(hny_t hny,
	enum hny_listing listing,
	struct hny_geist **list,
	size_t *len);

/**
 * Erases the geist/package from prefix, unlink if a geist, remove all files and
 * dereference everywhere possible if it's a package
 * @param hny Honey prefix
 * @param geist the geist or package to erase
 * @return error code
 */
enum hny_error
hny_erase(hny_t hny,
	const struct hny_geist *geist);

/**
 * Status follows the symlinks and/or file in the
 * prefix until it finds a problem or a package
 * directory, if it finds a broken symlink it will unlink it
 * @param hny Honey prefix
 * @param geist the geist or package to query
 * @param target pointer for returning the final target, should be hny_free_geister()'d if no error
 * @return error code
 */
enum hny_error
hny_status(hny_t hny,
	const struct hny_geist *geist,
	struct hny_geist **target);

/**
 * Executes the assiociated the given script associated to geist
 * the script will be chdir'd into the package prefix
 * TODO: Better definition of execution jail/conditions
 * @param hny Honey prefix
 * @param action script to execute
 * @param geist the geist or package for which the script shall be executed
 * @return error code
 */
enum hny_error
hny_execute(hny_t hny,
	enum hny_action action,
	const struct hny_geist *geist);

/********************
  UTILITIES SECTION
*********************/

/**
 * Allocates a structure for each name, or none if an error occured
 * @param names valid full-names of packages or geister
 * @param count count of names
 * @return NULL if an error occured. A list of count geister otherwise, which must be hny_free_geister()'d
 */
struct hny_geist *
hny_alloc_geister(const char **names,
	size_t count);

/**
 * Frees an hny_geist vector or unit allocated by the library
 * @param geister the vector to free
 * @param count the number of elements in the vector
 */
void
hny_free_geister(struct hny_geist *geister,
	size_t count);

/**
 * Checks if two geister are equals
 * @param g1 first geist
 * @param g2 second geist
 * @return HnyErrorNone if equals, something else else
 */
enum hny_error
hny_equals_geister(const struct hny_geist *g1,
	const struct hny_geist *g2);

/*@{*/
/**
 * Checks syntaxic integrity of packages/geister names
 * name is the prefix, a "pure" geist name, version (NULL is valid) is the
 * version suffix and package is the mix of both, packages
 * entry names (name + '-' + version). Check geister is to check
 * a geister instance
 */
enum hny_error
hny_check_name(const char *name);
enum hny_error
hny_check_version(const char *version);
enum hny_error
hny_check_package(const char *packagename);
enum hny_error
hny_check_geister(const struct hny_geist *geister,
	size_t n);
/*@}*/

/**
 * Macro to cast hny_compare_versions to qsort type
 * @see hny_compare_versions
 */
#define HNY_CMP_QSORT(x)	((int (*)(const void *, const void *))(x))
/**
 * Compare versions of geister
 * @param p1 first version
 * @param p2 second version
 * @return qsort valid comparison
 */
int
hny_compare_versions(const char **p1,
	const char **p2);

/* HNY_H */
#endif
