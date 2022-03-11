/* SPDX-License-Identifier: BSD-3-Clause */
#include <hny.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <alloca.h>
#include <dirent.h>
#include <libgen.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

#include "config.h"

struct hny_args {
	const char *prefix;
	int flags;
};

struct hny_buffer {
	char *data;
	size_t capacity;
};

static void
hny_subcommand_extract(struct hny *hny, char **argpos, char **argend) {
	const char *package, *filename;
	char *buffer;
	size_t size;
	int fd;

	{ /* Argument parsing */
		switch (argend - argpos) {
		case 0:
			errx(EXIT_FAILURE, "extract: Expected arguments");
		case 1: {
			const size_t length = strlen(*argpos) + 1;
			char * const bname = basename(strncpy(alloca(length), *argpos, length));
			char * const dot = strrchr(bname, '.');

			if (dot != NULL) {
				*dot = '\0';
			}

			package = bname; /* alloca() makes allocation safe for function duration */
			filename = *argpos;
		} break;
		case 2:
			package = argpos[0];
			filename = argpos[1];
			break;
		default:
			errx(EXIT_FAILURE, "extract: Too many arguments");
		}
	}

	/* Opening input file */
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		err(EXIT_FAILURE, "extract: Unable to open '%s'", filename);
	}

	/* Allocating exchange buffer TODO: malloc if size exceeds some threshold */
	size = getpagesize();
	buffer = alloca(size);

	/* Lock prefix */
	if (errno = hny_lock(hny), errno != 0) {
		err(EXIT_FAILURE, "extract: Unable to lock prefix '%s'", filename);
	}

	{ /* Extraction loop */
		struct hny_extraction *extraction;
		enum hny_extraction_status status;
		ssize_t readval;

		if (errno = hny_extraction_create(&extraction, hny, package), errno != 0) {
			err(EXIT_FAILURE, "extract: Unable to extract '%s'", filename);
		}

		while ((readval = read(fd, buffer, size), readval >= 0) && (status = hny_extraction_extract(extraction, buffer, readval), status == HNY_EXTRACTION_STATUS_OK));

		if (readval == -1) {
			err(EXIT_FAILURE, "extract: Unable to read from '%s'", filename);
		}

		if (HNY_EXTRACTION_STATUS_IS_ERROR(status)) {
			if (HNY_EXTRACTION_STATUS_IS_ERROR_XZ(status)) {
				errx(EXIT_FAILURE, "extract: Unable to extract '%s', error while uncompressing", filename);
			} else if (HNY_EXTRACTION_STATUS_IS_ERROR_CPIO(status)) {
				if (HNY_EXTRACTION_STATUS_IS_ERROR_CPIO_SYSTEM(status)) {
					errno = hny_extraction_errcode(extraction);
					err(EXIT_FAILURE, "extract: Unable to extract '%s', error while unarchiving", filename);
				} else {
					errx(EXIT_FAILURE, "extract: Unable to extract '%s', error while unarchiving", filename);
				}
			} else {
				errx(EXIT_FAILURE, "extract: Unable to extract '%s', archive not finished", filename);
			}
		}

		hny_extraction_destroy(extraction);
	}

	hny_unlock(hny);
	close(fd);
}

static inline bool
hny_list_is_dot_or_dot_dot(const char *name) {
	return name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'));
}

