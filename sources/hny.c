/*
	hny.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include <hny.h>

/* snprintf requires _BSD_SOURCE */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/param.h>	/* MAXPATHLEN */
#include <string.h>
#include <sys/dir.h>

struct hny_hive {
	char sem_name[32];
	sem_t *semaphore;
	pthread_mutex_t mutex;

	char *installdir;
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

	hive->installdir = malloc(MAXPATHLEN);
	hive->installdir = getcwd(hive->installdir, MAXPATHLEN);
	snprintf(hive->installdir, MAXPATHLEN,
		"%s/%s", hive->installdir, "packages/");
	hive->installdir = realloc(hive->installdir, strlen(hive->installdir));

	pthread_mutex_init(&hive->mutex, NULL);

	return HNY_OK;
}

void hny_disconnect() {
	if(hive != NULL) {
		pthread_mutex_destroy(&hive->mutex);

		sem_post(hive->semaphore);
		sem_close(hive->semaphore);

		free(hive->installdir);
		free(hive);
		hive = NULL;
	}
}

int hny_install(const char *file) {
	pthread_mutex_lock(&hive->mutex);

	printf("Installing %s in %s\n", file, hive->installdir);

	pthread_mutex_unlock(&hive->mutex);
	return HNY_OK;
}

int hny_shift(const char *geist, const struct hny_geist *package) {
	int retval = HNY_OK;
	DIR *dirp;

	if(hny_check_geister(package, 1) != HNY_OK) {
		return HNY_ERROR_INVALIDARGS;
	}

	pthread_mutex_lock(&hive->mutex);

	if((dirp = opendir(hive->installdir)) != NULL) {
		struct stat st;
		char name[NAME_MAX];

		if(package->version != NULL) {
			snprintf(name, NAME_MAX,
				"%s-%s", package->name, package->version);
		} else {
			strcpy(name, package->name);
		}

		/* We follow symlink, this is only to check if the file exists */
		if(fstatat(dirfd(dirp), name, &st, 0) == 0) {
			/* We check if the target exists, if it does, we unlink it */
			if((fstatat(dirfd(dirp), geist, &st, 0) == 0)
				&& (unlinkat(dirfd(dirp), geist, 0) == -1)) {
				switch(errno) {
					case ENOENT:
						retval = HNY_ERROR_NONEXISTANT;
						break;
					case EACCES:
						/* fallthrough */
					case EPERM:
						/* fallthrough */
					default:
						retval = HNY_ERROR_UNAUTHORIZED;
						break;
				}
			}

			if(retval == HNY_OK) {
				if(symlinkat(name, dirfd(dirp), geist) == -1) {
					switch(errno) {
						case ENOENT:
							retval = HNY_ERROR_NONEXISTANT;
							break;
						case EACCES:
							/* fallthrough */
						default:
							retval = HNY_ERROR_UNAUTHORIZED;
							break;
					}
				}
			}
		} else {
			switch(errno) {
				case ENOENT:
					retval = HNY_ERROR_NONEXISTANT;
					break;
				case EACCES:
					/* fallthrough */
				default:
					retval = HNY_ERROR_UNAUTHORIZED;
					break;
			}
		}

		closedir(dirp);
	}

	pthread_mutex_unlock(&hive->mutex);

	return retval;
}

struct hny_geist *hny_list(enum hny_listing listing, size_t *listed) {
	struct hny_geist *list = NULL;
	size_t alloced = 0;
	DIR *dirp;
	struct dirent *entry;
	*listed = 0;

	pthread_mutex_lock(&hive->mutex);

	if((dirp = opendir(hive->installdir)) != NULL) {
		while((entry = readdir(dirp)) != NULL) {
			if(entry->d_name[0] != '.') {
				char *stringp;

				if(*listed == alloced) {
					alloced += 16;
					list = realloc(list, sizeof(*list) * alloced);
				}

				if(listing == HnyListActive
					&& entry->d_type == DT_LNK) {
					ssize_t length;
					stringp = malloc(NAME_MAX);

					length = readlinkat(dirfd(dirp), entry->d_name, stringp, NAME_MAX);

					if(length != -1) {
						stringp[length] = '\0';
						stringp = realloc(stringp, length + 1);

						list[*listed].name = strsep(&stringp, "-");
						list[*listed].version = stringp;

						(*listed)++;
					} else {
						perror("hny");
					}
				} else if(listing == HnyListAll
					&& entry->d_type == DT_DIR) {
					stringp = strdup(entry->d_name);

					list[*listed].name = strsep(&stringp, "-");
					if(stringp != NULL) {
						list[*listed].version = stringp;

						(*listed)++;
					} else {
						free(stringp);
					}
				} else if(listing == HnyListLinks
					&& entry->d_type == DT_LNK
					&& strchr(entry->d_name, '-') == NULL) {
					list[*listed].name = strdup(entry->d_name);
					list[*listed].version = NULL;

					(*listed)++;
				}
			}
		}

		list = realloc(list, sizeof(*list) * (*listed));

		closedir(dirp);
	}

	pthread_mutex_unlock(&hive->mutex);

	return list;
}

int hny_remove(enum hny_removal removal, const struct hny_geist *geister, size_t count) {
	if(hny_check_geister(geister, count) != HNY_OK) {
		return HNY_ERROR_INVALIDARGS;
	}

	pthread_mutex_lock(&hive->mutex);

	switch(removal) {
		case HnyRemoveTotal:
			break;
		case HnyRemoveData:
			break;
		case HnyRemoveLinks:
			break;
		default:
			break;
	}

	pthread_mutex_unlock(&hive->mutex);
	return HNY_OK;
}

int hny_status(const struct hny_geist *geist) {
	if(hny_check_geister(geist, 1) != HNY_OK) {
		return HNY_ERROR_INVALIDARGS;
	}

	pthread_mutex_lock(&hive->mutex);
	pthread_mutex_unlock(&hive->mutex);

	return HNY_OK;
}

int hny_repair(const struct hny_geist *geist, int flags) {
	if(hny_check_geister(geist, 1) != HNY_OK) {
		return HNY_ERROR_INVALIDARGS;
	}

	pthread_mutex_lock(&hive->mutex);
	pthread_mutex_unlock(&hive->mutex);

	return HNY_OK;
}

/* UTILITIES */

void hny_free_geister(struct hny_geist *geister, size_t count) {
	size_t i;

	for(i = 0; i < count; i++) {
		/* Name and version should share the same allocation */
		free(geister[i].name);
	}

	free(geister);
}

int hny_check_geister(const struct hny_geist *geister, size_t n) {
	size_t i = 0;

	if(geister == NULL || n == 0) {
		return HNY_ERROR_INVALIDARGS;
	}

	while(i < n
		&& geister[i].name != NULL
		&& *(geister[i].name) != '\0'
		&& *(geister[i].name) != '.'
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

