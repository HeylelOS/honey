#include <hny.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void fatal(const char *format, ...) {
	va_list args;
	va_start(args, format);

	vfprintf(stderr, format, args);
	va_end(args);

	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	int i = 1;

	if(argc < 2) {
		fatal("error: expected action\n");
	}

	if(strcmp(argv[i], "fetch") == 0) {
		i++;
		if(i != argc) {
			for(; i < argc; i++) {
				printf("fetch package %s\n", argv[i]);
			}
		} else {
			fatal("error: %s fetch [packages names...]\n", argv[0]);
		}
	} else if(strcmp(argv[i], "install") == 0) {
		i++;
		if(i != argc) {
			for(; i < argc; i++) {
				printf("install file %s\n", argv[i]);
			}
		} else {
			fatal("error: %s install [packages files...]\n", argv[0]);
		}
	} else if(strcmp(argv[i], "list") == 0) {
		i++;
		if(i != argc) {
			if(strcmp(argv[i], "providers") == 0) {
				printf("list providers\n");
			} else if(strcmp(argv[i], "all") == 0) {
				printf("list all\n");
			} else if(strcmp(argv[i], "active") == 0) {
				printf("list active\n");
			} else {
				fatal("error: %s list, \"%s\" is invalid\n", argv[0], argv[i]);
			}
		} else {
			fatal("error: %s list [providers | all | active]\n", argv[0]);
		}
	} else if(strcmp(argv[i], "remove") == 0) {
		i++;
		if(i != argc) {
			if(strcmp(argv[i], "total") == 0) {
				printf("remove total\n");
			} else if(strcmp(argv[i], "partial") == 0) {
				printf("remove partial\n");
			} else {
				fatal("error: %s remove, \"%s\" is invalid\n", argv[0], argv[i]);
			}
		} else {
			fatal("error: %s remove [total | partial]\n", argv[0]);
		}
	} else if(strcmp(argv[i], "status") == 0) {
		if(++i != argc) {
			for(; i < argc; i++) {
				printf("status package %s\n", argv[i]);
			}
		} else {
			fatal("error: %s install [packages files...]\n", argv[0]);
		}
	} else if(strcmp(argv[i], "repair") == 0) {
		i++;
		if(i != argc) {
			if(strcmp(argv[i], "all") == 0) {
				printf("repair all\n");
			} else if(strcmp(argv[i], "clean") == 0) {
				printf("repair clean\n");
			} else if(strcmp(argv[i], "check") == 0) {
				printf("repair check\n");
			} else if(strcmp(argv[i], "config") == 0) {
				printf("repair config\n");
			} else {
				fatal("error: %s remove, \"%s\" is invalid\n", argv[0], argv[i]);
			}
		} else {
			fatal("error: %s repair [all | clean | check | config]\n", argv[0]);
		}
	}

	atexit(hny_disconnect);

	/* if(hny_connect(HNY_CONNECT_WAIT) == HNY_ERROR_UNAVAILABLE) { */
	if(hny_connect(0) == HNY_ERROR_UNAVAILABLE) {
		fatal("hny error: unable to connect\n");
	}

	exit(EXIT_SUCCESS);
}
