#include "hny_extraction_cpio.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cpio.h>
#include <errno.h>
#include <err.h>

#define HNY_EXTRACTION_CPIO_HEADER_SIZE 76
#define HNY_EXTRACTION_CPIO_FILENAME_DEFAULT_CAPACITY 32

static int
hny_extraction_cpio_header_serialize_at(struct hny_extraction_cpio_stat *cpiostatp,
	unsigned char byte, size_t position);

static int
hny_extraction_cpio_open(struct hny_extraction_cpio *cpio, int *errcode);

static int
hny_extraction_cpio_close(struct hny_extraction_cpio *cpio, mode_t filetype, int *errcode);

int
hny_extraction_cpio_init(struct hny_extraction_cpio *cpio, int dirfd, const char *path) {
	int errcode;

	cpio->capacity = HNY_EXTRACTION_CPIO_FILENAME_DEFAULT_CAPACITY;
	if((cpio->filename = malloc(cpio->capacity)) == NULL) {
		errcode = errno;
		goto hny_extraction_cpio_init_err0;
	}

	if(mkdirat(dirfd, path, 0777) == -1
		|| (cpio->dirfd = openat(dirfd, path, O_DIRECTORY | O_NOFOLLOW)) == -1) {
		errcode = errno;
		goto hny_extraction_cpio_init_err1;
	}

	if((cpio->owner = geteuid()) != 0) {
		cpio->extractids = 0;
		cpio->group = getegid();
	} else {
		cpio->extractids = 1;
	}

	cpio->state = HNY_EXTRACTION_CPIO_HEADER;
	cpio->offset = 0;
	cpio->fd = -1;
	cpio->link = NULL;
	cpio->linkcapacity = 0;

	return 0;
hny_extraction_cpio_init_err1:
	free(cpio->filename);
hny_extraction_cpio_init_err0:
	return errcode;
}

void
hny_extraction_cpio_deinit(struct hny_extraction_cpio *cpio) {

	if(cpio->fd != -1) {
		close(cpio->fd);
	}

	free(cpio->link);
	free(cpio->filename);
	close(cpio->dirfd);
}

enum hny_extraction_cpio_status
hny_extraction_cpio_decode(struct hny_extraction_cpio *cpio,
	const char *buffer, size_t size, int *errcode) {
	const char * const bufferend = buffer + size;

	while(buffer != bufferend) {
		switch(cpio->state) {
		case HNY_EXTRACTION_CPIO_HEADER: {
			const char *headerend = buffer + HNY_EXTRACTION_CPIO_HEADER_SIZE - cpio->offset;
			if(bufferend < headerend) {
				headerend = bufferend;
			}

			int valid;
			while(buffer != headerend
				&& (valid = hny_extraction_cpio_header_serialize_at(&cpio->stat,
					*buffer, cpio->offset)) == 0) {
				buffer++;
				cpio->offset++;
			}

			if(valid == 0) {
				if(cpio->offset == HNY_EXTRACTION_CPIO_HEADER_SIZE) {
					if(cpio->stat.cs_namesize != 0) {
						cpio->state = HNY_EXTRACTION_CPIO_FILENAME;
						cpio->offset = 0;
						break;
					}
				} else {
					break;
				}
			}

			*errcode = EINVAL;
			return HNY_EXTRACTION_CPIO_STATUS_ERROR;
		}
		case HNY_EXTRACTION_CPIO_FILENAME: {
			const char *filenameend = buffer + cpio->stat.cs_namesize - cpio->offset;

			while(cpio->capacity < cpio->stat.cs_namesize) {
				char *newfilename = realloc(cpio->filename, cpio->capacity * 2);

				if(newfilename != NULL) {
					cpio->filename = newfilename;
					cpio->capacity *= 2;
				} else {
					*errcode = ENOMEM;
					return HNY_EXTRACTION_CPIO_STATUS_ERROR;
				}
			}

			if(bufferend < filenameend) {
				filenameend = bufferend;
			}

			size_t copied = filenameend - buffer;
			memcpy(cpio->filename + cpio->offset, buffer, copied);
			cpio->offset += copied;
			if(cpio->offset == cpio->stat.cs_namesize) {
				cpio->filename[cpio->stat.cs_namesize - 1] = '\0';
				if(strncmp("TRAILER!!!", cpio->filename, cpio->stat.cs_namesize) != 0) {
					if(hny_extraction_cpio_open(cpio, errcode) == 0) {
						cpio->state = HNY_EXTRACTION_CPIO_FILE;
						cpio->offset = 0;
					} else {
						return HNY_EXTRACTION_CPIO_STATUS_ERROR;
					}
				} else {
					cpio->state = HNY_EXTRACTION_CPIO_END;
					cpio->offset = 0;
				}
			}

			buffer = filenameend;
		} break;
		case HNY_EXTRACTION_CPIO_FILE: {
			const char *fileend = buffer + cpio->stat.cs_filesize - cpio->offset;
			const mode_t filetype = cpio->stat.cs_mode & 0770000;

			if(bufferend < fileend) {
				fileend = bufferend;
			}

			size_t copied = fileend - buffer;
			if(filetype == C_ISLNK) {
				memcpy(cpio->filename + cpio->offset, buffer, copied);
			} else if(cpio->fd != -1) {
				size_t written = 0;
				ssize_t writeval;

				while(written != copied
					&& (writeval = write(cpio->fd, buffer, copied - written)) > 0) {
					written += writeval;
				}

				if(writeval == -1) {
					warn("write %s", cpio->filename);
					*errcode = errno;
					return HNY_EXTRACTION_CPIO_STATUS_ERROR;
				}
			}

			cpio->offset += copied;
			if(cpio->offset == cpio->stat.cs_filesize) {
				if(hny_extraction_cpio_close(cpio, filetype, errcode) == 0) {
					cpio->state = HNY_EXTRACTION_CPIO_HEADER;
					cpio->offset = 0;
				} else {
					return HNY_EXTRACTION_CPIO_STATUS_ERROR;
				}
			}

			buffer = fileend;
		} break;
		default: /* HNY_EXTRACTION_CPIO_END */
			return HNY_EXTRACTION_CPIO_STATUS_END;
		}
	}

	return HNY_EXTRACTION_CPIO_STATUS_OK;
}

