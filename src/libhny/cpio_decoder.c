/* SPDX-License-Identifier: BSD-3-Clause */
#include "cpio_decoder.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cpio.h>
#include <errno.h>
#include <err.h>

#include "hny_prefix.h"

#define CPIO_HEADER_SIZE 76

#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct cpio_stream {
	const char *next;
	size_t available;
};

static enum cpio_decoder_status
cpio_decoder_string_reserve_for(struct cpio_decoder_string *string, size_t required) {
	enum cpio_decoder_status status = CPIO_DECODER_STATUS_OK;

	if (string->capacity < required) {
		string->buffer = realloc(string->buffer, sizeof (*string->buffer) * required);
		string->capacity = required;

		if (string->buffer == NULL) {
			status = CPIO_DECODER_STATUS_ERROR_MEMORY_EXHAUSTED;
		}
	}

	return status;
}

static enum cpio_decoder_status
cpio_decoder_decode_header_byte(struct cpio_decoder *cpio, unsigned char byte) {
	enum cpio_decoder_status status = CPIO_DECODER_STATUS_OK;

	byte -= '0';
	if (byte <= 7) {
		switch (cpio->offset) {
		case 0:
		case 2:
		case 4:
			if (byte != 0) {
				status = CPIO_DECODER_STATUS_ERROR_HEADER_INVALID_MAGIC;
			}
			break;
		case 1:
		case 3:
		case 5:
			if (byte != 7) {
				status = CPIO_DECODER_STATUS_ERROR_HEADER_INVALID_MAGIC;
			}
			break;
		case 6:
			cpio->stat.c_dev = 0;
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
			cpio->stat.c_dev = cpio->stat.c_dev * 8 + byte;
			break;
		case 12:
			cpio->stat.c_ino = 0;
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
			cpio->stat.c_ino = cpio->stat.c_ino * 8 + byte;
			break;
		case 18:
			cpio->stat.c_mode = 0;
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
			cpio->stat.c_mode = cpio->stat.c_mode * 8 + byte;
			break;
		case 24:
			cpio->stat.c_uid = 0;
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
			cpio->stat.c_uid = cpio->stat.c_uid * 8 + byte;
			break;
		case 30:
			cpio->stat.c_gid = 0;
		case 31:
		case 32:
		case 33:
		case 34:
		case 35:
			cpio->stat.c_gid = cpio->stat.c_gid * 8 + byte;
			break;
		case 36:
			cpio->stat.c_nlink = 0;
		case 37:
		case 38:
		case 39:
		case 40:
		case 41:
			cpio->stat.c_nlink = cpio->stat.c_nlink * 8 + byte;
			break;
		case 42:
			cpio->stat.c_rdev = 0;
		case 43:
		case 44:
		case 45:
		case 46:
		case 47:
			cpio->stat.c_rdev = cpio->stat.c_rdev * 8 + byte;
			break;
		case 48:
			cpio->stat.c_mtime = 0;
		case 49:
		case 50:
		case 51:
		case 52:
		case 53:
		case 54:
		case 55:
		case 56:
		case 57:
		case 58:
			cpio->stat.c_mtime = cpio->stat.c_mtime * 8 + byte;
			break;
		case 59:
			cpio->stat.c_namesize = 0;
		case 60:
		case 61:
		case 62:
		case 63:
		case 64:
			cpio->stat.c_namesize = cpio->stat.c_namesize * 8 + byte;
			break;
		case 65:
			cpio->stat.c_filesize = 0;
		case 66:
		case 67:
		case 68:
		case 69:
		case 70:
		case 71:
		case 72:
		case 73:
		case 74:
		case 75:
			cpio->stat.c_filesize = cpio->stat.c_filesize * 8 + byte;
			break;
		}
	} else {
		status = CPIO_DECODER_STATUS_ERROR_HEADER_INVALID_BYTE;
	}

	return status;
}

