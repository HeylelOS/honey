/*
	hny.h
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE
*/
#ifndef HNY_H
#define HNY_H

/**
 * @mainpage honey package manager
 * honey is intended to be a unix-like OS package manager,
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
 * Opaque data type to represent a honey prefix
 * thread-safe through the library
 */
typedef struct hny hny_t;

/**
 * Honey's equivalent of size_t, avoid include of <sys/types.h>
 */
typedef unsigned long hny_size_t;

/**
 * enum type to represent errors, numeric values
 * are used as error codes for honey packages scripts
 */
enum hny_error {
	HNY_ERROR_NONE = 0,         /**< No error occured */
	HNY_ERROR_INVALID_ARGS = 1, /**< An argument is invalid */
	HNY_ERROR_UNAVAILABLE = 2,  /**< The prefix is busy, or a resource is missing */
	HNY_ERROR_MISSING = 3,      /**< Something is missing, procedure dependant */
	HNY_ERROR_UNAUTHORIZED = 4  /**< The user lacks privileges */
};

/**
 * Values used for honey prefix' flags configuration
 * @see hny_open @see hny_flags
 */
enum hny_flags {
	HNY_FLAGS_NONE = 0, /**< No flags */
	HNY_FLAGS_BLOCK = 1 /**< The prefix blocks until availability */
};

/**
 * Listing conditions. #HNY_LIST_PACKAGES lists
 * all packages, even unactive ones. #HNY_LIST_ACTIVE
 * lists all active packages.
 * @see hny_list
 */
enum hny_listing {
	HNY_LIST_PACKAGES, /**< Lists packages in the prefix (dirs) */
	HNY_LIST_ACTIVE    /**< Lists active geister in the prefix (symlinks) */
};

/**
 * List of valid honey package scripts,
 * see the package file format for further informations
 * @see hny_execute
 */