static int
hny_extraction_cpio_header_serialize_at(struct hny_extraction_cpio_stat *cpiostatp,
	unsigned char byte, size_t position) {
	int valid;

	byte -= '0';
	if(byte <= 7) {
		valid = 0;

		switch(position) {
		case 0:
		case 2:
		case 4:
			if(byte != 0) {
				valid = -1;
			}
			break;
		case 1:
		case 3:
		case 5:
			if(byte != 7) {
				valid = -1;
			}
			break;
		case 6:
			cpiostatp->cs_dev = 0;
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
			cpiostatp->cs_dev = cpiostatp->cs_dev * 8 + byte;
			break;
		case 12:
			cpiostatp->cs_ino = 0;
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
			cpiostatp->cs_ino = cpiostatp->cs_ino * 8 + byte;
			break;
		case 18:
			cpiostatp->cs_mode = 0;
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
			cpiostatp->cs_mode = cpiostatp->cs_mode * 8 + byte;
			break;
		case 24:
			cpiostatp->cs_uid = 0;
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
			cpiostatp->cs_uid = cpiostatp->cs_uid * 8 + byte;
			break;
		case 30:
			cpiostatp->cs_gid = 0;
		case 31:
		case 32:
		case 33:
		case 34:
		case 35:
			cpiostatp->cs_gid = cpiostatp->cs_gid * 8 + byte;
			break;
		case 36:
			cpiostatp->cs_nlink = 0;
		case 37:
		case 38:
		case 39:
		case 40:
		case 41:
			cpiostatp->cs_nlink = cpiostatp->cs_nlink * 8 + byte;
			break;
		case 42:
			cpiostatp->cs_rdev = 0;
		case 43:
		case 44:
		case 45:
		case 46:
		case 47:
			cpiostatp->cs_rdev = cpiostatp->cs_rdev * 8 + byte;
			break;
		case 48:
			cpiostatp->cs_mtime = 0;
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
			cpiostatp->cs_mtime = cpiostatp->cs_mtime * 8 + byte;
			break;
		case 59:
			cpiostatp->cs_namesize = 0;
		case 60:
		case 61:
		case 62:
		case 63:
		case 64:
			cpiostatp->cs_namesize = cpiostatp->cs_namesize * 8 + byte;
			break;
		case 65:
			cpiostatp->cs_filesize = 0;
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
			cpiostatp->cs_filesize = cpiostatp->cs_filesize * 8 + byte;
			break;
		default:
			valid = -1;
			break;
		}
	} else {
		valid = -1;
	}

	return valid;
}

