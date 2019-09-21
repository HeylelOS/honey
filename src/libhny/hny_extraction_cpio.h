/*
	hny_extraction_cpio.h
	Copyright (c) 2018-2019, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE
*/
#ifndef HNY_EXTRACTION_CPIO_H
#define HNY_EXTRACTION_CPIO_H

#include <sys/types.h>

enum hny_extraction_cpio_status {
	HNY_EXTRACTION_CPIO_STATUS_OK,
	HNY_EXTRACTION_CPIO_STATUS_END,
	HNY_EXTRACTION_CPIO_STATUS_ERROR_HEADER_INVALID_MAGIC,
	HNY_EXTRACTION_CPIO_STATUS_ERROR_HEADER_INVALID_BYTE,
	HNY_EXTRACTION_CPIO_STATUS_ERROR_HEADER_INVALID_NAMESIZE,
	HNY_EXTRACTION_CPIO_STATUS_ERROR_FILENAME_MEMORY_EXHAUSTED,
	HNY_EXTRACTION_CPIO_STATUS_ERROR_FILENAME_IS_EMPTY,
	HNY_EXTRACTION_CPIO_STATUS_ERROR_FILENAME_HAS_DOT_DOT,
	HNY_EXTRACTION_CPIO_STATUS_ERROR_MKDIR,
	HNY_EXTRACTION_CPIO_STATUS_ERROR_MKFIFO,
	HNY_EXTRACTION_CPIO_STATUS_ERROR_CREAT,
	HNY_EXTRACTION_CPIO_STATUS_ERROR_MKNOD,
	HNY_EXTRACTION_CPIO_STATUS_ERROR_SYMLINK,
	HNY_EXTRACTION_CPIO_STATUS_ERROR_CHOWN,
	HNY_EXTRACTION_CPIO_STATUS_ERROR_CHMOD,
	HNY_EXTRACTION_CPIO_STATUS_ERROR_WRITE
};

struct hny_extraction_cpio_stat {
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

struct hny_extraction_cpio {
	int dirfd;
	uid_t owner;
	gid_t group;
	unsigned extractids : 1;

	enum {
		HNY_EXTRACTION_CPIO_HEADER,
		HNY_EXTRACTION_CPIO_FILENAME,
		HNY_EXTRACTION_CPIO_FILE,
		HNY_EXTRACTION_CPIO_END
	} state;

	size_t offset;

	struct hny_extraction_cpio_stat stat;

	char *filename;
	size_t capacity;

	int fd;
	char *link;
	size_t linkcapacity;
};

int
hny_extraction_cpio_init(struct hny_extraction_cpio *cpio, int dirfd, const char *path);

void
hny_extraction_cpio_deinit(struct hny_extraction_cpio *cpio);

enum hny_extraction_cpio_status
hny_extraction_cpio_decode(struct hny_extraction_cpio *cpio,
	const char *buffer, size_t size, int *errcode);

/* HNY_EXTRACTION_CPIO_H */
#endif
