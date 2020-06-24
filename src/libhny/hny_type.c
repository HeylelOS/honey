/*
	hny_type.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE
*/
#include "hny_internal.h"

#include <stdlib.h>

enum hny_type
hny_type_of(const char *entry) {

	if(*entry != '\0'
		&& *entry != '-'
		&& *entry != '.'
		&& *entry != '/') {
		do {
			entry++;
		} while(*entry != '\0'
			&& *entry != '-'
			&& *entry != '/');

		if(*entry == '\0') {
			return HNY_TYPE_GEIST;
		}

		if(*entry == '-') {
			if(*++entry != '\0'
				&& *entry != '/') {
				do {
					entry++;
				} while(*entry != '\0'
					&& *entry != '/');
			}
		}

		if(*entry == '\0') {
			return HNY_TYPE_PACKAGE;
		}
	}

	return HNY_TYPE_NONE;
}

