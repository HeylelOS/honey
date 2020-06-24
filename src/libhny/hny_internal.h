/*
	hny_internal.h
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE
*/
#ifndef HNY_INTERNAL_H
#define HNY_INTERNAL_H

#include "hny.h"

#include <dirent.h>

struct hny {
	DIR *dirp;
	char *path;
	int flags;
};

/* HNY_INTERNAL_H */
#endif
