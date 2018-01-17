#include <hny.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void honey_fatal(const char *format, ...) {
	va_list args;
	va_start(args, format);

	vfprintf(stderr, format, args);
	va_end(args);

	exit(EXIT_FAILURE);
}

void honey_fetch(int count, char **names) {
	int i;

	for(i = 0; i < count; i++) {
		printf("fetch package %s\n", names[i]);
	}
}

void honey_install(int count, char **files) {
	int i;

	for(i = 0; i < count; i++) {
		printf("install file %s\n", files[i]);
	}
}

void honey_list(char *method) {
	if(strcmp(method, "providers") == 0) {
		printf("list providers\n");
	} else if(strcmp(method, "all") == 0) {
		printf("list all\n");
	} else if(strcmp(method, "active") == 0) {
		printf("list active\n");
	} else {
		honey_fatal("error: list, \"%s\" is invalid\n", method);
	}
}

void honey_remove(char *method, int count, char **names) {
	int i;

	if(strcmp(method, "total") == 0) {
		printf("remove total\n");
	} else if(strcmp(method, "partial") == 0) {
		printf("remove partial\n");
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
	atexit(hny_disconnect);

	/* if(hny_connect(HNY_CONNECT_WAIT) == HNY_ERROR_UNAVAILABLE) { */
	if(hny_connect(0) == HNY_ERROR_UNAVAILABLE) {
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
			honey_fatal("error: %s list [providers | all | active]\n", argv[0]);
		}
	} else if(strcmp(argv[i], "remove") == 0) {
		if(++i < argc - 1) {
			honey_remove(argv[i], argc - i - 1, &argv[i + 1]);
		} else {
			honey_fatal("error: %s remove [total | partial] [packages names...]\n", argv[0]);
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
