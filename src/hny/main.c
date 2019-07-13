/*
	main.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE
*/
#include <hny.h>

#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

#ifdef __clang__
#define HNY_NORETURN __attribute__((__noreturn__))
#else
#define HNY_NORETURN
#endif

#define DEFAULT		"\x1b[0m"
#define BOLD		"\x1b[1m"
#define RED		"\x1b[31m"
#define MAGENTA		"\x1b[35m"

#define usage() usage_with_error(HNY_ERROR_INVALID_ARGS)

static struct hny *hny;
static int hny_already_accepted;

static void
print(const char *format,
	...) {
	va_list args;

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

static void
print_error(const char *format,
	...) {
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

static void HNY_NORETURN
out(enum hny_error error) {

	if(hny != NULL) {
		hny_close(hny);
	}

	exit(error);
}

static void
usage_with_error(enum hny_error error) {

	print_error(BOLD RED "usage" DEFAULT ": hny [-h|-a|-b] [-p path] <command> [arguments...]\n");
	out(error);
}

static void
verify(int cmdargc,
	char **cmdargv) {
	enum hny_error error, retval = HNY_ERROR_NONE;
	char **iterator = cmdargv;
	char **end = cmdargv + cmdargc;

	while(iterator != end) {
		char *file = *iterator;
		char *eula;
		size_t len;

		if((error = hny_verify(file, &eula, &len)) == HNY_ERROR_NONE) {

			if(hny_already_accepted == 0) {
				int answer;

				print(BOLD "EULA" DEFAULT ":\n%*s\nDo you accept it? [y/N] ", len, eula);
				fflush(stdout);

				answer = getchar();
				if(answer == 'y') {
					print("You accepted EULA for package %s\n", file);
				} else {
					print_error("You didn't accept EULA for package %s\n", file);
					error = HNY_ERROR_UNAUTHORIZED;
				}
			} else {
				print("You already accepted EULA for package %s\n", file);
			}

			free(eula);
		} else {
			retval = HNY_ERROR_INVALID_ARGS;

			switch(error) {
			case HNY_ERROR_MISSING:
				print_error("File \"%s\" is not a honey package\n", file);
				break;
			case HNY_ERROR_UNAVAILABLE:
				print_error("Cannot access eula of file \"%s\"\n", file);
				break;
			default: /* HNY_ERROR_INVALID_ARGS */
				print_error("Cannot access file \"%s\"\n", file);
				break;
			}
		}

		iterator++;
	}

	out(retval);
}

static void
export(int cmdargc,
	char **cmdargv) {
	enum hny_error error;
	struct hny_geist *package;

	if(cmdargc != 2
		|| (package = hny_alloc_geister((const char **)(cmdargv + 1), 1)) == NULL) {
		print_error(BOLD MAGENTA "expected" DEFAULT ": hny export [package file] [package name]\n");
		out(HNY_ERROR_INVALID_ARGS);
	}

	if((error = hny_export(hny, cmdargv[0], package)) == HNY_ERROR_NONE) {
		print("%s exported as %s\n", cmdargv[0], cmdargv[1]);
	} else {
		switch(error) {
		case HNY_ERROR_UNAVAILABLE:
			print_error("Cannot export \"%s\", prefix busy\n", cmdargv[0]);
			break;
		case HNY_ERROR_INVALID_ARGS:
			print_error("Cannot export \"%s\", invalid integrity\n", cmdargv[0]);
			break;
		default: /* HNY_ERROR_MISSING */
			print_error("Cannot export \"%s\", missing resource\n", cmdargv[0]);
			break;
		}
		out(error);
	}

	hny_free_geister(package, 1);
}

static void
shift(int cmdargc,
	char **cmdargv) {
	enum hny_error error;
	struct hny_geist *package;

	if(cmdargc != 2
		|| (package = hny_alloc_geister((const char **)(cmdargv + 1), 1)) == NULL) {
		print_error(BOLD MAGENTA "expected" DEFAULT ": hny shift [target geist] [source geist]\n");
		out(HNY_ERROR_INVALID_ARGS);
	}

	if((error = hny_shift(hny, cmdargv[0], package)) == HNY_ERROR_NONE) {
		print("%s shifted for %s\n", cmdargv[0], cmdargv[1]);
	} else {
		print_error("Unable to shift Honey package \"%s\"\n", cmdargv[1]);
		out(error);
	}

	hny_free_geister(package, 1);
}

static void
list(int cmdargc,
	char **cmdargv) {
	enum hny_error retval = HNY_ERROR_NONE;
	enum hny_listing list;

	if(cmdargc == 1) {
		if(strcmp("active", cmdargv[0]) == 0) {
			list = HNY_LIST_ACTIVE;
		} else if(strcmp("packages", cmdargv[0]) == 0) {
			list = HNY_LIST_PACKAGES;
		} else {
			print_error("Invalid argument for listing \"%s\"\n", cmdargv[0]);
			usage();
		}
	} else {
		print_error("Invalid number of arguments for listing\n");
		usage();
	}

	if(retval == HNY_ERROR_NONE) {
		struct hny_geist *geister;
		size_t len;

		if(hny_list(hny, list, &geister, &len) == HNY_ERROR_NONE) {
			struct hny_geist *iterator = geister,
				*end = geister + len;

			while(iterator != end) {
				if(iterator->version != NULL) {
					print("%s-%s\n",
						iterator->name,
						iterator->version);
				} else {
					print("%s\n",
						iterator->name);
				}

				iterator++;
			}
		} else {
			retval = HNY_ERROR_UNAVAILABLE;
		}
	}

	out(retval);
}

static void
erase(int cmdargc,
	char **cmdargv) {
	enum hny_error retval = HNY_ERROR_NONE;
	struct hny_geist *geister
		= hny_alloc_geister((const char **)cmdargv, cmdargc);

	if(geister == NULL) {
		print_error("Invalid geist name\n");
		retval = HNY_ERROR_INVALID_ARGS;
	} else {
		int i;

		for(i = 0; i < cmdargc; i++) {
			switch(hny_erase(hny, geister + i)) {
			case HNY_ERROR_NONE:
				if(geister[i].version == NULL) {
					print("Geist %s unlinked\n", geister[i].name);
				} else {
					print("Package %s-%s removed\n",
						geister[i].name, geister[i].version);
				}
				break;
			case HNY_ERROR_UNAVAILABLE:
				print_error("Unable to erase %s, prefix is unavailable\n",
					cmdargv[i]);
				retval = HNY_ERROR_UNAVAILABLE;
				break;
			case HNY_ERROR_MISSING:
				print_error("Unable to erase %s, missing resource\n",
					cmdargv[i]);
				retval = HNY_ERROR_MISSING;
				break;
			case HNY_ERROR_UNAUTHORIZED:
				print_error("Unable to erase %s, unauthorized\n",
					cmdargv[i]);
				retval = HNY_ERROR_UNAUTHORIZED;
				break;
			default:
				print_error("Unable to erase %s\n",
					cmdargv[i]);
				retval = HNY_ERROR_INVALID_ARGS;
				break;
			}
		}

		hny_free_geister(geister, cmdargc);
	}

	out(retval);
}

static void
status(int cmdargc,
	char **cmdargv) {
	enum hny_error retval = HNY_ERROR_NONE;
	struct hny_geist *geister
		= hny_alloc_geister((const char **)cmdargv, cmdargc);

	if(geister == NULL) {
		print_error("Invalid geist name\n");
		retval = HNY_ERROR_INVALID_ARGS;
	} else {
		int i;

		for(i = 0; i < cmdargc; i++) {
			struct hny_geist *target;

			switch(hny_status(hny, geister + i, &target)) {
			case HNY_ERROR_NONE:
				print("%s-%s\n", target->name, target->version);
				hny_free_geister(target, 1);
				break;
			case HNY_ERROR_UNAVAILABLE:
				print_error("Unable to status %s: prefix is unavailable\n",
					cmdargv[i]);
				retval = HNY_ERROR_UNAVAILABLE;
				break;
			case HNY_ERROR_MISSING:
				print_error("Unable to status %s: the geist doesn't have a target\n",
					cmdargv[i]);
				retval = HNY_ERROR_MISSING;
				break;
			case HNY_ERROR_UNAUTHORIZED:
				print_error("Unable to status %s: unauthorized\n",
					cmdargv[i]);
				retval = HNY_ERROR_UNAUTHORIZED;
				break;
			default:
				print_error("Unable to status %s\n",
					cmdargv[i]);
				retval = HNY_ERROR_INVALID_ARGS;
				break;
			}
		}

		hny_free_geister(geister, cmdargc);
	}

	out(retval);
}

static void
execute(enum hny_action action,
	int cmdargc,
	char **cmdargv) {
	enum hny_error retval = HNY_ERROR_NONE;
	struct hny_geist *geister
		= hny_alloc_geister((const char **)cmdargv, cmdargc);

	if(geister == NULL) {
		print_error("Invalid geist name\n");
		retval = HNY_ERROR_INVALID_ARGS;
	} else {
		int i;

		for(i = 0; i < cmdargc; i++) {
			switch(hny_execute(hny, action, geister + i)) {
			case HNY_ERROR_NONE:
				break;
			case HNY_ERROR_UNAVAILABLE:
				print_error("Missing resource to execute action for %s\n",
					cmdargv[i]);
				retval = HNY_ERROR_UNAVAILABLE;
				break;
			case HNY_ERROR_MISSING:
				print_error("Non existant resource to execute action for %s\n",
					cmdargv[i]);
				retval = HNY_ERROR_MISSING;
				break;
			case HNY_ERROR_UNAUTHORIZED:
				print_error("Unauthorized to execute action for %s\n",
					cmdargv[i]);
				retval = HNY_ERROR_UNAUTHORIZED;
				break;
			default:
				print_error("Invalid arguments to execute action for %s\n",
					cmdargv[i]);
				retval = HNY_ERROR_INVALID_ARGS;
				break;
			}
		}

		hny_free_geister(geister, cmdargc);
	}

	out(retval);
}

int
main(int argc,
	char **argv) {
	char *prefix = getenv("HNY_PREFIX");
	int flags = HNY_FLAGS_NONE;
	int opt;

	while((opt = getopt(argc, argv, ":habp:")) != -1) {
		switch(opt) {
		case 'h':
			usage_with_error(HNY_ERROR_NONE);
		case 'a':
			hny_already_accepted = 1;
			break;
		case 'b':
			flags |= HNY_FLAGS_BLOCK;
			break;
		case 'p':
			prefix = optarg;
			break;
		case ':':
			print_error("Option -%c requires an operand\n", optopt);
			usage();
		case '?':
			print_error("Unrecognized option -%c\n", optopt);
			usage();
		}
	}

	if(optind < argc) {
		int cmdargc = argc - (optind + 1);
		char **cmdargv = argv + optind + 1;
		char *cmd = argv[optind];

		if(strcmp("verify", cmd) == 0) {
			verify(cmdargc, cmdargv);
		} else {
			if(prefix == NULL
				|| hny_open(prefix, flags, &hny) != HNY_ERROR_NONE) {
				print_error("Unable to access prefix %s\n", prefix);
				out(HNY_ERROR_UNAVAILABLE);
			}

			if(strcmp("export", cmd) == 0) {
				export(cmdargc, cmdargv);
			} else if(strcmp("shift", cmd) == 0) {
				shift(cmdargc, cmdargv);
			} else if(strcmp("list", cmd) == 0) {
				list(cmdargc, cmdargv);
			} else if(strcmp("erase", cmd) == 0) {
				erase(cmdargc, cmdargv);
			} else if(strcmp("status", cmd) == 0) {
				status(cmdargc, cmdargv);
			} else if(strcmp("setup", cmd) == 0) {
				execute(HNY_ACTION_SETUP, cmdargc, cmdargv);
			} else if(strcmp("clean", cmd) == 0) {
				execute(HNY_ACTION_CLEAN, cmdargc, cmdargv);
			} else if(strcmp("reset", cmd) == 0) {
				execute(HNY_ACTION_RESET, cmdargc, cmdargv);
			} else if(strcmp("check", cmd) == 0) {
				execute(HNY_ACTION_CHECK, cmdargc, cmdargv);
			} else if(strcmp("purge", cmd) == 0) {
				execute(HNY_ACTION_PURGE, cmdargc, cmdargv);
			} else {
				print_error("Invalid command \"%s\"\n", cmd);
				usage();
			}
		}
	} else {
		usage();
	}

	out(HNY_ERROR_NONE);
}