static enum cpio_decoder_status
cpio_decoder_decode_header(struct cpio_decoder *cpio, struct cpio_stream *stream) {
	const size_t soffset = cpio->offset; /* Start offset of decode sequence */
	size_t eoffset = soffset + MIN(CPIO_HEADER_SIZE - cpio->offset, stream->available); /* End offset of decode sequence */
	enum cpio_decoder_status status;

	while (cpio->offset != eoffset && (status = cpio_decoder_decode_header_byte(cpio, stream->next[cpio->offset - soffset]), status == CPIO_DECODER_STATUS_OK)) {
		cpio->offset++;
	}

	if (status == CPIO_DECODER_STATUS_OK && cpio->offset == CPIO_HEADER_SIZE) {
		if (cpio->stat.c_namesize != 0) {
			status = cpio_decoder_string_reserve_for(&cpio->filename, cpio->stat.c_namesize);
			if (status == CPIO_DECODER_STATUS_OK) {
				cpio->state = CPIO_DECODER_STATE_FILENAME;
				cpio->offset = 0;
			}
		} else {
			status = CPIO_DECODER_STATUS_ERROR_HEADER_INVALID_NAMESIZE;
		}
	}

	const size_t decoded = eoffset - soffset;

	stream->next += decoded;
	stream->available -= decoded;

	return status;
}

static int
cpio_decoder_normalize_path(char *path, size_t length) {
	enum {
		NORMALIZE_STATE_TRAILING_SLASH,
		NORMALIZE_STATE_NAME_DOT_DOT,
		NORMALIZE_STATE_NAME_DOT,
		NORMALIZE_STATE_NAME,
	} state = NORMALIZE_STATE_TRAILING_SLASH;
	unsigned int dst = 0, src = 0;

	if (path[length - 1] != '\0') {
		/* Expect at least to be null terminated */
		return -1;
	}

	while (path[src] != '\0') {
		switch (state) {
		case NORMALIZE_STATE_TRAILING_SLASH:
			switch (path[src]) {
			case '/':
				break;
			case '.':
				state = NORMALIZE_STATE_NAME_DOT;
				break;
			default:
				path[dst++] = path[src];
				state = NORMALIZE_STATE_NAME;
				break;
			}
			break;
		case NORMALIZE_STATE_NAME_DOT_DOT:
			if (path[src] == '/') {
				if (dst != 0) {
					dst -= 2;
					while (dst != 0 && path[dst - 1] != '/') {
						dst--;
					}
				}
				state = NORMALIZE_STATE_TRAILING_SLASH;
			} else {
				path[dst++] = '.';
				path[dst++] = '.';
				path[dst++] = path[src];
				state = NORMALIZE_STATE_NAME;
			}
			break;
		case NORMALIZE_STATE_NAME_DOT:
			switch (path[src]) {
			case '/':
				state = NORMALIZE_STATE_TRAILING_SLASH;
				break;
			case '.':
				state = NORMALIZE_STATE_NAME_DOT_DOT;
				break;
			default:
				path[dst++] = '.';
				path[dst++] = path[src];
				state = NORMALIZE_STATE_NAME;
				break;
			}
			break;
		case NORMALIZE_STATE_NAME:
			path[dst++] = path[src];
			if (path[src] == '/') {
				state = NORMALIZE_STATE_TRAILING_SLASH;
			}
			break;
		}
		src++;
	}

	if (state == NORMALIZE_STATE_NAME_DOT_DOT && dst != 0) {
		dst -= 2;
		while (dst != 0 && path[dst] != '/') {
			dst--;
		}
	}

	path[dst] = '\0';

	if (*path == '\0') {
		/* Empty, or resolved empty, is always invalid */
		return -1;
	}

	return 0;
}

