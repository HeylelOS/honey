/*
	remove.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"

enum hny_error hny_remove(enum hny_removal removal, const struct hny_geist *geist) {
	if(hny_check_geister(geist, 1) != HnyErrorNone) {
		return HnyErrorInvalidArgs;
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

	return HnyErrorNone;
}

