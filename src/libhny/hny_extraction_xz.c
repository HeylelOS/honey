/*
	hny_extraction_xz.c
	Copyright (c) 2018-2019, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE
*/
#include "hny_extraction_xz.h"

int
hny_extraction_xz_init(struct hny_extraction_xz *xz, size_t dictionarymax) {

	xz->state = HNY_EXTRACTION_XZ_STREAM_HEADER;

	return 0;
}

void
hny_extraction_xz_deinit(struct hny_extraction_xz *xz) {

	(void)xz;
}

enum hny_extraction_xz_status
hny_extraction_xz_decode(struct hny_extraction_xz *xz,
	const char *in, size_t insize,
	const char **out, size_t *outsize,
	int *errcode) {
	(void)xz;

	*out = in;
	*outsize = insize;

	return HNY_EXTRACTION_XZ_STATUS_OK;
}