enum hny_action {
	HNY_ACTION_SETUP, /**< Action which installs necessary files on the system */
	HNY_ACTION_CLEAN, /**< Action which removes setup files */
	HNY_ACTION_RESET, /**< Action reseting packages' configuration */
	HNY_ACTION_CHECK, /**< Action checking files from packages (not the one installed) */
	HNY_ACTION_PURGE  /**< Action which removes every user-related data */
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

/********************
    PREFIX SECTION
*********************/

/**
 * Hook on a honey prefix
 * @param path prefix directory absolute path
 * @param flags prefix flags, see ::hny_flags
 * @param hnyp Pointer to return honey prefix on success, which MUST be closed with
 * hny_close(). Untouched if an error happens.
 * @return #HNY_ERROR_NONE on success, #HNY_ERROR_INVALID_ARGS if prefix is not a directory,
 * #HNY_ERROR_UNAVAILABLE if the operation couldn't be performed,
 * #HNY_ERROR_MISSING if prefix couldn't be found,
 * #HNY_ERROR_UNAUTHORIZED if you don't have permissions to access the prefix.
 */
enum hny_error
hny_open(const char *path,
	int flags,
	hny_t **hnyp);

/**
 * Unhook a honey prefix.
 * @param hny Previously hny_open()'d valid honey prefix to close
 */
void
hny_close(hny_t *hny);

/**
 * Sets honey prefix parameters on behaviour
 * @param hny honey prefix
 * @param flags behaviour, see ::hny_flags
 * @return Previous flags
 */
int
hny_flags(hny_t *hny,
	int flags);

/********************
  OPERATIONS SECTION
*********************/

/**
 * Checks archive integrity, this special operation doesn't need a prefix.
 * @param file file to verify
 * @param eula pointer for returning a buffer to the end user license agreement,
 * should be free()'d if no error
 * @param len pointer for returning eula buffer size
 * @return #HNY_ERROR_NONE on success,
 * #HNY_ERROR_INVALID_ARGS if file couldn't get opened,
 * #HNY_ERROR_MISSING if the structure is invalid
 * and #HNY_ERROR_UNAVAILABLE if a problem
 * happened while extracting end user license agreement.
 */
enum hny_error
hny_verify(const char *file,
	char **eula,
	hny_size_t *len);

/**
 * Deflates the package in the associated prefix, if the
 * process running this function has effective user id 0,
 * owners will be extracted from the archive.
 * @param hny honey prefix
 * @param file archive to deflate, MUST have been hny_verify()'d
 * and the user MUST have accepted the eula
 * @param package the package installed, name and version provided
 * @return #HNY_ERROR_NONE on success,
 * #HNY_ERROR_UNAVAILABLE if prefix busy,
 * #HNY_ERROR_INVALID_ARGS if an argument is invalid
 * (archive format invalid, etc...)
 * #HNY_ERROR_MISSING if one of the files couldn't
 * be extracted
 */
enum hny_error
hny_export(hny_t *hny,
	const char *file,
	const struct hny_geist *package);

/**
 * Replaces the target of a geist
 * @param hny honey prefix
 * @param geist the geist to modify
 * @param dest the new target of the geist
 * @return #HNY_ERROR_NONE on success,
 * #HNY_ERROR_UNAVAILABLE if prefix busy
 */
enum hny_error
hny_shift(hny_t *hny,
	const char *geist,
	const struct hny_geist *dest);

/**
 * Lists all packages following condition
 * @param hny honey prefix
 * @param listing the listing condition
 * @param list pointer for returning the list,
 * should be hny_free_geister()'d if no error and not empty
 * @param len pointer for returning list length
 * @return #HNY_ERROR_NONE on success and a list of
 * len geister in list,
 * #HNY_ERROR_UNAVAILABLE if prefix busy,
 * list and len untouched.
 */
enum hny_error
hny_list(hny_t *hny,
	enum hny_listing listing,
	struct hny_geist **list,
	hny_size_t *len);

/**
 * Erases the package from prefix, remove all files.
 * If its an active geist, deactivates it.
 * Note: Doesn't dereference a package's references.
 * @param hny honey prefix
 * @param geist the geist to erase/deactivate
 * @return #HNY_ERROR_NONE on success,
 * #HNY_ERROR_UNAVAILABLE if prefix busy
 * #HNY_ERROR_MISSING if a ressource doesn't exist
 * #HNY_ERROR_INVALID_ARGS if a filename is too long or too much symlinks...
 * #HNY_ERROR_UNAUTHORIZED if not authorized
 */
enum hny_error
hny_erase(hny_t *hny,
	const struct hny_geist *geist);

/**
 * Status follows the symlinks and/or file in the
 * prefix until it finds a problem or a directory.
 * @param hny honey prefix
 * @param geist the geist or package to query
 * @param target pointer for returning the final target, should be hny_free_geister()'d if no error
 * @return #HNY_ERROR_NONE on success,
 * #HNY_ERROR_UNAVAILABLE if prefix busy,
 * #HNY_ERROR_INVALID_ARGS if geist isn't valid and
 * #HNY_ERROR_MISSING if the status couldn't be fetched
 */
enum hny_error
hny_status(hny_t *hny,
	const struct hny_geist *geist,
	struct hny_geist **target);

/**
 * Executes the given script associated to geist
 * the script will execute into the package prefix.
 * Environnements variable for the honey prefix absolute path as HNY_PREFIX.
 * and each error code will be available for the executed process.
 * @param hny honey prefix
 * @param action script to execute
 * @param geist the geist or package for which the script shall be executed
 * @return #HNY_ERROR_NONE on success,
 * #HNY_ERROR_UNAVAILABLE if prefix busy
 */
enum hny_error
hny_execute(hny_t *hny,
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
	hny_size_t count);

/**
 * Frees an hny_geist vector or unit allocated by the library
 * @param geister the vector to free
 * @param count the number of elements in the vector
 */
void
hny_free_geister(struct hny_geist *geister,
	hny_size_t count);

/**
 * Checks if two geister are equals
 * @param g1 first geist
 * @param g2 second geist
 * @return #HNY_ERROR_NONE if equals, something else else
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
	hny_size_t n);
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
