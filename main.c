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
#include <stdarg.h>
#include <signal.h>

void honey_fatal(const char *format, ...) {
	va_list args;
	va_start(args, format);

	vfprintf(stderr, format, args);
	va_end(args);

	exit(EXIT_FAILURE);
}

void signal_exit(int signal) {
	honey_fatal("Exited on signal %d\n", signal);
}

void setup_sigexits(void) {
	struct sigaction sa;

	sigfillset(&sa.sa_mask);
	sa.sa_handler = signal_exit;

	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGTRAP, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGALRM, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
}

void honey_fetch(int count, char **names) {
	int i;

	for(i = 0; i < count; i++) {
		printf("fetch package %s\n", names[i]);
	}
}

void honey_shift(char *geist, char *replacement) {
	struct hny_geist repl;
	repl.name = strsep(&replacement, "-");
	repl.version = replacement;

	switch(hny_shift(geist, &repl)) {
		case HnyErrorNone:
			exit(EXIT_SUCCESS);
		case HnyErrorNonExistant:
			honey_fatal("honey: unable to shift %s to %s-%s, file doesn't exist\n",
				geist, repl.name, repl.version);
		case HnyErrorUnauthorized:
			honey_fatal("honey: unauthorized to shift %s to %s-%s\n",
				geist, repl.name, repl.version);
		case HnyErrorUnavailable:
			honey_fatal("honey: unable to shift %s to %s-%s, a ressource isn't available\n",
				geist, repl.name, repl.version);
		case HnyErrorInvalidArgs:
			honey_fatal("honey: unable to shift %s to %s-%s, an argument is invalid\n",
				geist, repl.name, repl.version);
		default:
			honey_fatal("honey: unable to shift %s to %s-%s, unknown error\n",
				geist, repl.name, repl.version);
	}
}

void honey_install(int count, char **files) {
	int i;

	for(i = 0; i < count; i++) {
		hny_install(files[i]);
	}
}

void honey_list(char *method) {
	enum hny_listing listing = HnyListAll;
	struct hny_geist *list;
	size_t i, count;

	if(strcmp(method, "all") == 0) {
		/* Default declared All */
	} else if(strcmp(method, "active") == 0) {
		listing = HnyListActive;
	} else if(strcmp(method, "links") == 0) {
		listing = HnyListLinks;
	} else {
		honey_fatal("error: list, \"%s\" is invalid\n", method);
	}

	list = hny_list(listing, &count);

	for(i = 0; i < count; i++) {
		if(list[i].version != NULL) {
			printf("%s-%s\n", list[i].name, list[i].version);
		} else {
			printf("%s\n", list[i].name);
		}
	}

	hny_free_geister(list, count);
}

void honey_remove(char *method, int count, char **names) {
	int i;

	if(strcmp(method, "total") == 0) {
		printf("remove total\n");
	} else if(strcmp(method, "data") == 0) {
		printf("remove data\n");
	} else if(strcmp(method, "links") == 0) {
		printf("remove links\n");
	} else {
		honey_fatal("error: hny remove, \"%s\" is invalid\n", method);
	}

	for(i = 0; i < count; i++) {
		printf("remove %s package %s\n", method, names[i]);
	}
}

void honey_status(int count, char **names) {
	int i;

	for(i = 0; i < count; i++) {
		printf("status package %s\n", names[i]);
	}
}

void honey_repair(char *method, int count, char **names) {
	int i;

	if(strcmp(method, "all") == 0) {
		printf("repair all\n");
	} else if(strcmp(method, "clean") == 0) {
		printf("repair clean\n");
	} else if(strcmp(method, "check") == 0) {
		printf("repair check\n");
	} else if(strcmp(method, "config") == 0) {
		printf("repair config\n");
	} else {
		honey_fatal("error: repair, \"%s\" is invalid\n", method);
	}

	for(i = 0; i < count; i++) {
		printf("repair %s package %s\n", method, names[i]);
	}
}

int main(int argc, char **argv) {
	int i = 1;

	setup_sigexits();

	atexit(hny_disconnect);
	/* if(hny_connect(HNY_CONNECT_WAIT) == HNY_ERROR_UNAVAILABLE) { */
	if(hny_connect(0) == HnyErrorUnavailable) {
		honey_fatal("hny error: unable to connect\n");
	}

	if(argc < 2) {
		honey_fatal("error: expected action\n");
	}

	if(strcmp(argv[i], "fetch") == 0) {
		if(++i != argc) {
			honey_fetch(argc - i, &argv[i]);
		} else {
			honey_fatal("error: %s fetch [packages names...]\n", argv[0]);
		}
	} else if(strcmp(argv[i], "shift") == 0) {
		if(++i == argc - 2) {
			honey_shift(argv[i], argv[i + 1]);
		} else {
			honey_fatal("error: %s shift [geist name] [replacement name]\n", argv[0]);
		}
	} else if(strcmp(argv[i], "install") == 0) {
		if(++i != argc) {
			honey_install(argc - i, &argv[i]);
		} else {
			honey_fatal("error: %s install [packages files...]\n", argv[0]);
		}
	} else if(strcmp(argv[i], "list") == 0) {
		if(++i == argc - 1) {
			honey_list(argv[i]);
		} else {
			honey_fatal("error: %s list [all | active | links]\n", argv[0]);
		}
	} else if(strcmp(argv[i], "remove") == 0) {
		if(++i < argc - 1) {
			honey_remove(argv[i], argc - i - 1, &argv[i + 1]);
		} else {
			honey_fatal("error: %s remove [total | data | links] [packages names...]\n", argv[0]);
		}
	} else if(strcmp(argv[i], "status") == 0) {
		if(++i != argc) {
			honey_status(argc - i, &argv[i]);
		} else {
			honey_fatal("error: %s status [packages files...]\n", argv[0]);
		}
	} else if(strcmp(argv[i], "repair") == 0) {
		if(++i < argc - 1) {
			honey_repair(argv[i], argc - i - 1, &argv[i + 1]);
		} else {
			honey_fatal("error: %s repair [all | clean | check | config] [packages names...]\n", argv[0]);
		}
	}

	exit(EXIT_SUCCESS);
}