static void
hny_subcommand_list(struct hny *hny, char **argpos, char **argend) {
	bool packages = false, geister = false;
	struct dirent *entry;
	DIR *dirp;

	switch (argend - argpos) {
	case 0:
		packages = true;
		geister = true;
		break;
	case 1:
		if (strcmp("packages", *argpos) == 0) {
			packages = true;
		} else if (strcmp("geister", *argpos) == 0) {
			geister = true;
		} else {
			errx(EXIT_FAILURE, "list: Unexpected argument '%s'", *argpos);
		}
		break;
	default:
		errx(EXIT_FAILURE, "list: Too many arguments");
	}

	dirp = opendir(hny_path(hny));
	if (dirp == NULL) {
		err(EXIT_FAILURE, "list: Unable to open '%s' for listing", hny_path(hny));
	}

	while (errno = 0, entry = readdir(dirp)) {
		switch (entry->d_type) {
		case DT_LNK:
			if (hny_type_of(entry->d_name) != HNY_TYPE_GEIST) {
				errx(EXIT_FAILURE, "list: Inconsistency, symbolic link entry '%s' doesn't conform to a geist name", entry->d_name);
			}
			if (geister) {
				puts(entry->d_name);
			}
			break;
		case DT_DIR:
			if (!hny_list_is_dot_or_dot_dot(entry->d_name)) {
				if (hny_type_of(entry->d_name) != HNY_TYPE_PACKAGE) {
					errx(EXIT_FAILURE, "list: Inconsistency, directory entry '%s' doesn't conform to a package name", entry->d_name);
				}
				if (packages) {
					puts(entry->d_name);
				}
			}
			break;
		default:
			errx(EXIT_FAILURE, "list: Inconsistency, unexpected file type in prefix for '%s'", entry->d_name);
		}
	}

	if (errno != 0) {
		err(EXIT_FAILURE, "list: Unable to read directory entry");
	}

	closedir(dirp);
}

static void
hny_subcommand_remove(struct hny *hny, char **argpos, char **argend) {

	if (argpos == argend) {
		errx(EXIT_FAILURE, "remove: Expected arguments");
	}

	if (errno = hny_lock(hny), errno != 0) {
		err(EXIT_FAILURE, "remove: Unable to lock prefix");
	}

	while (argpos != argend) {
		const char *entry = *argpos;

		if (errno = hny_remove(hny, entry), errno != 0) {
			err(EXIT_FAILURE, "remove: Unable to remove '%s'", entry);
		}

		argpos++;
	}

	hny_unlock(hny);
}

static void
hny_subcommand_shift(struct hny *hny, char **argpos, char **argend) {

	if (argend - argpos != 2) {
		errx(EXIT_FAILURE, "shift: Expected 2 arguments");
	}

	const char * const geist = argpos[0];
	if (hny_type_of(geist) != HNY_TYPE_GEIST) {
		errx(EXIT_FAILURE, "shift: '%s' is not a valid geist name", geist);
	}

	const char * const target = argpos[1];
	if (hny_type_of(target) == HNY_TYPE_NONE) {
		errx(EXIT_FAILURE, "shift: '%s' is not a valid entry name", target);
	}

	if (errno = hny_lock(hny), errno != 0) {
		err(EXIT_FAILURE, "shift: Unable to lock prefix");
	}

	if (errno = hny_shift(hny, geist, target), errno != 0) {
		err(EXIT_FAILURE, "shift: Unable to shift '%s' to '%s'", geist, target);
	}

	hny_unlock(hny);
}

static void
hny_buffer_init(struct hny_buffer *buffer) {

	buffer->capacity = CONFIG_HNY_STATUS_BUFFER_DEFAULT_CAPACITY;
	buffer->data = malloc(buffer->capacity);

	if (buffer->data == NULL) {
		err(EXIT_FAILURE, "status: Unable to allocate memory buffer of %lu bytes", buffer->capacity);
	}
}

static void
hny_buffer_expand(struct hny_buffer *buffer) {
	const size_t newcapacity = buffer->capacity * 2;
	char * const newdata = realloc(buffer->data, newcapacity);

	if (newdata == NULL) {
		err(EXIT_FAILURE, "status: Unable to expand memory buffer up to %lu bytes", newcapacity);
	}

	buffer->data = newdata;
	buffer->capacity = newcapacity;
}

