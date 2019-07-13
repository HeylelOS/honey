#include "hny_internal.h"

#include <stdlib.h>

enum hny_type
hny_type_of(const char *entry) {
	const char *lastdash = NULL;

	if(*entry != '.') {
		while(*++entry != '\0'
			&& *entry != '/') {
			if(*entry == '-') {
				lastdash = entry;
			}
		}

		if(*entry == '/') {
			return HNY_TYPE_NONE;
		}
	} else {
		return HNY_TYPE_NONE;
	}

	if(lastdash != NULL) {
		return HNY_TYPE_PACKAGE;
	} else {
		return HNY_TYPE_GEIST;
	}
}

