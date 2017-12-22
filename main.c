#include <hny.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	atexit(hny_disconnect);

	/* if(hny_connect(HNY_CONNECT_WAIT) == HNY_ERROR_UNAVAILABLE) { */
	if(hny_connect(0) == HNY_ERROR_UNAVAILABLE) {
		fprintf(stderr, "hny error: unable to connect\n");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
