/* SPDX-License-Identifier: BSD-3-Clause */
#include "hny_prefix.h"

#include <stdlib.h>
#include <errno.h>

#include "config.h"

#include "cpio_decoder.h"
#include "xz_decoder.h"

struct hny_extraction {
	struct xz_decoder xz;
	struct cpio_decoder cpio;
	size_t size;
	char buffer[];
};

static enum hny_extraction_status
xz_status_error_to_hny(enum xz_decoder_status status) {

	_Static_assert(XZ_DECODER_STATUS_ERROR_FOOTER_INVALID_CRC32 - XZ_DECODER_STATUS_ERROR_HEADER_INVALID_MAGIC == HNY_EXTRACTION_STATUS_ERROR_XZ_FOOTER_INVALID_CRC32 - HNY_EXTRACTION_STATUS_ERROR_XZ_HEADER_INVALID_MAGIC, "Mismatch error codes count between enum xz_decoder_status and enum hny_extraction_status");

	return (status - XZ_DECODER_STATUS_ERROR_HEADER_INVALID_MAGIC) + HNY_EXTRACTION_STATUS_ERROR_XZ_HEADER_INVALID_MAGIC;
}

static enum hny_extraction_status
cpio_status_error_to_hny(enum cpio_decoder_status status) {

	_Static_assert(CPIO_DECODER_STATUS_ERROR_WRITE - CPIO_DECODER_STATUS_ERROR_HEADER_INVALID_MAGIC == HNY_EXTRACTION_STATUS_ERROR_CPIO_WRITE - HNY_EXTRACTION_STATUS_ERROR_CPIO_HEADER_INVALID_MAGIC, "Mismatch error codes count between enum xz_decoder_status and enum hny_extraction_status");

	return (status - CPIO_DECODER_STATUS_ERROR_HEADER_INVALID_MAGIC) + HNY_EXTRACTION_STATUS_ERROR_CPIO_HEADER_INVALID_MAGIC;
}

int
hny_extraction_create(struct hny_extraction **extractionp, struct hny *hny, const char *package) {
	return hny_extraction_create2(extractionp, hny, package, CONFIG_HNY_EXTRACTION_BUFFERSIZE_DEFAULT, CONFIG_HNY_EXTRACTION_DICTIONARYMAX_DEFAULT);
}

int
hny_extraction_create2(struct hny_extraction **extractionp, struct hny *hny, const char *package, size_t size, size_t dictionarymax) {
	struct hny_extraction *extraction;
	int errcode;

	if (size < CONFIG_HNY_EXTRACTION_BUFFERSIZE_MIN) {
		size = CONFIG_HNY_EXTRACTION_BUFFERSIZE_MIN;
	}

	if (hny_type_of(package) != HNY_TYPE_PACKAGE) {
		errcode = EINVAL;
		goto hny_extraction_create_err0;
	}

	extraction = malloc(sizeof (*extraction) + size);
	if (extraction == NULL) {
		errcode = errno;
		goto hny_extraction_create_err0;
	}

	extraction->size = size;

	errcode = xz_decoder_init(&extraction->xz, dictionarymax);
	if (errcode != 0) {
		goto hny_extraction_create_err1;
	}

	errcode = cpio_decoder_init(&extraction->cpio, dirfd(hny->dirp), package);
	if (errcode != 0) {
		goto hny_extraction_create_err2;
	}

	*extractionp = extraction;

	return 0;
hny_extraction_create_err2:
	xz_decoder_deinit(&extraction->xz);
hny_extraction_create_err1:
	free(extraction);
hny_extraction_create_err0:
	return errcode;
}

void
hny_extraction_destroy(struct hny_extraction *extraction) {
	cpio_decoder_deinit(&extraction->cpio);
	xz_decoder_deinit(&extraction->xz);
	free(extraction);
}

enum hny_extraction_status
hny_extraction_extract(struct hny_extraction *extraction, const char *buffer, size_t size) {
	enum hny_extraction_status status = HNY_EXTRACTION_STATUS_OK;
	struct xz_stream stream = { .input = { .next = buffer, .available = size } };

	while (size != 0) {
		stream.output.next = extraction->buffer;
		stream.output.available = extraction->size;

		const enum xz_decoder_status status1 = xz_decoder_decode(&extraction->xz, &stream);
		if (status1 > XZ_DECODER_STATUS_END) { /* XZ_DECODER_STATUS_ERROR_* */
			status = xz_status_error_to_hny(status1);
			break;
		}

		const enum cpio_decoder_status status2 = cpio_decoder_decode(&extraction->cpio, extraction->buffer, extraction->size - stream.output.available);
		if (status2 > CPIO_DECODER_STATUS_END) { /* CPIO_DECODER_STATUS_ERROR_* */
			status = cpio_status_error_to_hny(status2);
			break;
		}

		if (status1 == XZ_DECODER_STATUS_END) {
			if (status2 == CPIO_DECODER_STATUS_END) {
				status = HNY_EXTRACTION_STATUS_END;
			} else {
				status = HNY_EXTRACTION_STATUS_ERROR_UNFINISHED_CPIO;
			}
			break;
		}
	}

	return status;
}

int
hny_extraction_errcode(struct hny_extraction *extraction) {
	const int errcode = extraction->cpio.errcode;

	extraction->cpio.errcode = 0;

	return errcode;
}