static enum cpio_decoder_status
cpio_decoder_decode_filename(struct cpio_decoder *cpio, struct cpio_stream *stream) {
	const size_t copied = MIN(cpio->stat.c_namesize - cpio->offset, stream->available);
	enum cpio_decoder_status status = CPIO_DECODER_STATUS_OK;

	memcpy(cpio->filename.buffer + cpio->offset, stream->next, copied);
	cpio->offset += copied;
	stream->next += copied;
	stream->available -= copied;

	while (cpio->offset == cpio->stat.c_namesize) {
		static const char trailer[] = "TRAILER!!!";
		const char * const pathname = cpio->filename.buffer;

		if (cpio->stat.c_namesize == sizeof (trailer) && memcmp(trailer, pathname, sizeof (trailer)) == 0) {
			status = CPIO_DECODER_STATUS_END;
			cpio->state = CPIO_DECODER_STATE_END;
			break;
		}

		if (cpio_decoder_normalize_path(cpio->filename.buffer, cpio->stat.c_namesize) != 0) {
			status = CPIO_DECODER_STATUS_ERROR_FILENAME_INVALID;
			break;
		}
		/* Normalization is done in-place, thus pathname now points to a normalized path. */

		switch (cpio->stat.c_mode & 0770000) {
		case C_ISREG:
			cpio->fd = openat(cpio->dirfd, pathname, O_CREAT | O_WRONLY | O_EXCL, 0200);
			if (cpio->fd < 0) {
				status = CPIO_DECODER_STATUS_ERROR_CREAT;
				cpio->errcode = errno;
			}
			break;
		case C_ISLNK:
			if (cpio->stat.c_filesize != 0) {
				status = cpio_decoder_string_reserve_for(&cpio->sltarget, cpio->stat.c_filesize);
			} else {
				status = CPIO_DECODER_STATUS_ERROR_SYMLINK_TARGET_INVALID;
			}
			break;
		default:
			break;
		}

		if (status == CPIO_DECODER_STATUS_OK) {
			cpio->state = CPIO_DECODER_STATE_FILE;
			cpio->offset = 0;
		}
		break;
	}

	return status;
}

static enum cpio_decoder_status
cpio_decoder_decode_file_finish(struct cpio_decoder *cpio) {
	enum cpio_decoder_status status = CPIO_DECODER_STATUS_OK;
	const char * const pathname = cpio->filename.buffer;
	const mode_t type = cpio->stat.c_mode & 0770000;
	const mode_t perm = cpio->stat.c_mode & 07777;
	const mode_t mask = umask(0);
	uid_t owner;
	gid_t group;

	if (cpio->extractids) {
		owner = cpio->stat.c_uid;
		group = cpio->stat.c_gid;
	} else {
		owner = cpio->owner;
		group = cpio->group;
	}

	switch (type) {
	case C_ISDIR:
		if (mkdirat(cpio->dirfd, pathname, perm) != 0) {
			status = CPIO_DECODER_STATUS_ERROR_MKDIR;
			cpio->errcode = errno;
		}
		break;
	case C_ISFIFO:
		if (mkfifoat(cpio->dirfd, pathname, perm) != 0) {
			status = CPIO_DECODER_STATUS_ERROR_MKFIFO;
			cpio->errcode = errno;
		}
		break;
	case C_ISREG:
		if (fchmod(cpio->fd, perm) == 0) {
			if (fchown(cpio->fd, owner, group) != 0) {
				status = CPIO_DECODER_STATUS_ERROR_CHOWN;
				cpio->errcode = errno;
			}
		} else {
			status = CPIO_DECODER_STATUS_ERROR_CHMOD;
			cpio->errcode = errno;
		}
		close(cpio->fd);
		break;
	case C_ISBLK:
		/* fallthrough */
	case C_ISCHR:
		if (mknodat(cpio->dirfd, pathname, perm, cpio->stat.c_rdev) != 0) {
			status = CPIO_DECODER_STATUS_ERROR_MKNOD;
			cpio->errcode = errno;
		}
		break;
	case C_ISLNK:
		if (symlinkat(cpio->sltarget.buffer, cpio->dirfd, pathname) != 0) {
			status = CPIO_DECODER_STATUS_ERROR_SYMLINK;
			cpio->errcode = errno;
		}
		break;
	case C_ISSOCK:
	case C_ISCTG:
	default:
		break;
	}

	umask(mask);

	if (status == CPIO_DECODER_STATUS_OK) {
		switch (type) {
		case C_ISDIR:
		case C_ISFIFO:
		case C_ISBLK:
		case C_ISCHR:
		case C_ISLNK:
			if (fchownat(cpio->dirfd, pathname, owner, group, AT_SYMLINK_NOFOLLOW) != 0) {
				status = CPIO_DECODER_STATUS_ERROR_CHOWN;
				cpio->errcode = errno;
			}
			break;
		default:
			break;
		}
	}

	return status;
}

