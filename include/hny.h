/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef HNY_H
#define HNY_H

#include <sys/types.h>

/**
 * @mainpage honey package manager
 * honey is intended to be a unix-like OS package manager,
 * archive structure format for packages.
 * It is distributed under BSD 3-Clause license.
 *
 * It is meant to be composed of a C library and
 * a command line utility.
 *
 * It keeps in mind the desire to stay embeddable without
 * any compromise on its capabilities. However it isn't
 * meant to be used directly, you should build your
 * providers around it. The command line utility is meant for
 * shell script providers or advanced users.
 */

/********************
    PREFIX SECTION
*********************/

/**
 * Opaque data type to represent a honey prefix
 */
struct hny;

/**
 * Values used for honey prefix' flags configuration
 * @see hny_open @see hny_flags
 */
enum hny_flags {
	HNY_FLAGS_NONE  = 0,     /**< No flags */
	HNY_FLAGS_BLOCK = 1 << 0 /**< The prefix blocks until availability */
};

/**
 * Hook on a honey prefix
 * @param path prefix directory absolute path
 * @param flags prefix flags, see ::hny_flags
 * @param hnyp Pointer to return honey prefix on success, which may be closed
   with hny_close(). Untouched if an error happens.
 * @return 0 on success, an error code else.
 */
int
hny_open(struct hny **hnyp, const char *path, int flags);

/**
 * Unhook a honey prefix.
 * @param hny Previously hny_open()'d valid honey prefix to close
 */
void
hny_close(struct hny *hny);

/**
 * Sets honey prefix parameters on behaviour
 * @param hny honey prefix
 * @param flags behaviour, see ::hny_flags
 * @return Previous flags
 */
int
hny_flags(struct hny *hny, int flags);

/**
 * Get absolute path of the honey prefix
 * @param hny honey prefix
 * @return honey prefix absolute path, must not be free()'d
 */
const char *
hny_path(struct hny *hny);

/**
 * Locks a prefix, must be used if you create/destroy/modify an entry in the directory
 * The lock uses an exclusive flock(2), all implications concerning lifetimes
 * and persistence/duplications through fork(2) and execve(2) are implied.
 * @param hny prefix to lock
 * @return 0 on success, an error code else.
 */
int
hny_lock(struct hny *hny);

/**
 * Unlocks a prefix.
 * @param hny prefix to unlock
 */
void
hny_unlock(struct hny *hny);

/********************
  OPERATIONS SECTION
*********************/

/**
 * Macro shortcut to determine if a status is an error.
 */
#define HNY_EXTRACTION_STATUS_IS_ERROR(s) ((s) > HNY_EXTRACTION_STATUS_END)

/**
 * Macro shortcut to determine if a status is an error related to xz.
 */
#define HNY_EXTRACTION_STATUS_IS_ERROR_XZ(s) ((s) >= HNY_EXTRACTION_STATUS_ERROR_XZ_HEADER_INVALID_MAGIC && (s) <= HNY_EXTRACTION_STATUS_ERROR_XZ_FOOTER_INVALID_CRC32)

/**
 * Macro shortcut to determine if a status is an error related to cpio.
 */
#define HNY_EXTRACTION_STATUS_IS_ERROR_CPIO(s) ((s) >= HNY_EXTRACTION_STATUS_ERROR_CPIO_HEADER_INVALID_MAGIC && (s) <= HNY_EXTRACTION_STATUS_ERROR_CPIO_WRITE)

/**
 * Macro shortcut to determine if a status is an error related to cpio and a system interface.
 */
#define HNY_EXTRACTION_STATUS_IS_ERROR_CPIO_SYSTEM(s) ((s) >= HNY_EXTRACTION_STATUS_ERROR_CPIO_MKDIR && (s) <= HNY_EXTRACTION_STATUS_ERROR_CPIO_WRITE)

/**
 * Opaque data type to represent a package extraction
 */
struct hny_extraction;

/**
 * Progress status of an extraction
 * @see hny_extraction_extract
 */
enum hny_extraction_status {
	HNY_EXTRACTION_STATUS_OK,
	HNY_EXTRACTION_STATUS_END,

	HNY_EXTRACTION_STATUS_ERROR_UNFINISHED_CPIO,

	HNY_EXTRACTION_STATUS_ERROR_XZ_HEADER_INVALID_MAGIC,
	HNY_EXTRACTION_STATUS_ERROR_XZ_HEADER_UNSUPPORTED_CHECK,
	HNY_EXTRACTION_STATUS_ERROR_XZ_HEADER_INVALID_CRC32,
	HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_UNSUPPORTED_FLAG,
	HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_UNSUPPORTED_FILTER_FLAG,
	HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_UNSUPPORTED_PROPERTIES_SIZE,
	HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_UNSUPPORTED_PROPERTY,
	HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_INVALID_COMPRESSED_SIZE,
	HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_INVALID_UNCOMPRESSED_SIZE,
	HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_INVALID_PADDING,
	HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_INVALID_CRC32,
	HNY_EXTRACTION_STATUS_ERROR_XZ_LZMA2_UNABLE_DICTIONARY_RESET,
	HNY_EXTRACTION_STATUS_ERROR_XZ_LZMA2_MEMORY_EXHAUSTED,
	HNY_EXTRACTION_STATUS_ERROR_XZ_LZMA2_MEMORY_LIMIT,
	HNY_EXTRACTION_STATUS_ERROR_XZ_LZMA2_INVALID_DICTIONARY_BITS,
	HNY_EXTRACTION_STATUS_ERROR_XZ_LZMA2_CORRUPTED_DATA,
	HNY_EXTRACTION_STATUS_ERROR_XZ_INDEX_INVALID_RECORDS_COUNT,
	HNY_EXTRACTION_STATUS_ERROR_XZ_INDEX_INVALID,
	HNY_EXTRACTION_STATUS_ERROR_XZ_INDEX_INVALID_PADDING,
	HNY_EXTRACTION_STATUS_ERROR_XZ_INDEX_INVALID_CRC32,
	HNY_EXTRACTION_STATUS_ERROR_XZ_FOOTER_INVALID_STREAM_FLAGS,
	HNY_EXTRACTION_STATUS_ERROR_XZ_FOOTER_INVALID_BACKWARD_SIZE,
	HNY_EXTRACTION_STATUS_ERROR_XZ_FOOTER_INVALID_MAGIC,
	HNY_EXTRACTION_STATUS_ERROR_XZ_FOOTER_INVALID_CRC32,