static int
hny_extraction_cpio_filename_has_dot_dot(const char *filename) {
	const char *dotdot;

	while((dotdot = strstr(filename, "..")) != NULL) {
		if((dotdot == filename || dotdot[-1] == '/')
			&& (dotdot[2] == '\0' || dotdot[2] == '/')) {
			return 0;
		}

		filename = dotdot + 2;
	}

	return -1;
}

static int
hny_extraction_cpio_open(struct hny_extraction_cpio *cpio, int *errcode) {
	const char *pathname = cpio->filename;
	if(*pathname == '/') {
		pathname++;
	}

	if(hny_extraction_cpio_filename_has_dot_dot(cpio->filename) == 0) {
		*errcode = EINVAL;
		return -1;
	}

	switch(cpio->stat.cs_mode & 0770000) {
	case C_ISDIR:
		if(mkdirat(cpio->dirfd, pathname, S_IRUSR | S_IWUSR | S_IXUSR) == -1) {
			warn("mkdir %s", pathname);
			*errcode = errno;
			return -1;
		}
		break;
	case C_ISFIFO:
		if(mkfifoat(cpio->dirfd, pathname, S_IRUSR | S_IWUSR) == -1) {
			warn("mkfifo %s", pathname);
			*errcode = errno;
			return -1;
		}
		break;
	case C_ISREG:
		if((cpio->fd = openat(cpio->dirfd, pathname,
			O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR)) == -1) {
			warn("creat %s", pathname);
			*errcode = errno;
			return -1;
		}
		break;
	case C_ISBLK:
	case C_ISCHR:
		if(mknodat(cpio->dirfd, pathname, S_IRUSR | S_IWUSR, cpio->stat.cs_rdev) == -1) {
			warn("mknod %s", pathname);
			*errcode = errno;
			return -1;
		}
		break;
	case C_ISLNK:
		if(cpio->linkcapacity < cpio->stat.cs_filesize) {
			char *newlink = realloc(cpio->link, cpio->stat.cs_filesize);

			if(newlink == NULL) {
				*errcode = errno;
				return -1;
			}

			cpio->link = newlink;
			cpio->linkcapacity = cpio->stat.cs_filesize;
		}
		break;
	case C_ISSOCK:
	case C_ISCTG:
	default:
		break;
	}

	return 0;
}

static int
hny_extraction_cpio_close(struct hny_extraction_cpio *cpio, mode_t filetype, int *errcode) {
	const char *pathname = cpio->filename;
	if(*pathname == '/') {
		pathname++;
	}

	if(filetype == C_ISLNK) {
		cpio->link[cpio->stat.cs_filesize - 1] = '\0';
		if(symlinkat(cpio->link, cpio->dirfd, cpio->filename) == -1) {
			warn("symlink %s", pathname);
			*errcode = errno;
			return -1;
		}
	} else if(cpio->fd != -1) {
		close(cpio->fd);
		cpio->fd = -1;
	}

	uid_t owner;
	gid_t group;

	if(cpio->extractids == 1) {
		owner = cpio->stat.cs_uid;
		group = cpio->stat.cs_gid;
	} else {
		owner = cpio->owner;
		group = cpio->group;
	}

	if(fchownat(cpio->dirfd, pathname, owner, group, AT_SYMLINK_NOFOLLOW) == -1) {
		warn("chown %s", pathname);
		*errcode = errno;
		return -1;
	}

	mode_t savedmask = umask(0);
	int retval = 0;
	/* AT_SYMLINK_NOFOLLOW not yet implemented, generates 'Unsupported operation' on linux */
	if((retval = fchmodat(cpio->dirfd, pathname,
			cpio->stat.cs_mode & 0007777, 0)) == -1) {
		warn("chmod %s", pathname);
		*errcode = errno;
	}
	umask(savedmask);

	return retval;
}