static enum cpio_decoder_status
cpio_decoder_decode_file(struct cpio_decoder *cpio, struct cpio_stream *stream) {
	const size_t copied = MIN(cpio->stat.c_filesize - cpio->offset, stream->available);
	enum cpio_decoder_status status = CPIO_DECODER_STATUS_OK;

	switch (cpio->stat.c_mode & 0770000) {
	case C_ISREG: {
		size_t written = 0;
		ssize_t writeval;

		while (written < copied && (writeval = write(cpio->fd, stream->next + written, copied - written), writeval >= 0)) {
			written += writeval;
		}

		if (writeval < 0) {
			status = CPIO_DECODER_STATUS_ERROR_WRITE;
			cpio->errcode = errno;
		}
	}	break;
	case C_ISLNK:
		memcpy(cpio->sltarget.buffer + cpio->offset, stream->next, copied);
		break;
	default:
		/* Discard */
		break;
	}

	if (status == CPIO_DECODER_STATUS_OK) {
		cpio->offset += copied;
		stream->next += copied;
		stream->available -= copied;

		if (cpio->offset == cpio->stat.c_filesize) {
			status = cpio_decoder_decode_file_finish(cpio);
			if (status == CPIO_DECODER_STATUS_OK) {
				cpio->state = CPIO_DECODER_STATE_HEADER;
				cpio->offset = 0;
			}
		}
	}

	return status;
}

int
cpio_decoder_init(struct cpio_decoder *cpio, int dirfd, const char *path) {
	int errcode;

	cpio->state = CPIO_DECODER_STATE_HEADER;

	cpio->offset = 0;
	cpio->errcode = 0;

	if (mkdirat(dirfd, path, 0777) != 0) {
		errcode = errno;
		goto cpio_decoder_init_err0;
	}

	cpio->dirfd = openat(dirfd, path, O_DIRECTORY | O_NOFOLLOW);
	if (cpio->dirfd < 0) {
		errcode = errno;
		goto cpio_decoder_init_err1;
	}

	{ /* Is root extracting? If so, then we apply uids and gids. */
		const uid_t euid = geteuid();

		if (euid != 0) {
			cpio->owner = euid;
			cpio->group = getegid();
			cpio->extractids = false;
		} else {
			cpio->extractids = true;
		}
	}

	cpio->filename.buffer = NULL;
	cpio->filename.capacity = 0;

	cpio->sltarget.buffer = NULL;
	cpio->sltarget.capacity = 0;

	return 0;
cpio_decoder_init_err1:
	unlinkat(dirfd, path, AT_REMOVEDIR);
cpio_decoder_init_err0:
	return errcode;
}

void
cpio_decoder_deinit(struct cpio_decoder *cpio) {

	if (cpio->state == CPIO_DECODER_STATE_FILE && (cpio->stat.c_mode & 0770000) == C_ISREG) {
		close(cpio->fd);
	}

	free(cpio->sltarget.buffer);
	free(cpio->filename.buffer);

	close(cpio->dirfd);
}

enum cpio_decoder_status
cpio_decoder_decode(struct cpio_decoder *cpio, const char *buffer, size_t size) {
	struct cpio_stream stream = { .next = buffer, .available = size };
	enum cpio_decoder_status status = CPIO_DECODER_STATUS_OK;

	while (status == CPIO_DECODER_STATUS_OK && stream.available != 0) {
		switch (cpio->state) {
		case CPIO_DECODER_STATE_HEADER:
			status = cpio_decoder_decode_header(cpio, &stream);
			break;
		case CPIO_DECODER_STATE_FILENAME:
			status = cpio_decoder_decode_filename(cpio, &stream);
			break;
		case CPIO_DECODER_STATE_FILE:
			status = cpio_decoder_decode_file(cpio, &stream);
			break;
		case CPIO_DECODER_STATE_END:
			status = CPIO_DECODER_STATUS_END;
			break;
		}
	}

	return status;
}
