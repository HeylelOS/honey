/*
	main.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include <hny.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static hny_t hny;

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

	hny_destroy(hny);
	exit(error);
}

static void
usage(void) {

	print_error("usage: hny <command> <args>\n");
	out(HnyErrorInvalidArgs);
}

static void
verify(int cmdargc,
	char **cmdargv) {
	enum hny_error error;
	char **iterator = cmdargv;
	char **end = &cmdargv[cmdargc];

	while(iterator != end) {
		char *file = *iterator;
		char *eula;
		size_t len;

		if((error = hny_verify(hny, file, &eula, &len)) == HnyErrorNone) {
			print("EULA:\n%*s\n", len, eula);

			free(eula);
		} else {
			print_error("File %s is an invalid Honey package\n", file);
		}

		iterator++;
	}
}

static void
export(int cmdargc,
	char **cmdargv) {
	enum hny_error error;
	struct hny_geist *package;

	if(cmdargc != 2
		|| (package = hny_alloc_geister((const char **)&cmdargv[1], 1)) == NULL) {
		print_error("expected: hny export [package file] [package name]\n");
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
		print_error("expected: hny shift [target geist] [source geist]\n");
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
}

static void
erase(int cmdargc,
	char **cmdargv) {
}

static void
status(int cmdargc,
	char **cmdargv) {
}

static void
execute(enum hny_action action,
	int cmdargc,
	char **cmdargv) {
	struct hny_geist *geister
		= hny_alloc_geister((const char **)cmdargv, cmdargc);
	int i;

	if(geister == NULL) {
		print_error("Invalid geist name\n");
		out(HnyErrorInvalidArgs);
	}

	for(i = 0; i < cmdargc; i++) {
		switch(hny_execute(hny, action, &geister[i])) {
		case HnyErrorInvalidArgs:
			print_error("Invalid arguments to execute action for %s-%s\n",
				geister[i].name, geister[i].version);
			break;
		case HnyErrorUnavailable:
			print_error("Missing resource to execute action for %s-%s\n",
				geister[i].name, geister[i].version);
			break;
		default:
			break;
		}
	}

	hny_free_geister(geister, cmdargc);
}

int
main(int argc,
	char **argv) {
	char *prefix = getenv("HNY_PREFIX");

	if(prefix == NULL
		|| (hny = hny_create(prefix)) == NULL) {
		print_error("Unable to access prefix %s\n", prefix);
		exit(HnyErrorUnavailable);
	}

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
			print_error("Invalid command:\n");
			usage();
		}
	} else {
		usage();
	}

	out(HnyErrorNone);
}
