#include "hny.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

struct hny_export_file {
	const char *package;
	const char *name;
	size_t blocksize;
	int fd;
};

static void
hny_export_file_init(struct hny_export_file *file,
	char **argpos, char **argend) {

	switch(argend - argpos) {
	case 0:
		errx(EXIT_FAILURE, "export: Expected arguments");
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
		errx(EXIT_FAILURE, "export: Too much arguments");
	}

	if((file->fd = open(file->name, O_RDONLY)) == -1) {
		err(EXIT_FAILURE, "export: Unable to open %s", file->name);
	}

	file->blocksize = getpagesize();
}

static void
hny_export(struct hny *hny, char **argpos, char **argend) {
	struct hny_extraction *extraction;
	struct hny_export_file file;

	hny_export_file_init(&file, argpos, argend);
	if((errno = hny_extraction_create(&extraction, hny, file.package)) != 0) {
		err(EXIT_FAILURE, "Unable to extract %s", file.name);
	}

	char buffer[file.blocksize];
	ssize_t readval;
	int errcode;
	while((readval = read(file.fd, buffer, file.blocksize)) > 0) {
		switch(hny_extraction_extract(extraction, buffer, readval, &errcode)) {
		case HNY_EXTRACTION_STATUS_ERROR_UNARCHIVE:
			errno = errcode;
			err(EXIT_FAILURE, "Unable to extract %s, error when unarchiving", file.name);
		case HNY_EXTRACTION_STATUS_ERROR_DECOMPRESSION:
			errno = errcode;
			err(EXIT_FAILURE, "Unable to extract %s, error when decompressing", file.name);
		case HNY_EXTRACTION_STATUS_END:
			return;
		default: /* HNY_EXTRACTION_STATUS_OK */
			break;
		}
	}

	if(readval == -1) {
		err(EXIT_FAILURE, "Unable to read from %s", file.name);
	}
}

static void
hny_remove_list(struct hny *hny, char **argpos, char **argend) {

	if(argpos != argend) {
		while(argpos != argend) {
			int errcode = hny_remove(hny, *argpos);

			if(errcode != 0) {
				errno = errcode;
				warn("Unable to remove %s", *argpos);
			}

			argpos++;
		}
	} else {
		errx(EXIT_FAILURE, "remove: Expected arguments");
	}
}

static void
hny_usage(const char *hnyname, int status) {

	fprintf(stderr, "usage: %s [-h] [-p <prefix>] export [filename]\n",
		hnyname);
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
		err(EXIT_FAILURE, "Unable to open honey prefix %s", prefix);
	}

	return hny;
}

static const struct hny_command {
	const char *command;
	void (*handle)(struct hny *, char **, char **);
} hnycommands[] = {
	{ "export", hny_export },
	{ "remove", hny_remove_list },
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
			warnx("Invalid command %s", *argpos);
			hny_usage(*argv, EXIT_FAILURE);
		}
	} else {
		warnx("Expected command");
		hny_usage(*argv, EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}