	HNY_EXTRACTION_STATUS_ERROR_CPIO_HEADER_INVALID_MAGIC,
	HNY_EXTRACTION_STATUS_ERROR_CPIO_HEADER_INVALID_BYTE,
	HNY_EXTRACTION_STATUS_ERROR_CPIO_HEADER_INVALID_NAMESIZE,
	HNY_EXTRACTION_STATUS_ERROR_CPIO_MEMORY_EXHAUSTED,
	HNY_EXTRACTION_STATUS_ERROR_CPIO_FILENAME_INVALID,
	HNY_EXTRACTION_STATUS_ERROR_CPIO_SYMLINK_TARGET_INVALID,
	HNY_EXTRACTION_STATUS_ERROR_CPIO_MKDIR,
	HNY_EXTRACTION_STATUS_ERROR_CPIO_MKFIFO,
	HNY_EXTRACTION_STATUS_ERROR_CPIO_CREAT,
	HNY_EXTRACTION_STATUS_ERROR_CPIO_MKNOD,
	HNY_EXTRACTION_STATUS_ERROR_CPIO_SYMLINK,
	HNY_EXTRACTION_STATUS_ERROR_CPIO_CHOWN,
	HNY_EXTRACTION_STATUS_ERROR_CPIO_CHMOD,
	HNY_EXTRACTION_STATUS_ERROR_CPIO_WRITE,
};

/**
 * Create an extraction handler.
 * @param extractionp pointer to the handler.
 * @param hny prefix of the package.
 * @param package name of the package.
 * @return 0 on success, an error code else.
 */
int
hny_extraction_create(struct hny_extraction **extractionp, struct hny *hny, const char *package);

/**
 * Create an extraction handler.
 * @param extractionp pointer to the handler.
 * @param hny prefix of the package.
 * @param package name of the package.
 * @param size size of the intermediate buffer between xz and cpio steps.
 * @param dictionarymax maximum size of the lzma2 dictionary.
 * @return 0 on success, an error code else.
 */
int
hny_extraction_create2(struct hny_extraction **extractionp, struct hny *hny, const char *package, size_t size, size_t dictionarymax);

/**
 * Destroys a previously hny_extraction_create()'d extraction handler
 * @param extraction Handler to destroy
 */
void
hny_extraction_destroy(struct hny_extraction *extraction);

/**
 * Extracts an archive from a byte stream
 * @param extraction extraction handler
 * @param buffer bytes to extract
 * @param size size of @p buffer
 * @return #HNY_EXTRACTION_STATUS_OK if extracting, #HNY_EXTRACTION_STATUS_END
 * when successfull extraction is done. Else the step in which an error occurred.
 */
enum hny_extraction_status
hny_extraction_extract(struct hny_extraction *extraction, const char *buffer, size_t size);

/**
 * Get previous error's code, and reset to 0.
 * @param extraction extraction handler
 * @return If a previous call to @ref hny_extraction_extract returned a syscall error, associated error code, 0 else.
 */
int
hny_extraction_errcode(struct hny_extraction *extraction);

/**
 * Replaces the target of a geist.
 * @param hny honey prefix
 * @param geist geist to modify, must be of type #HNY_TYPE_GEIST
 * @param target the new target of the geist
 * @return 0 on success, an error code else.
 */
int
hny_shift(struct hny *hny, const char *geist, const char *target);

/**
 * Depending on type of @p entry, it will unlink a #HNY_TYPE_GEIST
 * and recursively remove a #HNY_TYPE_PACKAGE.
 * @param hny honey prefix
 * @param entry entry to remove
 * @return 0 on success, an error code else.
 */
int
hny_remove(struct hny *hny, const char *entry);

#define HNY_SPAWN_STATUS_ERROR 127

/**
 * Executes the given file associated with @p entry
 * the process will execute into the package prefix.
 * The argument list only contains the basename of @p path.
 * The @p hny absolute path is an environment variable named HNY_PREFIX.
 * The @p entry is an environment variable named HNY_ENTRY.
 * Note the execution can fail even if the return value is 0. In this case
 * the process returns with an exit code #HNY_SPAWN_STATUS_ERROR.
 * @param hny honey prefix
 * @param entry the geist or package for which the script shall be executed
 * @param path path relative to the package directory of the executable
 * @param pid PID of the spawned process
 * @return 0 on success, an error code else.
 */
int
hny_spawn(struct hny *hny, const char *entry, const char *path, pid_t *pid);

/********************
  UTILITIES SECTION
*********************/

/**
 * Type of an entry
 * @see hny_type_of
 */
enum hny_type {
	HNY_TYPE_NONE,    /**< The entry is invalid */
	HNY_TYPE_PACKAGE, /**< The entry references a package */
	HNY_TYPE_GEIST    /**< The entry references a geist */
};

/**
 * Returns the type of an entry
 * @param entry entry
 * @return #HNY_TYPE_NONE on error
 */
enum hny_type
hny_type_of(const char *entry);

/* HNY_H */
#endif