static int
hny_status_resolve(int dirfd, const char *geist, struct hny_buffer *buffer1, struct hny_buffer *buffer2) {

	/* Fill buffer 1 with first geist */
	while (stpncpy(buffer1->data, geist, buffer1->capacity) >= buffer1->data + buffer1->capacity) {
		hny_buffer_expand(buffer1);
	}

	/* While buffer 1 targets a geist */
	while (hny_type_of(buffer1->data) == HNY_TYPE_GEIST) {
		ssize_t length;

		/* Resolve buffer 1 in buffer 2, expanding it if necessary */
		while (length = readlinkat(dirfd, buffer1->data, buffer2->data, buffer2->capacity), length != -1 && length >= buffer2->capacity) {
			hny_buffer_expand(buffer2);
		}

		/* If we weren't able to readlink, return error, buffer 1 holds the latest entry */
		if (length == -1) {
			return -1;
		}

		/* Don't forget to terminate the string, readlink doesn't do it for us */
		buffer2->data[length] = '\0';

		/* Swap buffer 1 & 2 so buffer 1 holds latest */
		struct hny_buffer swap = *buffer2;
		*buffer2 = *buffer1;
		*buffer1 = swap;
	}

	return 0;
}

static void
hny_subcommand_status(struct hny *hny, char **argpos, char **argend) {
	DIR * const dirp = opendir(hny_path(hny));
	struct hny_buffer buffer1, buffer2;

	if (dirp == NULL) {
		err(EXIT_FAILURE, "status: Unable to open '%s'", hny_path(hny));
	}

	hny_buffer_init(&buffer1);
	hny_buffer_init(&buffer2);

	while (argpos != argend) {
		const char * const geist = *argpos;

		if (hny_status_resolve(dirfd(dirp), geist, &buffer1, &buffer2) == 0) {
			const char * const package = buffer1.data;
			struct stat st;

			if (fstatat(dirfd(dirp), package, &st, AT_SYMLINK_NOFOLLOW) == 0) {
				if (S_ISDIR(st.st_mode)) {
					puts(package);
				} else {
					errx(EXIT_FAILURE, "status: '%s' is not a directory", package);
				}
			} else {
				err(EXIT_FAILURE, "status: stat '%s'", package);
			}
		} else {
			err(EXIT_FAILURE, "status: Unable to readlink '%s'", buffer1.data);
		}

		argpos++;
	}

	free(buffer1.data);
	free(buffer2.data);
	closedir(dirp);
}

static void
hny_action(struct hny *hny, const char *path, const char *action, char **argpos, char **argend) {

	while (argpos != argend) {
		const char * const entry = *argpos;
		siginfo_t info;
		pid_t pid;

		/* Checking if it's a valid entry name */
		if (hny_type_of(entry) == HNY_TYPE_NONE) {
			errx(EXIT_FAILURE, "%s: '%s' is not a valid entry name", action, entry);
		}

		/* Spawning child */
		printf("%s: Starting for '%s'\n", action, entry);
		fflush(stdout);

		if (errno = hny_spawn(hny, entry, path, &pid), errno != 0) {
			err(EXIT_FAILURE, "%s: Unable to start for '%s'", action, entry);
		}

		/* Waiting for child termination immediately */
		while (waitid(P_PID, pid, &info, WEXITED) == 0) {
			switch (info.si_code) {
			case CLD_EXITED:
				if (info.si_status != 0) {
					errx(EXIT_FAILURE, "%s: Terminated with exit status %d\n", action, info.si_status);
				}
				break;
			case CLD_KILLED:
				errx(EXIT_FAILURE, "%s: Process killed by signal %s (%d)\n", action, strsignal(info.si_status), info.si_status);
				break;
			case CLD_DUMPED:
				errx(EXIT_FAILURE, "%s: Process dumped core (%d)\n", action, info.si_status);
				break;
			default:
				break;
			}
		}

		argpos++;
	}
}

static void
hny_subcommand_setup(struct hny *hny, char **argpos, char **argend) {
	hny_action(hny, "pkg/setup", "setup", argpos, argend);
}

