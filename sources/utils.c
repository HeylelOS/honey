/*
	utils.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

void hny_free_geister(struct hny_geist *geister, size_t count) {
	size_t i;

	for(i = 0; i < count; i++) {
		/* Name and version should share the same allocation */
		free(geister[i].name);
	}

	free(geister);
}

_Bool hny_equals_geister(const struct hny_geist *g1, const struct hny_geist *g2) {
	if(hny_check_geister(g1, 1) == HnyErrorNone
		&& hny_check_geister(g2, 1) == HnyErrorNone) {
		if(strcmp(g1->name, g2->name) == 0) {
			if(g1->version != NULL
				&& g2->version != NULL) {
				return (strcmp(g1->version, g2->version) == 0);
			} else if(g1->version == NULL
				&& g2->version == NULL) {
				return 1;
			}
		}
	}

	return 0;
}

enum hny_error hny_check_geister(const struct hny_geist *geister, size_t n) {
	size_t i = 0;

	if(geister == NULL || n == 0) {
		return HnyErrorInvalidArgs;
	}

	while(i < n
		&& geister[i].name != NULL
		&& *(geister[i].name) != '\0'
		&& *(geister[i].name) != '.'
		&& strchr(geister[i].name, '-') == NULL
		&& strchr(geister[i].name, '/') == NULL) {
		i++;
	}

	return i == n ? HnyErrorNone : HnyErrorInvalidArgs;
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

/* INTERNAL UTILS */

enum hny_error hny_errno(int err) {
	switch(err) {
		case EBADF:
		case ENOENT:
			return HnyErrorNonExistant;
		case EAGAIN:
		case ENFILE:
		case ENOMEM:
			return HnyErrorUnavailable;
		case ELOOP:
		case ENAMETOOLONG:
		case ENOTEMPTY:
			return HnyErrorInvalidArgs;
		case EACCES:
		case EPERM:
			return HnyErrorUnauthorized;
		default:
			break;
	}

	return HnyErrorNone;
}

