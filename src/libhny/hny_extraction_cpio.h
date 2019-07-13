#ifndef HNY_EXTRACTION_CPIO_H
#define HNY_EXTRACTION_CPIO_H

#include <sys/types.h>

enum hny_extraction_cpio_status {
	HNY_EXTRACTION_CPIO_STATUS_OK,
	HNY_EXTRACTION_CPIO_STATUS_END,
	HNY_EXTRACTION_CPIO_STATUS_ERROR
};

struct hny_extraction_cpio_stat {
	dev_t cs_dev;
	ino_t cs_ino;
	mode_t cs_mode;
	uid_t cs_uid;
	gid_t cs_gid;
	nlink_t cs_nlink;
	dev_t cs_rdev;
	time_t cs_mtime;
	size_t cs_namesize;
	off_t cs_filesize;
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
