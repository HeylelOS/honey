/*
	archive.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

enum hny_error hny_install(const char *file) {
	pthread_mutex_lock(&hive->mutex);

	printf("Installing %s in %s\n", file, hive->installdir);

	pthread_mutex_unlock(&hive->mutex);

	return HnyErrorNone;
}


