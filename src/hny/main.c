#include "hny.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

#define HNY_STATUS_BUFFER_DEFAULT_SIZE 64

struct hny_extract_file {
	const char *package;
	const char *name;
	size_t blocksize;
	int fd;
};

static void
hny_extract_file_init(struct hny_extract_file *file,
	char **argpos, char **argend) {

	switch(argend - argpos) {
	case 0:
		errx(EXIT_FAILURE, "extract: Expected arguments");
	case 1: {
		char *base = basename(*argpos);
		char *dot = strrchr(base, '.');

		file->package = dot != NULL ? strndup(base, dot - base) : base;
		file->name = *argpos;
	} break;
	case 2:
		file->package = *argpos;
		file->name = argpos[1];
		break;
	default:
		errx(EXIT_FAILURE, "extract: Too much arguments");
	}

	if((file->fd = open(file->name, O_RDONLY)) == -1) {
		err(EXIT_FAILURE, "extract: Unable to open '%s'", file->name);
	}

	file->blocksize = getpagesize();
}

static void
hny_command_extract(struct hny *hny, char **argpos, char **argend) {
	struct hny_extraction *extraction;
	struct hny_extract_file file;

	hny_extract_file_init(&file, argpos, argend);
	if((errno = hny_extraction_create(&extraction, hny, file.package)) != 0) {
		err(EXIT_FAILURE, "Unable to extract '%s'", file.name);
	}

	char buffer[file.blocksize];
	ssize_t readval;
	int errcode;
	while((readval = read(file.fd, buffer, file.blocksize)) > 0) {
		switch(hny_extraction_extract(extraction, buffer, readval, &errcode)) {
		case HNY_EXTRACTION_STATUS_ERROR_UNARCHIVE:
			errno = errcode;
			err(EXIT_FAILURE, "Unable to extract '%s', error when unarchiving", file.name);
		case HNY_EXTRACTION_STATUS_ERROR_DECOMPRESSION:
			errno = errcode;
			err(EXIT_FAILURE, "Unable to extract '%s', error when decompressing", file.name);
		case HNY_EXTRACTION_STATUS_END:
			return;
		default: /* HNY_EXTRACTION_STATUS_OK */
			break;
		}
	}

	if(readval == -1) {
		err(EXIT_FAILURE, "Unable to read from '%s'", file.name);
	}
}

static inline bool
hny_list_is_dot_or_dot_dot(const char *name) {

	return name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'));
}

static void
hny_command_list(struct hny *hny, char **argpos, char **argend) {
	struct {
		unsigned packages : 1;
		unsigned geister : 1;
	} listing = { 0, 0 };
	DIR *dirp;

	switch(argend - argpos) {
	case 0:
		listing.packages = 1;
		listing.geister = 1;
		break;
	case 1:
		if(strcmp("packages", *argpos) == 0) {
			listing.packages = 1;
		} else if(strcmp("geister", *argpos) == 0) {
			listing.geister = 1;
		} else {
			errx(EXIT_FAILURE, "list: Unexpected argument '%s'", *argpos);
		}
		break;
	default:
		errx(EXIT_FAILURE, "list: Too much arguments");
	}

	if((dirp = opendir(hny_path(hny))) != NULL) {
		struct dirent *entry;

		while((errno = 0, entry = readdir(dirp))) {
			if(entry->d_type == DT_LNK) {
				if(hny_type_of(entry->d_name) == HNY_TYPE_GEIST) {
					if(listing.geister == 1) {
						puts(entry->d_name);
					}
				} else {
					warnx("list: Symbolic link entry '%s' doesn't conform to a geist name", entry->d_name);
				}
			} else if(entry->d_type == DT_DIR) {
				if(!hny_list_is_dot_or_dot_dot(entry->d_name) && listing.packages == 1) {
					if(hny_type_of(entry->d_name) == HNY_TYPE_PACKAGE) {
						puts(entry->d_name);
					} else {
						warnx("list: Directory entry '%s' doesn't conform to a package name", entry->d_name);
					}
				}
			} else {
				warnx("list: Unexpected file type in prefix for '%s'", entry->d_name);
			}
		}

		if(errno != 0) {
			warn("list: Unable to read directory entry");
		}
	} else {
		err(EXIT_FAILURE, "list: Unable to open '%s' for listing", hny_path(hny));
	}
}

static void
hny_command_remove(struct hny *hny, char **argpos, char **argend) {

	if(argpos != argend) {
		while(argpos != argend) {
			if((errno = hny_remove(hny, *argpos)) != 0) {
				warn("Unable to remove '%s'", *argpos);
			}

			argpos++;
		}
	} else {
		errx(EXIT_FAILURE, "remove: Expected arguments");
	}
}

static void
hny_command_shift(struct hny *hny, char **argpos, char **argend) {

	if(argend - argpos == 2) {
		const char *geist = *argpos,
			*target = argpos[1];

		if(hny_type_of(geist) != HNY_TYPE_GEIST) {
			errx(EXIT_FAILURE, "shift: '%s' is not a valid geist name", geist);
		}

		if(hny_type_of(target) == HNY_TYPE_NONE) {
			errx(EXIT_FAILURE, "shift: '%s' is not a valid entry name", target);
		}

		if((errno = hny_shift(hny, geist, target)) != 0) {
			errx(EXIT_FAILURE, "shift: Unable to shift '%s' to '%s'", geist, target);
		}
	} else {
		errx(EXIT_FAILURE, "shift: Expected 2 arguments");
	}
}

static int
hny_buffer_expand(char **bufferp, size_t *sizep) {
	char *newbuffer = realloc(*bufferp, sizeof(*newbuffer) * *sizep * 2);

	if(newbuffer != NULL) {
		*bufferp = newbuffer;
		*sizep = *sizep * 2;
		return 0;
	} else {
		return -1;
	}
}

