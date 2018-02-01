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
	sa.sa_flags = 0;
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

_Bool honey_eula(const char *text, size_t bufsize) {
	char answer;
	printf("Do you agree the following license?:\n%.*s\n", (int)bufsize, text);

	do {
		printf("\r[y/n] ");
		scanf("%c", &answer);
	} while(answer != 'y'
		&& answer != 'n');

	return answer == 'y';
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
			break;
		case HnyErrorNonExistant:
			honey_fatal("honey: unable to shift %s to %s-%s, a file is missing\n",
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
		switch(hny_install(files[i], honey_eula)) {
			case HnyErrorNone:
				printf("package %s successfully installed\n", files[i]);
				break;
			case HnyErrorNonExistant:
				fprintf(stderr, "honey: unable to install %s, a file is missing\n", files[i]);
				break;
			case HnyErrorUnauthorized:
				fprintf(stderr, "honey: unable to install %s, unauthorized\n", files[i]);
				break;
			case HnyErrorUnavailable:
				fprintf(stderr, "honey: unable to install %s, the system lacked of ressources\n", files[i]);
				break;
			case HnyErrorInvalidArgs:
				fprintf(stderr, "honey: unable to install %s, the archive doesn't exist or isn't valid\n", files[i]);
				break;
			default:
				fprintf(stderr, "honey: unable to install %s, unknown error\n", files[i]);
				break;
		}
	}
}

void honey_list(char *method) {
	enum hny_listing listing = HnyListPackages;
	struct hny_geist *list;
	size_t i, count;

	if(strcmp(method, "packages") == 0) {
		/* Default */
	} else if(strcmp(method, "active") == 0) {
		listing = HnyListActive;
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

void honey_remove(int count, char **names) {
	struct hny_geist geist;
	int i;

	for(i = 0; i < count; i++) {
		geist.name = strsep(&names[i], "-");
		geist.version = names[i];

		switch(hny_remove(&geist)) {
			case HnyErrorNone:
				break;
			case HnyErrorNonExistant:
				honey_fatal("honey: unable to remove %s-%s, invalid file\n",
					geist.name, geist.version);
			case HnyErrorUnauthorized:
				honey_fatal("honey: unauthorized to remove %s-%s\n",
					geist.name, geist.version);
			case HnyErrorUnavailable:
				honey_fatal("honey: unable to remove %s-%s, a ressource isn't available\n",
					geist.name, geist.version);
			case HnyErrorInvalidArgs:
				honey_fatal("honey: unable to remove %s-%s, an argument is invalid\n",
					geist.name, geist.version);
			default:
				honey_fatal("honey: unable to remove %s-%s, unknown error\n",
					geist.name, geist.version);
		}
	}
}

void honey_status(char *geist) {
	struct hny_geist *target, source;

	source.name = strsep(&geist, "-");
	source.version = geist;

	target = hny_status(&source);

	if(target != NULL) {
		if(target->version != NULL) {
			printf("%s-%s\n", target->name, target->version);
		} else {
			printf("%s\n", target->name);
		}

		hny_free_geister(target, 1);
	} else {
		exit(EXIT_FAILURE);
	}
}

void honey_execute(char *method, int count, char **names) {
	enum hny_action action = HnyActionSetup;
	struct hny_geist geist;
	int i;

	if(strcmp(method, "setup") == 0) {
		/* Default */
	} else if(strcmp(method, "drain") == 0) {
		action = HnyActionDrain;
	} else if(strcmp(method, "reset") == 0) {
		action = HnyActionReset;
	} else if(strcmp(method, "check") == 0) {
		action = HnyActionCheck;
	} else if(strcmp(method, "clean") == 0) {
		action = HnyActionClean;
	} else {
		honey_fatal("error: execute, \"%s\" is invalid\n", method);
	}

	for(i = 0; i < count; i++) {
		geist.name = strsep(&names[i], "-");
		geist.version = names[i];

		hny_execute(action, &geist);
	}
}

int main(int argc, char **argv) {
	int i = 1;

	setup_sigexits();

	atexit(hny_disconnect);

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
			honey_fatal("error: %s list [packages | active]\n", argv[0]);
		}
	} else if(strcmp(argv[i], "remove") == 0) {
		if(++i != argc) {
			honey_remove(argc - i, &argv[i]);
		} else {
			honey_fatal("error: %s remove [packages names...]\n", argv[0]);
		}
	} else if(strcmp(argv[i], "status") == 0) {
		if(++i == argc - 1) {
			honey_status(argv[i]);
		} else {
			honey_fatal("error: %s status [geist name]\n", argv[0]);
		}
	} else if(strcmp(argv[i], "execute") == 0) {
		if(++i < argc - 1) {
			honey_execute(argv[i], argc - i - 1, &argv[i + 1]);
		} else {
			honey_fatal("error: %s execute [setup | drain | reset | check | clean] [geister names...]\n", argv[0]);
		}
	} else {
		honey_fatal("error: expected action\n");
	}

	exit(EXIT_SUCCESS);
}
