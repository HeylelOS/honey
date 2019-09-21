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
#include <errno.h>

struct hny_extraction {
	struct hny_extraction_xz xz;
	struct hny_extraction_cpio cpio;
};

int
hny_extraction_create(struct hny_extraction **extractionp,
	struct hny *hny, const char *package) {
	struct hny_extraction *extraction;
	int errcode;

	if((errno = EINVAL, hny_type_of(package) != HNY_TYPE_PACKAGE)
		|| (extraction = malloc(sizeof(*extraction))) == NULL) {
		errcode = errno;
		goto hny_extraction_create_err0;
	}

	if((errcode = hny_extraction_xz_init(&extraction->xz)) != 0) {
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

	switch(hny_extraction_cpio_decode(&extraction->cpio, buffer, size, errcode)) {
	case HNY_EXTRACTION_CPIO_STATUS_OK:
		return HNY_EXTRACTION_STATUS_OK;
	case HNY_EXTRACTION_CPIO_STATUS_END:
		return HNY_EXTRACTION_STATUS_END;
	default: /* HNY_EXTRACTION_CPIO_STATUS_ERROR */
		return HNY_EXTRACTION_STATUS_ERROR_UNARCHIVE;
	}
/*
	const char *decoded;
	size_t decodedsize;

	if(hny_extraction_xz_decode(&extraction->xz,
		buffer, size, &decoded, &decodedsize, errcode) != HNY_EXTRACTION_XZ_STATUS_ERROR) {
		switch(hny_extraction_cpio_decode(&extraction->cpio,
			decoded, decodedsize, errcode)) {
		case HNY_EXTRACTION_CPIO_STATUS_OK:
			return HNY_EXTRACTION_STATUS_OK;
		case HNY_EXTRACTION_CPIO_STATUS_END:
			return HNY_EXTRACTION_STATUS_END;
		default: // HNY_EXTRACTION_CPIO_STATUS_ERROR
			return HNY_EXTRACTION_STATUS_ERROR_UNARCHIVE;
		}
	} else {
		return HNY_EXTRACTION_STATUS_ERROR_DECOMPRESSION;
	}
*/
}

