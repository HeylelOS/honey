/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef XZ_DECODER_H
#define XZ_DECODER_H

#include "lzma2_decoder.h"

#include <stddef.h>
#include <stdint.h>

enum xz_decoder_status {
	XZ_DECODER_STATUS_OK,
	XZ_DECODER_STATUS_END,
	XZ_DECODER_STATUS_ERROR_HEADER_INVALID_MAGIC,
	XZ_DECODER_STATUS_ERROR_HEADER_UNSUPPORTED_CHECK,
	XZ_DECODER_STATUS_ERROR_HEADER_INVALID_CRC32,
	XZ_DECODER_STATUS_ERROR_BLOCK_UNSUPPORTED_FLAG,
	XZ_DECODER_STATUS_ERROR_BLOCK_UNSUPPORTED_FILTER_FLAG,
	XZ_DECODER_STATUS_ERROR_BLOCK_UNSUPPORTED_PROPERTIES_SIZE,
	XZ_DECODER_STATUS_ERROR_BLOCK_UNSUPPORTED_PROPERTY,
	XZ_DECODER_STATUS_ERROR_BLOCK_INVALID_COMPRESSED_SIZE,
	XZ_DECODER_STATUS_ERROR_BLOCK_INVALID_UNCOMPRESSED_SIZE,
	XZ_DECODER_STATUS_ERROR_BLOCK_INVALID_PADDING,
	XZ_DECODER_STATUS_ERROR_BLOCK_INVALID_CRC32,
	XZ_DECODER_STATUS_ERROR_LZMA2_UNABLE_DICTIONARY_RESET,
	XZ_DECODER_STATUS_ERROR_LZMA2_MEMORY_EXHAUSTED,
	XZ_DECODER_STATUS_ERROR_LZMA2_MEMORY_LIMIT,
	XZ_DECODER_STATUS_ERROR_LZMA2_INVALID_DICTIONARY_BITS,
	XZ_DECODER_STATUS_ERROR_LZMA2_CORRUPTED_DATA,
	XZ_DECODER_STATUS_ERROR_INDEX_INVALID_RECORDS_COUNT,
	XZ_DECODER_STATUS_ERROR_INDEX_INVALID,
	XZ_DECODER_STATUS_ERROR_INDEX_INVALID_PADDING,
	XZ_DECODER_STATUS_ERROR_INDEX_INVALID_CRC32,
	XZ_DECODER_STATUS_ERROR_FOOTER_INVALID_STREAM_FLAGS,
	XZ_DECODER_STATUS_ERROR_FOOTER_INVALID_BACKWARD_SIZE,
	XZ_DECODER_STATUS_ERROR_FOOTER_INVALID_MAGIC,
	XZ_DECODER_STATUS_ERROR_FOOTER_INVALID_CRC32,
};

struct xz_stream {
	struct {
		const char *next;
		size_t available;
	} input;
	struct {
		char *next;
		size_t available;
	} output;
};

struct xz_decoder_stream_header {
	uint32_t crc32;
	uint16_t flags;
};

struct xz_decoder_stream_block {
	enum {
		XZ_DECODER_STATE_STREAM_BLOCK_HEADER,
		XZ_DECODER_STATE_STREAM_BLOCK_DATA,
		XZ_DECODER_STATE_STREAM_BLOCK_PADDING,
		XZ_DECODER_STATE_STREAM_BLOCK_CHECK,
	} state;

	struct {
		enum {
			XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FLAGS,
			XZ_DECODER_STATE_STREAM_BLOCK_HEADER_COMPRESSED_SIZE,
			XZ_DECODER_STATE_STREAM_BLOCK_HEADER_UNCOMPRESSED_SIZE,
			XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FILTER_FLAGS,
			XZ_DECODER_STATE_STREAM_BLOCK_HEADER_PADDING,
			XZ_DECODER_STATE_STREAM_BLOCK_HEADER_CRC32
		} state;

		size_t realsize;
		uint8_t flags;
		uint64_t compressedsize;
		uint64_t uncompressedsize;
		struct {
			enum {
				XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FILTER_FLAGS_ID,
				XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FILTER_PROPERTIES_SIZE,
				XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FILTER_PROPERTIES
			} state;
			uint8_t dictionarybits;
		} filters;
		uint32_t crc32;
	} header;

	/**
	 * The following must be a uint64_t to stay coherent with
	 * the decoded uncompressedsize type in the index to check indexcrc32's
	 */
	uint64_t uncompressedsize;
	size_t compressedsize;

	/* Check */
	uint32_t crc32;
};

struct xz_decoder_stream_index {
	enum {
		XZ_DECODER_STATE_STREAM_INDEX_RECORDS_COUNT,
		XZ_DECODER_STATE_STREAM_INDEX_RECORDS_LIST_UNPADDED_SIZE,
		XZ_DECODER_STATE_STREAM_INDEX_RECORDS_LIST_UNCOMPRESSED_SIZE,
		XZ_DECODER_STATE_STREAM_INDEX_PADDING,
		XZ_DECODER_STATE_STREAM_INDEX_CRC32
	} state;

	uint64_t recordscount;
	uint64_t recordsleft;
	uint64_t temporary;
	uint32_t indexcrc32;
	uint32_t crc32;
};

struct xz_decoder_stream_footer {
	uint64_t backwardsize;
	uint32_t readcrc32;
	uint32_t crc32;
};

struct xz_decoder {
	enum {
		XZ_DECODER_STATE_STREAM_HEADER,
		XZ_DECODER_STATE_STREAM_BLOCK_OR_INDEX,
		XZ_DECODER_STATE_STREAM_BLOCK,
		XZ_DECODER_STATE_STREAM_INDEX,
		XZ_DECODER_STATE_STREAM_FOOTER,
		XZ_DECODER_STATE_STREAM_END
	} state;

	uint64_t recordscount;
	size_t offset;
	size_t multibyteindex;
	uint32_t indexcrc32;

	struct xz_decoder_stream_header header;
	struct xz_decoder_stream_block block;
	struct xz_decoder_stream_index index;
	struct xz_decoder_stream_footer footer;

	struct lzma2_decoder lzma2;
};

int
xz_decoder_init(struct xz_decoder *xz, size_t dictionarymax);

void
xz_decoder_deinit(struct xz_decoder *xz);

enum xz_decoder_status
xz_decoder_decode(struct xz_decoder *xz, struct xz_stream *stream);

/* XZ_DECODER_H */
#endif
