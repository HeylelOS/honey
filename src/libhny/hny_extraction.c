/* SPDX-License-Identifier: BSD-3-Clause */
#include "hny_internal.h"
#include "hny_extraction_xz.h"
#include "hny_extraction_cpio.h"

#include <stdlib.h>
#include <errno.h>

#include "config.h"

struct hny_extraction {
	size_t buffersize;
	struct hny_extraction_xz xz;
	struct hny_extraction_cpio cpio;
	char buffer[];
};

int
hny_extraction_create(struct hny_extraction **extractionp, struct hny *hny, const char *package) {

	return hny_extraction_create2(extractionp, hny, package,
		CONFIG_HNY_EXTRACTION_BUFFERSIZE_DEFAULT, CONFIG_HNY_EXTRACTION_DICTIONARYMAX_DEFAULT);
}

int
hny_extraction_create2(struct hny_extraction **extractionp,
	struct hny *hny, const char *package, size_t buffersize, size_t dictionarymax) {
	struct hny_extraction *extraction;
	int errcode;

	if(buffersize < CONFIG_HNY_EXTRACTION_BUFFERSIZE_MIN) {
		buffersize = CONFIG_HNY_EXTRACTION_BUFFERSIZE_MIN;
	}

	if(hny_type_of(package) != HNY_TYPE_PACKAGE) {
		errcode = EINVAL;
		goto hny_extraction_create_err0;
	}

	extraction = malloc(sizeof(*extraction) + buffersize);
	if(extraction == NULL) {
		errcode = errno;
		goto hny_extraction_create_err0;
	}

	extraction->buffersize = buffersize;

	errcode = hny_extraction_xz_init(&extraction->xz, dictionarymax);
	if(errcode != 0) {
		goto hny_extraction_create_err1;
	}

	errcode = hny_extraction_cpio_init(&extraction->cpio, dirfd(hny->dirp), package);
	if(errcode != 0) {
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
hny_extraction_extract(struct hny_extraction *extraction, const char *buffer, size_t size, int *errcode) {
	enum hny_extraction_status status;
	struct hny_extraction_xz_io buffers = {
		.input = buffer, .inputsize = size,
		.output = extraction->buffer, .outputsize = extraction->buffersize
	};

	while(!HNY_EXTRACTION_STATUS_IS_ERROR(status = hny_extraction_xz_decode(&extraction->xz, &buffers))) {
		enum hny_extraction_status cpiostatus = hny_extraction_cpio_decode(&extraction->cpio,
			extraction->buffer, extraction->buffersize - buffers.outputsize, errcode);

		if(!HNY_EXTRACTION_STATUS_IS_ERROR(cpiostatus)) {
			buffers.output = extraction->buffer;
			buffers.outputsize = extraction->buffersize;

			if(buffers.inputsize == 0 || status == HNY_EXTRACTION_STATUS_END) {
				if(status == HNY_EXTRACTION_STATUS_END && cpiostatus != HNY_EXTRACTION_STATUS_END) {
					status = HNY_EXTRACTION_STATUS_ERROR_UNFINISHED_CPIO;
				}
				break;
			}
		} else {
			status = cpiostatus;
			break;
		}
	}

	return status;
}

