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
