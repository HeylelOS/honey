/*
	hny_extraction_xz.h
	Copyright (c) 2018, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE
*/
#ifndef HNY_EXTRACTION_XZ_H
#define HNY_EXTRACTION_XZ_H

#include "hny_internal.h"
#include "hny_extraction_lzma2.h"

#include <stddef.h>
#include <stdint.h>

struct hny_extraction_xz_io {
	const char *input;
	size_t inputsize;
	char *output;
	size_t outputsize;
};

struct hny_extraction_xz_stream_header {
	uint32_t crc32;
	uint16_t flags;
};

struct hny_extraction_xz_stream_block {
	enum {
		HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER,
		HNY_EXTRACTION_XZ_STREAM_BLOCK_DATA,
		HNY_EXTRACTION_XZ_STREAM_BLOCK_PADDING,
		HNY_EXTRACTION_XZ_STREAM_BLOCK_CHECK,
	} state;

	struct {
		enum {
			HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FLAGS,
			HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_COMPRESSED_SIZE,
			HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_UNCOMPRESSED_SIZE,
			HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FILTER_FLAGS,
			HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_PADDING,
			HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_CRC32
		} state;

		size_t realsize;
		uint8_t flags;
		uint64_t compressedsize;
		uint64_t uncompressedsize;
		struct {
			enum {
				HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FILTER_FLAGS_ID,
				HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FILTER_PROPERTIES_SIZE,
				HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FILTER_PROPERTIES
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

struct hny_extraction_xz_stream_index {
	enum {
		HNY_EXTRACTION_XZ_STREAM_INDEX_RECORDS_COUNT,
		HNY_EXTRACTION_XZ_STREAM_INDEX_RECORDS_LIST_UNPADDED_SIZE,
		HNY_EXTRACTION_XZ_STREAM_INDEX_RECORDS_LIST_UNCOMPRESSED_SIZE,
		HNY_EXTRACTION_XZ_STREAM_INDEX_PADDING,
		HNY_EXTRACTION_XZ_STREAM_INDEX_CRC32
	} state;

	uint64_t recordscount;
	uint64_t recordsleft;
	uint64_t temporary;
	uint32_t indexcrc32;
	uint32_t crc32;
};

struct hny_extraction_xz_stream_footer {
	uint64_t backwardsize;
	uint32_t readcrc32;
	uint32_t crc32;
};

struct hny_extraction_xz {
	enum {
		HNY_EXTRACTION_XZ_STREAM_HEADER,
		HNY_EXTRACTION_XZ_STREAM_BLOCK_OR_INDEX,
		HNY_EXTRACTION_XZ_STREAM_BLOCK,
		HNY_EXTRACTION_XZ_STREAM_INDEX,
		HNY_EXTRACTION_XZ_STREAM_FOOTER,
		HNY_EXTRACTION_XZ_STREAM_END
	} state;

	uint64_t recordscount;
	size_t offset;
	size_t multibyteindex;
	uint32_t indexcrc32;

	struct hny_extraction_xz_stream_header header;
	struct hny_extraction_xz_stream_block block;
	struct hny_extraction_xz_stream_index index;
	struct hny_extraction_xz_stream_footer footer;

	struct hny_extraction_lzma2 lzma2;
};

int
hny_extraction_xz_init(struct hny_extraction_xz *xz, size_t dictionarymax);

void
hny_extraction_xz_deinit(struct hny_extraction_xz *xz);

enum hny_extraction_status
hny_extraction_xz_decode(struct hny_extraction_xz *xz, struct hny_extraction_xz_io *buffers);

/* HNY_EXTRACTION_XZ_H */
#endif