static void
hny_subcommand_clean(struct hny *hny, char **argpos, char **argend) {
	hny_action(hny, "pkg/clean", "clean", argpos, argend);
}

static void
hny_subcommand_reset(struct hny *hny, char **argpos, char **argend) {
	hny_action(hny, "pkg/reset", "reset", argpos, argend);
}

static void
hny_subcommand_check(struct hny *hny, char **argpos, char **argend) {
	hny_action(hny, "pkg/check", "check", argpos, argend);
}

static void
hny_subcommand_purge(struct hny *hny, char **argpos, char **argend) {
	hny_action(hny, "pkg/purge", "purge", argpos, argend);
}

static void noreturn
hny_usage(int status) {
	const char * const progname
#ifdef CONFIG_HAS_GETPROGNAME
		= getprogname();
#else
		= "hny";
#endif

	fprintf(stderr, "usage: %s [-hb] [-p <prefix>] extract [<geist>] <file>\n"
		"       %s [-h] [-p <prefix>] list [packages|geister]\n"
		"       %s [-hb] [-p <prefix>] remove [<entry>...]\n"
		"       %s [-hb] [-p <prefix>] shift <geist> <target>\n"
		"       %s [-h] [-p <prefix>] status [<geist>...]\n"
		"       %s [-h] [-p <prefix>] <subcommand> [<entry>...]\n",
		progname, progname, progname, progname, progname, progname);

	exit(status);
}

static const struct hny_args
hny_parse_args(int argc, char **argv) {
	struct hny_args args = {
		.prefix = getenv("HNY_PREFIX"),
		.flags = HNY_FLAGS_NONE,
	};
	int c;

#ifdef CONFIG_HAS_SETPROGNAME
	setprogname(*argv);
#endif

	while (c = getopt(argc, argv, ":hbp:"), c != -1) {
		switch (c) {
		case 'h':
			hny_usage(EXIT_SUCCESS);
		case 'b':
			args.flags |= HNY_FLAGS_BLOCK;
			break;
		case 'p':
			args.prefix = optarg;
			break;
		case ':':
			warnx("Option -%c requires an operand\n", optopt);
			hny_usage(EXIT_FAILURE);
		case '?':
			warnx("Unrecognized option -%c\n", optopt);
			hny_usage(EXIT_FAILURE);
		}
	}

	if (args.prefix == NULL) {
		args.prefix = "/hub";
	}

	if (optind == argc) {
		warnx("Expected subcommand");
		hny_usage(EXIT_FAILURE);
	}

	return args;
}

int
main(int argc, char **argv) {
	static const struct {
		const char *name;
		void (*run)(struct hny *, char **, char **);
	} subcommands[] = {
		{ "extract", hny_subcommand_extract },
		{ "list", hny_subcommand_list },
		{ "remove", hny_subcommand_remove },
		{ "shift", hny_subcommand_shift },
		{ "status", hny_subcommand_status },
		{ "setup", hny_subcommand_setup },
		{ "clean", hny_subcommand_clean },
		{ "reset", hny_subcommand_reset },
		{ "check", hny_subcommand_check },
		{ "purge", hny_subcommand_purge },
	};
	const struct hny_args args = hny_parse_args(argc, argv);
	unsigned int index = 0;
	struct hny *hny;

	while (index < sizeof (subcommands) / sizeof (*subcommands) && strcmp(argv[optind], subcommands[index].name) != 0) {
		index++;
	}

	if (index == sizeof (subcommands) / sizeof (*subcommands)) {
		warnx("Invalid subcommand '%s'", argv[optind]);
		hny_usage(EXIT_FAILURE);
	}

	if (errno = hny_open(&hny, args.prefix, args.flags), errno != 0) {
		err(EXIT_FAILURE, "Unable to open honey prefix '%s'", args.prefix);
	}

	subcommands[index].run(hny, argv + optind + 1, argv + argc);

	hny_close(hny);

	return EXIT_SUCCESS;
}

