#include <hny.h>

/* snprintf requires _BSD_SOURCE */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
		if((hive = malloc(sizeof(*hive))) == NULL) {
			return HNY_ERROR_UNAVAILABLE;
		}
	}

	snprintf(hive->sem_name, sizeof(hive->sem_name),
		"hny-%u-sem", geteuid());
	/* effective user id because the semaphore gets ownership from
		effective user id */

	hive->semaphore = sem_open(hive->sem_name, O_CREAT, S_IRUSR | S_IWUSR, 1);
	if(hive->semaphore == SEM_FAILED) {
		free(hive);
		hive = NULL;

		return HNY_ERROR_UNAVAILABLE;
	}

	if(flags & HNY_CONNECT_WAIT) {
		wait_ret = sem_wait(hive->semaphore);
	} else {
		wait_ret = sem_trywait(hive->semaphore);
	}

	if(wait_ret == -1) {
		if(errno != EAGAIN) {
			perror("hny error sem_open");
		}

		return HNY_ERROR_UNAVAILABLE;
	}

	return HNY_OK;
}

void hny_disconnect() {
	if(hive != NULL) {
		sem_post(hive->semaphore);
		sem_close(hive->semaphore);

		free(hive);
		hive = NULL;
	}
}

int hny_provider_add(const char *url, int flags) {
	return HNY_OK;
}

int hny_fetch_list(struct hny_geist geist, struct hny_geist **packages, size_t *fetched, int flags) {
	return HNY_OK;
}

int hny_check_geister(struct hny_geist *geister, size_t n) {
	size_t i = 0;

	if(geister == NULL
		|| n == 0)
		return HNY_ERROR_INVALIDARGS;

	while(i < n
		&& geister[i].name != NULL
		&& *(geister[i].name) != '\0'
		&& strchr(geister[i].name, '-') == NULL) {
		i++;
	}

	return i == n ? HNY_OK : HNY_ERROR_INVALIDARGS;
}

int hny_compare_versions(const char **p1, const char **p2) {
	unsigned long m1, m2;
	char *endptr1, *endptr2;

	m1 = strtoul(*p1, &endptr1, 10);
	m2 = strtoul(*p2, &endptr2, 10);

	if(m1 == m2
		&& (*endptr1 == '.' && *endptr2 == '.')) {
		m1 = strtoul(++endptr1, NULL, 10);
		m2 = strtoul(++endptr2, NULL, 10);
	}

	return m1 - m2;
}

