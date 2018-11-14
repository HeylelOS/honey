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
 * archive structure format for packages.
 * It is distributed under BSD 3-Clause license.
 *
 * It is meant to be composed of an ansi C library and
 * a command line utility.
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
	HnyErrorNone = 0,		/**< No error occured */
	HnyErrorInvalidArgs = 1,	/**< An argument is invalid */
	HnyErrorUnavailable = 2,	/**< The prefix is busy, or a resource is missing */
	HnyErrorNonExistant = 3,	/**< Something is missing, procedure dependant */
	HnyErrorUnauthorized = 4	/**< The user lacks privileges */
};


/**
 * Internal representation of a geist,
 * a reference geist has a NULL version,
 * a package has a name and a version.
 * Both must be valid values
 * @see hny_check_geister
 */
struct hny_geist {
	char *name;	/**< name of the geist @see hny_check_name */
	char *version;	/**< version of the geist @see hny_check_version */
};

/**
 * Listing conditions. #HnyListPackages lists
 * all packages, even unactive ones. #HnyListActive
 * lists all active packages.
 * @see hny_list
 */
enum hny_listing {
	HnyListPackages,	/**< Lists packages in the prefix (dirs) */
	HnyListActive		/**< Lists active geister in the prefix (symlinks) */
};

/**
 * List of valid Honey package scripts,
 * see the package file format for further informations
 * @see hny_execute
 */
enum hny_action {
	HnyActionSetup,	/**< Action which installs necessary files on the system */
	HnyActionClean,	/**< Action which removes setup files */
	HnyActionReset,	/**< Action reseting packages' configuration */
	HnyActionCheck,	/**< Action checking files from packages (not the one installed) */
	HnyActionPurge	/**< Action which removes every user-related data */
};

/********************
    PREFIX SECTION
*********************/

/**
 * Hook on a Honey prefix
 * @param prefix prefix directory
 * @return A valid Honey prefix, which MUST be closed with
 * hny_destroy() on success. NULL on error.
 */
hny_t
hny_create(const char *prefix);

/**
 * Unhook a Honey prefix
 * @param hny Honey prefix to close
 */
void
hny_destroy(hny_t hny);

#define HNY_BLOCK	1	/**< The prefix blocks until availability @see hny_locking */
#define HNY_NONBLOCK	0	/**< The prefix is non-blocking and may be unavailable @see hny_locking */

/**
 * Sets Honey prefix behavior on locking, default is #HNY_BLOCK
 * @param hny Honey prefix
 * @param block the behavior to set, either #HNY_BLOCK or #HNY_NONBLOCK
 */
void
hny_locking(hny_t hny,
	int block);

/********************
  OPERATIONS SECTION
*********************/

/**
 * Checks archive integrity
 * @param hny Honey prefix
 * @param file file to verify
 * @param eula pointer for returning a buffer to the end user license agreement,
 * should be free()'d if no error
 * @param len pointer for returning eula buffer size
 * @return #HnyErrorNone on success,
 * #HnyErrorInvalidArgs if file couldn't get opened,
 * #HnyErrorNonExistant if the structure is invalid
 * and #HnyErrorUnavailable if a problem
 * happened while extracting end user license agreement.
 */
enum hny_error
hny_verify(hny_t hny,
	const char *file,
	char **eula,
	size_t *len);

/**
 * Deflates the package in the associated prefix, if the
 * user running this function has effective user id 0,
 * owners will be extracted from the archive.
 * @param hny Honey prefix
 * @param file archive to deflate, MUST have been hny_verify()'d
 * and the user MUST have accepted the eula
 * @param package the package installed, name and version provided
 * @return #HnyErrorNone on success,
 * #HnyErrorUnavailable if prefix busy,
 * #HnyErrorInvalidArgs if an argument is invalid
 * (archive format invalid, etc...)
 * #HnyErrorNonExistant if one of the files couldn't
 * be extracted
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
 * @return #HnyErrorNone on success,
 * #HnyErrorUnavailable if prefix busy
 */
enum hny_error
hny_shift(hny_t hny,
	const char *geist,
	const struct hny_geist *package);

/**
 * Lists all packages following condition
 * @param hny Honey prefix
 * @param listing the listing condition
 * @param list pointer for returning the list,
 * should be hny_free_geister()'d if no error and not empty
 * @param len pointer for returning list length
 * @return #HnyErrorNone on success and a list of
 * len geister in list,
 * #HnyErrorUnavailable if prefix busy,
 * list and len untouched.
 */
enum hny_error
hny_list(hny_t hny,
	enum hny_listing listing,
	struct hny_geist **list,
	size_t *len);

/**
 * Erases the package from prefix, remove all files and
 * dereference everywhere possible. If its an active geist,
 * deactivates it.
 * @param hny Honey prefix
 * @param geist the geist to erase/deactivate
 * @return #HnyErrorNone on success,
 * #HnyErrorUnavailable if prefix busy
 * #HnyErrorNonExistant if a ressource doesn't exist
 * #HnyErrorInvalidArgs if a filename is too long or too much symlinks...
 * #HnyErrorUnauthorized if not authorized
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
 * @return #HnyErrorNone on success,
 * #HnyErrorUnavailable if prefix busy,
 * #HnyErrorInvalidArgs if geist isn't valid and
 * #HnyErrorNonExistant if the status couldn't be fetched
 */
enum hny_error
hny_status(hny_t hny,
	const struct hny_geist *geist,
	struct hny_geist **target);

/**
 * Executes the assiociated the given script associated to geist
 * the script will be chdir'd into the package prefix
 * @param hny Honey prefix
 * @param action script to execute
 * @param geist the geist or package for which the script shall be executed
 * @return #HnyErrorNone on success,
 * #HnyErrorUnavailable if prefix busy
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
 * @return #HnyErrorNone if equals, something else else
 */
enum hny_error
hny_equals_geister(const struct hny_geist *g1,
	const struct hny_geist *g2);

/**
 * @defgroup hny_check Checking strings integrity
 * Checks syntaxic integrity of packages/geister names
 * hny_checks_name() checks the prefix, a "pure" geist name,
 * hny_check_version() checks the version suffix and
 * hny_check_package() checks the mix of both, packages
 * entry names (name + '-' + version).
 * hny_check_geister() checks geister instance(s)
 * @{
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
/** @} */

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
