/*
	archive.c
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/
#include "internal.h"
/*
#include <archive.h>
#include <archive_entry.h>
*/

enum hny_error hny_install(const char *file, _Bool (*eula)(const char *, size_t)) {
	enum hny_error error = HnyErrorNone;
	static const char license[64] = "Do you agree?";
/*
	struct archive *a;
	struct archive_entry *entry;
*/
	pthread_mutex_lock(&hive->mutex);
/*
	a = archive_read_new();
	archive_read_support_filter_xz(a);
	archive_read_support_format_tar(a);

	archive_read_open_filename(a, file, 4096);

	while(archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		printf("%s\n", archive_entry_pathname(entry));
		archive_read_data_skip(a);
	}

	archive_read_free(a);
*/

	if(!eula(license, sizeof(license))) {
		error = HnyErrorUnavailable;
	}

	pthread_mutex_unlock(&hive->mutex);

	return error;
}

