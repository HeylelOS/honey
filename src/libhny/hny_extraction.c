/*
	hny_extraction.c
	Copyright (c) 2018-2019, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE
*/
#include "hny_internal.h"
#include "hny_extraction_xz.h"
#include "hny_extraction_cpio.h"

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#define HNY_EXTRACTION_BUFFERSIZE_DEFAULT    4096
#define HNY_EXTRACTION_BUFFERSIZE_MIN        512
#define HNY_EXTRACTION_DICTIONARYMAX_DEFAULT UINT32_MAX

struct hny_extraction {
	size_t buffersize;
	struct hny_extraction_xz xz;
	struct hny_extraction_cpio cpio;
	char buffer[];
};

int
hny_extraction_create(struct hny_extraction **extractionp,
	struct hny *hny, const char *package) {

	return hny_extraction_create2(extractionp, hny, package,
		HNY_EXTRACTION_BUFFERSIZE_DEFAULT, HNY_EXTRACTION_DICTIONARYMAX_DEFAULT);
}

int
hny_extraction_create2(struct hny_extraction **extractionp,
	struct hny *hny, const char *package,
	size_t buffersize, size_t dictionarymax) {
	struct hny_extraction *extraction;
	int errcode;

	if(buffersize < HNY_EXTRACTION_BUFFERSIZE_MIN) {
		buffersize = HNY_EXTRACTION_BUFFERSIZE_MIN;
	}

	if((errno = EINVAL, hny_type_of(package) != HNY_TYPE_PACKAGE)
		|| (extraction = malloc(sizeof(*extraction) + buffersize)) == NULL) {
		errcode = errno;
		goto hny_extraction_create_err0;
	}

	if((errcode = hny_extraction_xz_init(&extraction->xz, dictionarymax)) != 0) {
		goto hny_extraction_create_err1;
	}

	if((errcode = hny_extraction_cpio_init(&extraction->cpio, dirfd(hny->dirp), package)) != 0) {
		goto hny_extraction_create_err2;
	}

	*extractionp = extraction;

	return 0;
hny_extraction_create_err2:
	hny_extraction_xz_deinit(&extraction->xz);
hny_extraction_create_err1:
	free(extraction);
hny_extraction_create_err0:
	return errcode;
}

void
hny_extraction_destroy(struct hny_extraction *extraction) {

	hny_extraction_cpio_deinit(&extraction->cpio);
	hny_extraction_xz_deinit(&extraction->xz);
	free(extraction);
}

enum hny_extraction_status
hny_extraction_extract(struct hny_extraction *extraction,
	const char *buffer, size_t size, int *errcode) {
	enum hny_extraction_status cpiostatus;

	cpiostatus = hny_extraction_cpio_decode(&extraction->cpio, buffer, size, errcode);

	if(cpiostatus == HNY_EXTRACTION_STATUS_END_CPIO) {
		return HNY_EXTRACTION_STATUS_END;
	}

	return cpiostatus;
}

