/* SPDX-License-Identifier: BSD-3-Clause */
#include "hny_prefix.h"

enum hny_type
hny_type_of(const char *entry) {

	if (*entry != '\0' && *entry != '-' && *entry != '.' && *entry != '/') {
		do {
			entry++;
		} while (*entry != '\0' && *entry != '-' && *entry != '/');

		if (*entry == '\0') {
			return HNY_TYPE_GEIST;
		}

		if (*entry == '-') {
			do {
				entry++;
			} while (*entry != '\0' && *entry != '/');
		}

		if (*entry == '\0') {
			return HNY_TYPE_PACKAGE;
		}
	}

	return HNY_TYPE_NONE;
}