static int
hny_status_resolve(int dirfd, const char *geist,
	char **buffer1p, size_t *buffer1sizep,
	char **buffer2p, size_t *buffer2sizep) {
	ssize_t length;

	while(stpncpy(*buffer1p, geist, *buffer1sizep) >= *buffer1p + *buffer1sizep
		&& hny_buffer_expand(buffer1p, buffer1sizep) == 0);

	if((*buffer1p)[*buffer1sizep - 1] != '\0') {
		err(EXIT_FAILURE, "status: Unable to expand buffer");
		return -1;
	}

	do {
		char *swap = *buffer2p;
		size_t swapsize = *buffer2sizep;

		*buffer2p = *buffer1p;
		*buffer2sizep = *buffer1sizep;
		*buffer1p = swap;
		*buffer1sizep = swapsize;

		length = readlinkat(dirfd, *buffer2p, *buffer1p, *buffer1sizep);
	} while(length != -1
		&& (length < *buffer1sizep || hny_buffer_expand(buffer1p, buffer1sizep) == 0)
		&& ((*buffer1p)[length] = '\0', hny_type_of(*buffer1p) == HNY_TYPE_GEIST));

	if(length == -1) {
		warn("status: Unable to readlink '%s'", *buffer2p);
		return -1;
	} else if(length == *buffer1sizep) {
		err(EXIT_FAILURE, "status: Unable to expand buffer");
		return -1;
	} else {
		return 0;
	}
}

static void
hny_command_status(struct hny *hny, char **argpos, char **argend) {
	DIR *dirp = opendir(hny_path(hny));

	if(dirp != NULL) {
		size_t buffer1size = HNY_STATUS_BUFFER_DEFAULT_SIZE,
			buffer2size = HNY_STATUS_BUFFER_DEFAULT_SIZE;
		char *buffer1 = malloc(buffer1size), *buffer2 = malloc(buffer2size);

		if(buffer1 == NULL || buffer2 == NULL) {
			err(EXIT_FAILURE, "status: Unable to allocate buffer");
		}

		while(argpos != argend) {
			if(hny_type_of(*argpos) == HNY_TYPE_GEIST) {
				if(hny_status_resolve(dirfd(dirp), *argpos,
					&buffer1, &buffer1size,
					&buffer2, &buffer2size) == 0) {
					struct stat st;

					if(fstatat(dirfd(dirp), buffer1, &st, AT_SYMLINK_NOFOLLOW) == 0) {
						if(S_ISDIR(st.st_mode)) {
							puts(buffer1);
						} else {
							warnx("status: '%s' is not a directory", buffer1);
						}
					} else {
						warn("status: stat '%s'", buffer1);
					}
				}
			} else {
				warnx("status: '%s' is not a valid geist name", *argpos);
			}

			argpos++;
		}
	} else {
		err(EXIT_FAILURE, "status: Unable to open '%s'", hny_path(hny));
	}
}

static void
hny_usage(const char *hnyname, int status) {

	fprintf(stderr, "usage: %s [-hb] [-p <prefix>] extract [<geist>] <file>\n"
		"       %s [-hb] [-p <prefix>] list [packages|geister]\n"
		"       %s [-hb] [-p <prefix>] remove [<entry>...]\n"
		"       %s [-hb] [-p <prefix>] shift <geist> <target>\n"
		"       %s [-hb] [-p <prefix>] status [<geist>...]\n",
		hnyname, hnyname, hnyname, hnyname, hnyname);
	exit(status);
}

static struct hny *
hny_parse_args(int argc, char **argv) {
	char *prefix = getenv("HNY_PREFIX");
	int flags = HNY_FLAGS_NONE;
	struct hny *hny;
	int c;

	while((c = getopt(argc, argv, ":habp:")) != -1) {
		switch(c) {
		case 'h':
			hny_usage(*argv, EXIT_SUCCESS);
		case 'b':
			flags |= HNY_FLAGS_BLOCK;
			break;
		case 'p':
			prefix = optarg;
			break;
		case ':':
			warnx("Option -%c requires an operand\n", optopt);
			hny_usage(*argv, EXIT_FAILURE);
		case '?':
			warnx("Unrecognized option -%c\n", optopt);
			hny_usage(*argv, EXIT_FAILURE);
		}
	}

	if(prefix == NULL) {
		prefix = "/hub";
	}

	if((errno = hny_open(&hny, prefix, flags)) != 0) {
		err(EXIT_FAILURE, "Unable to open honey prefix '%s'", prefix);
	}

	return hny;
}

static const struct hny_command {
	const char *command;
	void (*handle)(struct hny *, char **, char **);
} hnycommands[] = {
	{ "extract", hny_command_extract },
	{ "list", hny_command_list },
	{ "remove", hny_command_remove },
	{ "shift", hny_command_shift },
	{ "status", hny_command_status },
	{ NULL, NULL },
};

int
main(int argc, char **argv) {
	struct hny *hny = hny_parse_args(argc, argv);
	char **argpos = argv + optind, ** const argend = argv + argc;

	if(argpos != argend) {
		const struct hny_command *current = hnycommands;

		while(current->command != NULL
			&& strcmp(current->command, *argpos) != 0) {
			current++;
		}

		if(current->command != NULL) {
			current->handle(hny, argpos + 1, argend);
		} else {
			warnx("Invalid command '%s'", *argpos);
			hny_usage(*argv, EXIT_FAILURE);
		}
	} else {
		warnx("Expected command");
		hny_usage(*argv, EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}

