/*
	main.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include <hny.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define DEFAULT		"\x1b[0m"
#define BOLD		"\x1b[1m"
#define RED		"\x1b[31m"
#define MAGENTA		"\x1b[35m"

static hny_t hny;
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

static void __attribute__ ((__noreturn__))
out(enum hny_error error) {

	if(hny != NULL) {
		hny_destroy(hny);
	}

	exit(error);
}

static void
usage(void) {

	print_error(BOLD RED "usage" DEFAULT ": hny [--help] [--accepted-eulas|-a] [--prefix=<path>] <command> <args>\n");
	out(HnyErrorInvalidArgs);
}

static void
verify(int cmdargc,
	char **cmdargv) {
	enum hny_error error, retval = HnyErrorNone;
	char **iterator = cmdargv;
	char **end = &cmdargv[cmdargc];

	while(iterator != end) {
		char *file = *iterator;
		char *eula;
		size_t len;

		if((error = hny_verify(hny, file, &eula, &len)) == HnyErrorNone) {

			if(hny_already_accepted == 0) {
				int answer;

				print(BOLD "EULA" DEFAULT ":\n%*s\nDo you accept it? [y/N] ", len, eula);

				answer = getchar();
				if(answer == 'y') {
					print("You accepted EULA for package %s\n", file);
				} else {
					print_error("You didn't accept EULA for package %s\n", file);
					error = HnyErrorUnauthorized;
				}
			} else {
				print("You already accepted EULA for package %s\n", file);
			}

			free(eula);
		} else {
			retval = HnyErrorInvalidArgs;

			switch(error) {
			case HnyErrorNonExistant:
				print_error("File \"%s\" is not a Honey package\n", file);
				break;
			case HnyErrorUnavailable:
				print_error("Cannot access eula of file \"%s\"\n", file);
				break;
			case HnyErrorUnauthorized:
				print_error("Unauthorized access for file \"%s\"\n", file);
				break;
			default:
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
		|| (package = hny_alloc_geister((const char **)&cmdargv[1], 1)) == NULL) {
		print_error(BOLD MAGENTA "expected" DEFAULT ": hny export [package file] [package name]\n");
		out(HnyErrorInvalidArgs);
	}

	if((error = hny_export(hny, cmdargv[0], package)) == HnyErrorNone) {
		print("%s exported as %s\n", cmdargv[0], cmdargv[1]);
	} else {
		print_error("Unable to export Honey package\n");
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
		|| (package = hny_alloc_geister((const char **)&cmdargv[1], 1)) == NULL) {
		print_error(BOLD MAGENTA "expected" DEFAULT ": hny shift [target geist] [source geist]\n");
		out(HnyErrorInvalidArgs);
	}

	if((error = hny_shift(hny, cmdargv[0], package)) == HnyErrorNone) {
		print("%s shifted for %s\n", cmdargv[0], cmdargv[1]);
	} else {
		print_error("Unable to shift Honey package\n");
		out(error);
	}

	hny_free_geister(package, 1);
}

static void
list(int cmdargc,
	char **cmdargv) {
	enum hny_error retval = HnyErrorNone;
	enum hny_listing list;

	if(cmdargc == 1) {
		if(strcmp("active", cmdargv[0]) == 0) {
			list = HnyListActive;
		} else if(strcmp("packages", cmdargv[0]) == 0) {
			list = HnyListPackages;
		} else {
			print_error("Invalid argument for listing \"%s\"\n", cmdargv[0]);
			usage();
		}
	} else {
		print_error("Invalid number of arguments for listing\n");
		usage();
	}

	if(retval == HnyErrorNone) {
		struct hny_geist *geister;
		size_t len;

		if(hny_list(hny, list, &geister, &len) == HnyErrorNone) {
			struct hny_geist *iterator = geister,
				*end = &geister[len];

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
			retval = HnyErrorUnavailable;
		}
	}

	out(retval);
}

static void
erase(int cmdargc,
	char **cmdargv) {
	enum hny_error retval = HnyErrorNone;
	struct hny_geist *geister
		= hny_alloc_geister((const char **)cmdargv, cmdargc);

	if(geister == NULL) {
		print_error("Invalid geist name\n");
		retval = HnyErrorInvalidArgs;
	} else {
		int i;

		for(i = 0; i < cmdargc; i++) {
			switch(hny_erase(hny, &geister[i])) {
			case HnyErrorNone:
				if(geister[i].version == NULL) {
					print("Geist %s unlinked\n", geister[i].name);
				} else {
					print("Package %s-%s removed\n",
						geister[i].name, geister[i].version);
				}
				break;
			case HnyErrorUnavailable:
				print_error("Unable to erase %s, prefix is unavailable\n",
					cmdargv[i]);
				retval = HnyErrorUnavailable;
				break;
			case HnyErrorNonExistant:
				print_error("Unable to erase %s, a needed resource is missing\n",
					cmdargv[i]);
				retval = HnyErrorNonExistant;
				break;
			case HnyErrorUnauthorized:
				print_error("Unable to erase %s, unauthorized\n",
					cmdargv[i]);
				retval = HnyErrorUnauthorized;
				break;
			default:
				print_error("Unable to erase %s\n",
					cmdargv[i]);
				retval = HnyErrorInvalidArgs;
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
	enum hny_error retval = HnyErrorNone;
	struct hny_geist *geister
		= hny_alloc_geister((const char **)cmdargv, cmdargc);

	if(geister == NULL) {
		print_error("Invalid geist name\n");
		retval = HnyErrorInvalidArgs;
	} else {
		int i;

		for(i = 0; i < cmdargc; i++) {
			struct hny_geist *target;

			switch(hny_status(hny, &geister[i], &target)) {
			case HnyErrorNone:
				print("%s-%s\n", target->name, target->version);
				hny_free_geister(target, 1);
				break;
			case HnyErrorUnavailable:
				print_error("Unable to status %s: prefix is unavailable\n",
					cmdargv[i]);
				retval = HnyErrorUnavailable;
				break;
			case HnyErrorNonExistant:
				print_error("Unable to status %s: the geist doesn't have a target\n",
					cmdargv[i]);
				retval = HnyErrorNonExistant;
				break;
			case HnyErrorUnauthorized:
				print_error("Unable to status %s: unauthorized\n",
					cmdargv[i]);
				retval = HnyErrorUnauthorized;
				break;
			default:
				print_error("Unable to status %s\n",
					cmdargv[i]);
				retval = HnyErrorInvalidArgs;
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
	enum hny_error retval = HnyErrorNone;
	struct hny_geist *geister
		= hny_alloc_geister((const char **)cmdargv, cmdargc);

	if(geister == NULL) {
		print_error("Invalid geist name\n");
		retval = HnyErrorInvalidArgs;
	} else {
		int i;

		for(i = 0; i < cmdargc; i++) {
			switch(hny_execute(hny, action, &geister[i])) {
			case HnyErrorNone:
				break;
			case HnyErrorUnavailable:
				print_error("Missing resource to execute action for %s\n",
					cmdargv[i]);
				retval = HnyErrorUnavailable;
				break;
			case HnyErrorNonExistant:
				print_error("Non existant resource to execute action for %s\n",
					cmdargv[i]);
				retval = HnyErrorNonExistant;
				break;
			case HnyErrorUnauthorized:
				print_error("Unauthorized to execute action for %s\n",
					cmdargv[i]);
				retval = HnyErrorUnauthorized;
				break;
			default:
				print_error("Invalid arguments to execute action for %s\n",
					cmdargv[i]);
				retval = HnyErrorInvalidArgs;
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
	int block = HNY_BLOCK;

	while(argc >= 2
		&& argv[1][0] == '-') {

		if(strcmp("--accepted-eulas", argv[1]) == 0
			|| strcmp("-a", argv[1]) == 0) {
			hny_already_accepted = 1;
		} else if(strcmp("--non-blocking", argv[1]) == 0) {
			block = HNY_NONBLOCK;
		} else if(strncmp("--prefix=", argv[1], 9) == 0) {
			prefix = &argv[1][9];
		} else {
			if(strcmp("--help", argv[1]) != 0) {
				print_error("Invalid argument \"%s\"\n", argv[1]);
			}
			usage();
		}

		argc--;
		argv++;
	}

	if(prefix == NULL
		|| (hny = hny_create(prefix)) == NULL) {
		print_error("Unable to access prefix %s\n", prefix);
		out(HnyErrorUnavailable);
	}

	hny_locking(hny, block);

	if(argc >= 2) {
		int cmdargc = argc - 2;
		char **cmdargv = &argv[2];
		char *cmd = argv[1];

		if(strcmp("verify", cmd) == 0) {
			verify(cmdargc, cmdargv);
		} else if(strcmp("export", cmd) == 0) {
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
			execute(HnyActionSetup, cmdargc, cmdargv);
		} else if(strcmp("clean", cmd) == 0) {
			execute(HnyActionClean, cmdargc, cmdargv);
		} else if(strcmp("reset", cmd) == 0) {
			execute(HnyActionReset, cmdargc, cmdargv);
		} else if(strcmp("check", cmd) == 0) {
			execute(HnyActionCheck, cmdargc, cmdargv);
		} else if(strcmp("purge", cmd) == 0) {
			execute(HnyActionPurge, cmdargc, cmdargv);
		} else {
			print_error("Invalid command \"%s\"\n", cmd);
			usage();
		}
	} else {
		usage();
	}

	out(HnyErrorNone);
}
