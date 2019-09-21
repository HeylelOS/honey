/*
	hny_extraction_xz.h
	Copyright (c) 2018-2019, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE
*/
#ifndef HNY_EXTRACTION_XZ_H
#define HNY_EXTRACTION_XZ_H

#include <stdint.h>
#include <sys/types.h>

enum hny_extraction_xz_status {
	HNY_EXTRACTION_XZ_STATUS_OK,
	HNY_EXTRACTION_XZ_STATUS_END,
	HNY_EXTRACTION_XZ_STATUS_ERROR
};

struct hny_extraction_xz_stream_header {
	uint32_t crc32;
	uint16_t flags;
};

struct hny_extraction_xz_stream_block {
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
};

struct hny_extraction_xz {
	enum {
		HNY_EXTRACTION_XZ_STREAM_HEADER,
		HNY_EXTRACTION_XZ_STREAM_BLOCK_OR_INDEX,
		HNY_EXTRACTION_XZ_STREAM_BLOCK,
		HNY_EXTRACTION_XZ_STREAM_BLOCK_DATA,
		HNY_EXTRACTION_XZ_STREAM_INDEX,
		HNY_EXTRACTION_XZ_STREAM_FOOTER,
		HNY_EXTRACTION_XZ_STREAM_END
	} state;

	struct hny_extraction_xz_stream_header header;
	struct hny_extraction_xz_stream_block block;

	size_t offset;
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
