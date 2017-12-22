#include <hny.h>

/* snprintf requires _BSD_SOURCE */
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#include <unistd.h>

#include <errno.h>

struct hny_hive {
	char sem_name[32];
	sem_t *semaphore;
};

static struct hny_hive *hive;

int hny_connect(int flags) {
	int wait_ret;

	if(hive == NULL) {
		if((hive = malloc(sizeof(*hive))) == NULL)
			return HNY_ERROR_UNAVAILABLE;
	}

	snprintf(hive->sem_name, sizeof(hive->sem_name),
		"hny-%u-sem", geteuid());
	/* effective user id because the semaphore gets ownership from
		effective user id */

	printf("%s\n", hive->sem_name);

	hive->semaphore = sem_open(hive->sem_name, O_CREAT, S_IRUSR | S_IWUSR, 1);
	if(hive->semaphore == SEM_FAILED) {
		free(hive);
		hive = NULL;
		perror("hny error sem_open");
		return HNY_ERROR_UNAVAILABLE;
	}

	if(flags & HNY_CONNECT_WAIT) {
		printf("Waiting for semaphore\n");
		wait_ret = sem_wait(hive->semaphore);
	} else {
		printf("Not waiting for semaphore\n");
		wait_ret = sem_trywait(hive->semaphore);
	}

	if(wait_ret == -1) {
		if(errno != EAGAIN)
			perror("hny error sem_open");
		return HNY_ERROR_UNAVAILABLE;
	}

	return HNY_OK;
}

size_t hny_fetch(const char *name, const char *version, struct hny_package **packages, int flags) {
	if(name == NULL
		|| packages == NULL
		|| !(flags & HNY_STANDARD
			|| flags & HNY_GEIST)) {
		return 0;
	}

	return 0;
}

int hny_compare_versions(const char **p1, const char **p2) {
	return 0;
}

void hny_disconnect() {
	if(hive != NULL) {
		printf("Disconnecting\n");

		sem_post(hive->semaphore);
		sem_close(hive->semaphore);

		free(hive);
		hive = NULL;
	}
}

