/*
	utils.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

#include <stdlib.h>
#include <string.h>

struct hny_geist *
hny_alloc_geister(const char **names,
	hny_size_t count) {
	struct hny_geist *list = NULL;
	size_t i = 0;

	while(i < count
		&& hny_check_package(names[i]) == HNY_ERROR_NONE) {
		++i;
	}

	if(i == count) {
		char *stringp;

		list = malloc(count * sizeof(*list));

		for(i = 0; i < count; ++i) {
			stringp = strdup(names[i]);

			list[i].name = strsep(&stringp, "-");
			list[i].version = stringp;
		}
	}

	return list;
}

void
hny_free_geister(struct hny_geist *geister,
	hny_size_t count) {
	size_t i;

	for(i = 0; i < count; ++i) {
		/* Name and version should share the same allocation */
		free(geister[i].name);
	}

	free(geister);
}

enum hny_error
hny_equals_geister(const struct hny_geist *g1,
	const struct hny_geist *g2) {

	if(hny_check_geister(g1, 1) == HNY_ERROR_NONE
		&& hny_check_geister(g2, 1) == HNY_ERROR_NONE) {
		if(strcmp(g1->name, g2->name) == 0) {
			if(g1->version != NULL
				&& g2->version != NULL
				&& strcmp(g1->version, g2->version) == 0) {

				return HNY_ERROR_NONE;
			} else if(g1->version == NULL
				&& g2->version == NULL) {

				return HNY_ERROR_NONE;
			}
		}
	}

	return HNY_ERROR_INVALID_ARGS;
}

enum hny_error
hny_check_name(const char *name) {
	enum hny_error retval = HNY_ERROR_INVALID_ARGS;

	if(name != NULL
		&& *name != '\0'
		&& *name != '.') {
		while(*name != '\0'
			&& *name != '/'
			&& *name != '-') {
			++name;
		}

		if(*name == '\0') {
			retval = HNY_ERROR_NONE;
		}
	}

	return retval;
}

enum hny_error
hny_check_version(const char *version) {
	enum hny_error retval = HNY_ERROR_INVALID_ARGS;

	if(version == NULL) {
		retval = HNY_ERROR_NONE;
	} else if(*version != '\0') {
		while(*version != '\0'
			&& *version != '/') {
			++version;
		}

		if(*version == '\0') {
			retval = HNY_ERROR_NONE;
		}
	}

	return retval;
}

enum hny_error
hny_check_package(const char *packagename) {
	enum hny_error retval;
	const char *dash = strchr(packagename, '-');

	if(dash == NULL) {
		retval = hny_check_name(packagename);
	} else {
		if((retval = hny_check_version(dash + 1)) == HNY_ERROR_NONE) {
			if(*packagename != '.') {
				while(packagename != dash
					&& *packagename != '/') {
					++packagename;
				}
			}

			if(packagename != dash) {
				retval = HNY_ERROR_INVALID_ARGS;
			}
		}
	}

	return retval;
}

enum hny_error
hny_check_geister(const struct hny_geist *geister,
	hny_size_t n) {
	enum hny_error retval = HNY_ERROR_INVALID_ARGS;
	size_t i = 0;

	if(geister != NULL && n != 0) {
		while(i < n
			&& hny_check_name(geister[i].name) == HNY_ERROR_NONE
			&& hny_check_version(geister[i].version) == HNY_ERROR_NONE) {
			++i;
		}

		if(i == n) {
			retval = HNY_ERROR_NONE;
		}
	}

	return retval;
}

int
hny_compare_versions(const char **p1,
	const char **p2) {
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

