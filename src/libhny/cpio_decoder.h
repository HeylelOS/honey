/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef CPIO_DECODER_H
#define CPIO_DECODER_H

#include <stdbool.h>
#include <sys/types.h>

enum cpio_decoder_status {
	CPIO_DECODER_STATUS_OK,
	CPIO_DECODER_STATUS_END,
	CPIO_DECODER_STATUS_ERROR_HEADER_INVALID_MAGIC,
	CPIO_DECODER_STATUS_ERROR_HEADER_INVALID_BYTE,
	CPIO_DECODER_STATUS_ERROR_HEADER_INVALID_NAMESIZE,
	CPIO_DECODER_STATUS_ERROR_MEMORY_EXHAUSTED,
	CPIO_DECODER_STATUS_ERROR_FILENAME_INVALID,
	CPIO_DECODER_STATUS_ERROR_SYMLINK_TARGET_INVALID,
	CPIO_DECODER_STATUS_ERROR_MKDIR,
	CPIO_DECODER_STATUS_ERROR_MKFIFO,
	CPIO_DECODER_STATUS_ERROR_CREAT,
	CPIO_DECODER_STATUS_ERROR_MKNOD,
	CPIO_DECODER_STATUS_ERROR_SYMLINK,
	CPIO_DECODER_STATUS_ERROR_CHOWN,
	CPIO_DECODER_STATUS_ERROR_CHMOD,
	CPIO_DECODER_STATUS_ERROR_WRITE,
};

struct cpio_decoder_stat {
	dev_t c_dev;
	ino_t c_ino;
	mode_t c_mode;
	uid_t c_uid;
	gid_t c_gid;
	nlink_t c_nlink;
	dev_t c_rdev;
	time_t c_mtime;
	size_t c_namesize;
	off_t c_filesize;
};

struct cpio_decoder_string {
	char *buffer;
	size_t capacity;
};

struct cpio_decoder {
	enum {
		CPIO_DECODER_STATE_HEADER,
		CPIO_DECODER_STATE_FILENAME,
		CPIO_DECODER_STATE_FILE,
		CPIO_DECODER_STATE_END
	} state; /**< State of the decode stream. */

	size_t offset; /**< Position in the current stream state. */
	int errcode; /**< Last reported error code. */

	int dirfd; /**< Root of file extractions. */

	uid_t owner; /**< User id we apply to files if we don't extract ids. */
	gid_t group; /**< Group id we apply to files if we don't extract ids. */
	bool extractids; /**< Whether we apply uid/gid from the stream. */

	struct cpio_decoder_stat stat; /**< Informations extracted from the header of a file. */

	struct cpio_decoder_string filename; /**< Path of the currently extracting file. */

	struct cpio_decoder_string sltarget; /**< Buffer to store target of symbolic link. */
	int fd; /**< File descriptor of the currently written file */
};

int
cpio_decoder_init(struct cpio_decoder *cpio, int dirfd, const char *path);

void
cpio_decoder_deinit(struct cpio_decoder *cpio);

enum cpio_decoder_status
cpio_decoder_decode(struct cpio_decoder *cpio, const char *buffer, size_t size);

/* CPIO_DECODER_H */
#endif
