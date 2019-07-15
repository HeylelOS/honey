/*
	hny_extraction_xz.h
	Copyright (c) 2018-2019, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE
*/
#ifndef HNY_EXTRACTION_XZ_H
#define HNY_EXTRACTION_XZ_H

#include <sys/types.h>

enum hny_extraction_xz_status {
	HNY_EXTRACTION_XZ_STATUS_OK,
	HNY_EXTRACTION_XZ_STATUS_END,
	HNY_EXTRACTION_XZ_STATUS_ERROR
};

struct hny_extraction_xz {
	int dummy;
};

int
hny_extraction_xz_init(struct hny_extraction_xz *xz);

void
hny_extraction_xz_deinit(struct hny_extraction_xz *xz);

enum hny_extraction_xz_status
hny_extraction_xz_decode(struct hny_extraction_xz *xz,
	const char *in, size_t insize,
	const char **out, size_t *outsize,
	int *errcode);

/* HNY_EXTRACTION_XZ_H */
#endif
