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

