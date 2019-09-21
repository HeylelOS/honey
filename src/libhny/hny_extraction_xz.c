/*
	hny_extraction_xz.c
	Copyright (c) 2018-2019, Valentin Debon

	This file is part of the honey package manager
	subject the BSD 3-Clause License, see LICENSE
*/
#include "hny_extraction_xz.h"

#include <stdlib.h>
#include <stdbool.h>

/**
 * TODO:
 * - Remove multibyteindex by using possible overflow as a hint.
 */

#define HNY_EXTRACTION_XZ_STREAM_HEADER_SIZE 12
#define HNY_EXTRACTION_XZ_STREAM_FOOTER_SIZE 12

#define HNY_EXTRACTION_XZ_MIN(a, b) ((a) < (b) ? (a) : (b))

/*****************************************************************
 * Private functions for decoding, implemented later in the file *
 *****************************************************************/

static enum hny_extraction_status
hny_extraction_xz_decode_stream_header(struct hny_extraction_xz *xz, struct hny_extraction_xz_io *buffers);

static inline enum hny_extraction_status
hny_extraction_xz_decode_stream_block_or_index(struct hny_extraction_xz *xz, struct hny_extraction_xz_io *buffers);

static enum hny_extraction_status
hny_extraction_xz_decode_stream_block(struct hny_extraction_xz *xz, struct hny_extraction_xz_io *buffers);

static enum hny_extraction_status
hny_extraction_xz_decode_stream_index(struct hny_extraction_xz *xz, struct hny_extraction_xz_io *buffers);

static enum hny_extraction_status
hny_extraction_xz_decode_stream_footer(struct hny_extraction_xz *xz, struct hny_extraction_xz_io *buffers);

/***********************************
 * Public interface of the decoder *
 ***********************************/

int
hny_extraction_xz_init(struct hny_extraction_xz *xz, size_t dictionarymax) {
	xz->state = HNY_EXTRACTION_XZ_STREAM_HEADER;
	xz->offset = 0;

	return hny_extraction_lzma2_init(&xz->lzma2, HNY_EXTRACTION_LZMA2_MODE_DYNAMIC, HNY_EXTRACTION_XZ_MIN(dictionarymax, UINT32_MAX));
}

void
hny_extraction_xz_deinit(struct hny_extraction_xz *xz) {

	hny_extraction_lzma2_deinit(&xz->lzma2);
}

enum hny_extraction_status
hny_extraction_xz_decode(struct hny_extraction_xz *xz, struct hny_extraction_xz_io *buffers) {
	enum hny_extraction_status status = HNY_EXTRACTION_STATUS_OK;

	while(status == HNY_EXTRACTION_STATUS_OK && buffers->inputsize != 0 && buffers->outputsize != 0) {
		switch(xz->state) {
		case HNY_EXTRACTION_XZ_STREAM_HEADER:
			status = hny_extraction_xz_decode_stream_header(xz, buffers);
			break;
		case HNY_EXTRACTION_XZ_STREAM_BLOCK_OR_INDEX:
			status = hny_extraction_xz_decode_stream_block_or_index(xz, buffers);
			break;
		case HNY_EXTRACTION_XZ_STREAM_BLOCK:
			status = hny_extraction_xz_decode_stream_block(xz, buffers);
			break;
		case HNY_EXTRACTION_XZ_STREAM_INDEX:
			status = hny_extraction_xz_decode_stream_index(xz, buffers);
			break;
		case HNY_EXTRACTION_XZ_STREAM_FOOTER:
			status = hny_extraction_xz_decode_stream_footer(xz, buffers);
			break;
		default: /* HNY_EXTRACTION_XZ_STREAM_END */
			status = HNY_EXTRACTION_STATUS_END;
			break;
		}
	}

	return status;
}

/**********************************************
 * Implementation of private values/functions *
 **********************************************/

/*********
 * CRC32 *
 *********/

static const uint32_t hny_extraction_xz_crc32_init = -1;

static const uint32_t hny_extraction_xz_crc32_table[UINT8_MAX + 1] = {
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
	0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
	0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
	0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
	0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
	0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
	0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
	0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
	0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
	0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
	0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
	0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
	0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
	0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
	0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
	0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
	0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
	0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
	0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
	0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
	0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
	0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
	0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
	0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
	0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
	0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
	0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
	0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
	0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
	0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
	0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
	0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
	0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
	0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
	0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
	0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
	0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
	0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
	0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
	0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
	0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
	0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
	0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
	0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

static inline uint32_t
hny_extraction_xz_crc32_update(uint32_t crc32,
	const uint8_t *bytes,
	size_t size) {
	const uint8_t *end = bytes + size;

	while(bytes < end) {
		const uint8_t index = crc32 ^ *bytes;
		crc32 = (crc32 >> 8) ^ hny_extraction_xz_crc32_table[index];

		bytes += 1;
	}

	return crc32;
}

static inline uint32_t
hny_extraction_xz_crc32_end(uint32_t crc32) {

	return ~crc32;
}

static inline bool
hny_extraction_xz_multibyte_integer(uint64_t *value, size_t *index,
	const char **buffer, const char *bufferend) {
	uint8_t byte;

	do {
		byte = **buffer;
		*value |= (byte & 0x7F) << (*index * 7);
		++*index;
		++*buffer;
	} while((byte & 0x80) != 0
		&& *index < 9
		&& *buffer < bufferend);

	return (byte & 0x80) == 0 || *index >= 9;
}

/********************
 * XZ Stream Header *
 ********************/

static const uint8_t hny_extraction_xz_stream_header_magic[] = { 0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00 };

static enum hny_extraction_status
hny_extraction_xz_decode_stream_header(struct hny_extraction_xz *xz, struct hny_extraction_xz_io *buffers) {
	const char *headerbegin = buffers->input;
	const char *headerend = buffers->input + HNY_EXTRACTION_XZ_MIN(HNY_EXTRACTION_XZ_STREAM_HEADER_SIZE - xz->offset, buffers->inputsize);
	enum hny_extraction_status status = HNY_EXTRACTION_STATUS_OK;

	while(buffers->input < headerend) {
		const uint8_t byte = *buffers->input;

		if((xz->offset & ~0x07) == 0) {
			switch(xz->offset) {
			default: /* magic number check */
				if(byte != hny_extraction_xz_stream_header_magic[xz->offset]) {
					status = HNY_EXTRACTION_STATUS_ERROR_XZ_HEADER_INVALID_MAGIC;
					goto hny_extraction_xz_decode_stream_header_end;
				}
				break;
			case 6:
				if(byte != 0) {
					status = HNY_EXTRACTION_STATUS_ERROR_XZ_HEADER_UNSUPPORTED_CHECK;
					goto hny_extraction_xz_decode_stream_header_end;
				}
				break;
			case 7:
				if((byte & 0xF0) != 0
					|| (byte != 0x00 && byte != 0x01)) { /* Doesn't support None or CRC32 check */
					status = HNY_EXTRACTION_STATUS_ERROR_XZ_HEADER_UNSUPPORTED_CHECK;
					goto hny_extraction_xz_decode_stream_header_end;
				}
				xz->header.flags = byte;
				/* The following is a precomputed crc32 from the static header */
				xz->header.crc32 = hny_extraction_xz_crc32_end(hny_extraction_xz_crc32_update(0x2DFD1072, &byte, 1));
				break;
			}
			xz->offset++;
		} else {
			/* Checking CRC32 one byte at a time */
			if(byte != (uint8_t)(xz->header.crc32 >> (uint8_t)(xz->offset - 8) * 8)) {
				status = HNY_EXTRACTION_STATUS_ERROR_XZ_HEADER_INVALID_CRC32;
				goto hny_extraction_xz_decode_stream_header_end;
			}

			xz->offset++;
			if(xz->offset == HNY_EXTRACTION_XZ_STREAM_HEADER_SIZE) {
				xz->state = HNY_EXTRACTION_XZ_STREAM_BLOCK_OR_INDEX;
				xz->offset = 0;
				xz->recordscount = 0;
				xz->indexcrc32 = hny_extraction_xz_crc32_init;
			}
		}
		buffers->input++;
	}

hny_extraction_xz_decode_stream_header_end:
	buffers->inputsize -= buffers->input - headerbegin;

	return status;
}

/****************************
 * XZ Stream Block or Index *
 ****************************/

static inline enum hny_extraction_status
hny_extraction_xz_decode_stream_block_or_index(struct hny_extraction_xz *xz, struct hny_extraction_xz_io *buffers) {
	uint8_t encodedsize = *buffers->input;

	if(encodedsize != 0) {
		xz->state = HNY_EXTRACTION_XZ_STREAM_BLOCK;
		xz->block.state = HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER;
		xz->block.header.state = HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FLAGS;
		xz->block.header.crc32 = hny_extraction_xz_crc32_update(hny_extraction_xz_crc32_init, &encodedsize, 1);
		xz->block.header.realsize = (encodedsize + 1) * 4;
	} else {
		xz->state = HNY_EXTRACTION_XZ_STREAM_INDEX;
		xz->multibyteindex = 0;
		xz->indexcrc32 = hny_extraction_xz_crc32_end(xz->indexcrc32);
		xz->index.state = HNY_EXTRACTION_XZ_STREAM_INDEX_RECORDS_COUNT;
		xz->index.crc32 = hny_extraction_xz_crc32_update(hny_extraction_xz_crc32_init, &encodedsize, 1);
		xz->index.recordscount = 0;
		xz->index.indexcrc32 = hny_extraction_xz_crc32_init;
	}

	xz->offset++;
	buffers->input++;
	buffers->inputsize--;

	return HNY_EXTRACTION_STATUS_OK;
}

/*******************
 * XZ Stream Block *
 *******************/

static int
hny_extraction_xz_decode_stream_block_header(struct hny_extraction_xz *xz, struct hny_extraction_xz_io *buffers) {
	const char *blockheaderbegin = buffers->input;
	const char *blockheaderend = buffers->input + HNY_EXTRACTION_XZ_MIN(xz->block.header.realsize - xz->offset, buffers->inputsize);
	int retval = HNY_EXTRACTION_STATUS_OK;

	while(buffers->input < blockheaderend) {
		const char *chunkbegin = buffers->input;

		switch(xz->block.header.state) {
		case HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FLAGS:
			xz->block.header.flags = *buffers->input;

			/* Only LZMA2 is supported here so we add 0x03 to limit to one filter */
			if((xz->block.header.flags & (0x3C | 0x03)) != 0) {
				retval = HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_UNSUPPORTED_FLAG;
				goto hny_extraction_xz_decode_stream_block_end;
			}

			switch(xz->block.header.flags & 0xC0) {
			case 0x00:
				xz->block.header.state = HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FILTER_FLAGS;
				break;
			case 0x80:
				xz->block.header.state = HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_UNCOMPRESSED_SIZE;
				break;
			default: /* 0x40, 0xC0 */
				xz->block.header.state = HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_COMPRESSED_SIZE;
				break;
			}

			xz->multibyteindex = 0;
			xz->block.header.compressedsize = 0;
			xz->block.header.uncompressedsize = 0;
			/* From this point, HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FILTER_FLAGS will be reached, initializing here */
			xz->block.header.filters.state = HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FILTER_FLAGS_ID;
			buffers->input++;
			break;
		case HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_COMPRESSED_SIZE:
			if(hny_extraction_xz_multibyte_integer(&xz->block.header.compressedsize, &xz->multibyteindex,
				&buffers->input, blockheaderend)) {
				xz->multibyteindex = 0;
				xz->block.header.state = (xz->block.header.flags & 0x80) != 0 ?
					HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_UNCOMPRESSED_SIZE : HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FILTER_FLAGS;
			}
			break;
		case HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_UNCOMPRESSED_SIZE:
			if(hny_extraction_xz_multibyte_integer(&xz->block.header.uncompressedsize, &xz->multibyteindex,
				&buffers->input, blockheaderend)) {
				xz->block.header.state = HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FILTER_FLAGS;
			}
			break;
		case HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FILTER_FLAGS:
			switch(xz->block.header.filters.state) {
			case HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FILTER_FLAGS_ID:
				/* Only LZMA2 is supported here, so we only check for its flags */
				if((uint8_t)*buffers->input == 0x21) {
					xz->block.header.filters.state = HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FILTER_PROPERTIES_SIZE;
					buffers->input++;
					break;
				} else {
					retval = HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_UNSUPPORTED_FILTER_FLAG;
					goto hny_extraction_xz_decode_stream_block_end;
				}
			case HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FILTER_PROPERTIES_SIZE:
				/* Only LZMA2 is supported here, so we only check its properties size */
				if((uint8_t)*buffers->input == 0x01) {
					xz->block.header.filters.state = HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FILTER_PROPERTIES;
					buffers->input++;
					break;
				} else {
					retval = HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_UNSUPPORTED_PROPERTIES_SIZE;
					goto hny_extraction_xz_decode_stream_block_end;
				}
			default: /* HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_FILTER_PROPERTIES: */ {
				/* Only LZMA2 is supported here, so we acquire dictionary size */
				const uint8_t byte = *buffers->input;

				if((byte & 0xC0) == 0x00) {
					xz->block.header.state = HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_PADDING;
					xz->block.header.filters.dictionarybits = byte & 0x3F;
					buffers->input++;
					break;
				} else {
					retval = HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_UNSUPPORTED_PROPERTY;
					goto hny_extraction_xz_decode_stream_block_end;
				}
			} break;
			}
			break;
		case HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_PADDING:
			if(xz->offset < xz->block.header.realsize - sizeof(xz->block.header.crc32)) {
				if(*buffers->input != 0) {
					retval = HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_INVALID_PADDING;
					goto hny_extraction_xz_decode_stream_block_end;
				}
				buffers->input++;
			} else {
				xz->block.header.state = HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_CRC32;
				xz->block.header.crc32 = hny_extraction_xz_crc32_end(xz->block.header.crc32);
			}
			break;
		case HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER_CRC32:
			if((uint8_t)*buffers->input != (uint8_t)(xz->block.header.crc32
				>> ((uint8_t)(xz->block.header.realsize - xz->offset - 1) ^ 0x03) * 8)) {
				retval = HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_INVALID_CRC32;
				goto hny_extraction_xz_decode_stream_block_end;
			}

			buffers->input++;
			xz->offset++;
			if(xz->offset == xz->block.header.realsize) {
				xz->block.state = HNY_EXTRACTION_XZ_STREAM_BLOCK_DATA;
				if(xz->header.flags == 0x01) { /* CRC32 Check else None */
					xz->block.crc32 = hny_extraction_xz_crc32_init;
				}

				if(hny_extraction_lzma2_reset(&xz->lzma2, xz->block.header.filters.dictionarybits) != HNY_EXTRACTION_LZMA2_OK) {
					retval = HNY_EXTRACTION_STATUS_ERROR_XZ_LZMA2_UNABLE_DICTIONARY_RESET;
				}

				goto hny_extraction_xz_decode_stream_block_end;
			} else {
				continue;
			}
		}

		size_t decoded = buffers->input - chunkbegin;
		xz->block.header.crc32 = hny_extraction_xz_crc32_update(xz->block.header.crc32,
			(const uint8_t *)chunkbegin, decoded);
		xz->offset += decoded;
	}

hny_extraction_xz_decode_stream_block_end:
	buffers->inputsize -= buffers->input - blockheaderbegin;

	return retval;
}

static inline int
hny_extraction_xz_decode_stream_block_data(struct hny_extraction_xz *xz, struct hny_extraction_xz_io *buffers) {
	struct hny_extraction_lzma2_io lzma2buffers = {
		.input = (const uint8_t *)buffers->input, .inputposition = 0, .inputsize = buffers->inputsize,
		.output = (uint8_t *)buffers->output, .outputposition = 0, .outputsize = buffers->outputsize
	};
	enum hny_extraction_lzma2_status lzma2status = hny_extraction_lzma2_decode(&xz->lzma2, &lzma2buffers);

	if(xz->header.flags == 0x01) { /* Check CRC32, else None */
		xz->block.crc32 = hny_extraction_xz_crc32_update(xz->block.crc32,
			lzma2buffers.output, lzma2buffers.outputposition);
	}

	buffers->input += lzma2buffers.inputposition;
	buffers->inputsize -= lzma2buffers.inputposition;
	buffers->output += lzma2buffers.outputposition;
	buffers->outputsize -= lzma2buffers.outputposition;

	xz->block.compressedsize += lzma2buffers.inputposition;
	xz->block.uncompressedsize += lzma2buffers.outputposition;

	switch(lzma2status) {
	case HNY_EXTRACTION_LZMA2_END:
		if((xz->block.header.flags & 0x40) != 0
			&& xz->block.header.compressedsize != xz->block.compressedsize) {
			return HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_INVALID_COMPRESSED_SIZE;
		}

		if((xz->block.header.flags & 0x80) != 0
			&& xz->block.header.uncompressedsize != xz->block.uncompressedsize) {
			return HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_INVALID_UNCOMPRESSED_SIZE;
		}

		xz->block.state = HNY_EXTRACTION_XZ_STREAM_BLOCK_PADDING;
		xz->offset = xz->block.compressedsize;
		if(xz->header.flags == 0x01) { /* Check CRC32, else None */
			xz->block.crc32 = hny_extraction_xz_crc32_end(xz->block.crc32);
		}
	case HNY_EXTRACTION_LZMA2_OK:
		/* fallthrough */
		return HNY_EXTRACTION_STATUS_OK;
	case HNY_EXTRACTION_LZMA2_ERROR_MEMORY_EXHAUSTED:
		return HNY_EXTRACTION_STATUS_ERROR_XZ_LZMA2_MEMORY_EXHAUSTED;
	case HNY_EXTRACTION_LZMA2_ERROR_MEMORY_LIMIT:
		return HNY_EXTRACTION_STATUS_ERROR_XZ_LZMA2_MEMORY_LIMIT;
	case HNY_EXTRACTION_LZMA2_ERROR_INVALID_DICTIONARY_BITS:
		return HNY_EXTRACTION_STATUS_ERROR_XZ_LZMA2_INVALID_DICTIONARY_BITS;
	default: /* HNY_EXTRACTION_LZMA2_ERROR_CORRUPTED_DATA */
		return HNY_EXTRACTION_STATUS_ERROR_XZ_LZMA2_CORRUPTED_DATA;
	}
}

static enum hny_extraction_status
hny_extraction_xz_decode_stream_block(struct hny_extraction_xz *xz, struct hny_extraction_xz_io *buffers) {

	switch(xz->block.state) {
	case HNY_EXTRACTION_XZ_STREAM_BLOCK_HEADER:
		return hny_extraction_xz_decode_stream_block_header(xz, buffers);
	case HNY_EXTRACTION_XZ_STREAM_BLOCK_DATA:
		return hny_extraction_xz_decode_stream_block_data(xz, buffers);
	case HNY_EXTRACTION_XZ_STREAM_BLOCK_PADDING:
		if((xz->offset & 0x03) == 0) {
			/* The following is computed instead of counted, check size is either 0 (none) or 4 (crc32) */
			uint64_t unpaddedsize = xz->block.header.realsize + xz->block.compressedsize + (xz->header.flags << 2);

			if(xz->header.flags == 0x01) {
				xz->block.state = HNY_EXTRACTION_XZ_STREAM_BLOCK_CHECK;
			} else {
				xz->state = HNY_EXTRACTION_XZ_STREAM_BLOCK_OR_INDEX;
			}
			xz->offset = 0;
			xz->recordscount++;
			xz->indexcrc32 = hny_extraction_xz_crc32_update(xz->indexcrc32, (const uint8_t *)&unpaddedsize, sizeof(unpaddedsize));
			xz->indexcrc32 = hny_extraction_xz_crc32_update(xz->indexcrc32, (const uint8_t *)&xz->block.uncompressedsize, sizeof(xz->block.uncompressedsize));
		} else {
			if(*buffers->input != 0) {
				return HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_INVALID_PADDING;
			}
			xz->offset++;
			buffers->input++;
			buffers->inputsize--;
		}
		return HNY_EXTRACTION_STATUS_OK;
	default: /* HNY_EXTRACTION_XZ_STREAM_BLOCK_CHECK */
		if((uint8_t)*buffers->input != (uint8_t)(xz->block.crc32 >> (uint8_t)xz->offset * 8)) {
			return HNY_EXTRACTION_STATUS_ERROR_XZ_BLOCK_INVALID_CRC32;
		}

		buffers->input++;
		buffers->inputsize--;
		if(xz->offset++ == 3) {
			xz->state = HNY_EXTRACTION_XZ_STREAM_BLOCK_OR_INDEX;
			xz->offset = 0;
		}

		return HNY_EXTRACTION_STATUS_OK;
	}
}

/*******************
 * XZ Stream Index *
 *******************/

static enum hny_extraction_status
hny_extraction_xz_decode_stream_index(struct hny_extraction_xz *xz, struct hny_extraction_xz_io *buffers) {
	const char *indexbegin = buffers->input;
	const char *indexend = buffers->input + buffers->inputsize;
	enum hny_extraction_status status = HNY_EXTRACTION_STATUS_OK;

	while(buffers->input < indexend) {
		const char *chunkbegin = buffers->input;

		switch(xz->index.state) {
		case HNY_EXTRACTION_XZ_STREAM_INDEX_RECORDS_COUNT:
			if(hny_extraction_xz_multibyte_integer(&xz->index.recordscount, &xz->multibyteindex, &buffers->input, indexend)) {
				if(xz->index.recordscount != xz->recordscount) {
					status = HNY_EXTRACTION_STATUS_ERROR_XZ_INDEX_INVALID_RECORDS_COUNT;
					goto hny_extraction_xz_decode_stream_index_end;
				}
				xz->multibyteindex = 0;
				xz->index.state = HNY_EXTRACTION_XZ_STREAM_INDEX_RECORDS_LIST_UNPADDED_SIZE;
				xz->index.recordsleft = xz->index.recordscount;
				xz->index.temporary = 0;
			}
			break;
		case HNY_EXTRACTION_XZ_STREAM_INDEX_RECORDS_LIST_UNPADDED_SIZE:
			if(xz->index.recordsleft == 0) {
				xz->index.indexcrc32 = hny_extraction_xz_crc32_end(xz->index.indexcrc32);
				if(xz->indexcrc32 != xz->index.indexcrc32) {
					status = HNY_EXTRACTION_STATUS_ERROR_XZ_INDEX_INVALID;
					goto hny_extraction_xz_decode_stream_index_end;
				}
				xz->index.state = HNY_EXTRACTION_XZ_STREAM_INDEX_PADDING;
			} else if(hny_extraction_xz_multibyte_integer(&xz->index.temporary, &xz->multibyteindex, &buffers->input, indexend)) {
				xz->multibyteindex = 0;
				xz->index.state = HNY_EXTRACTION_XZ_STREAM_INDEX_RECORDS_LIST_UNCOMPRESSED_SIZE;
				xz->index.indexcrc32 = hny_extraction_xz_crc32_update(xz->index.indexcrc32, (const uint8_t *)&xz->index.temporary, sizeof(xz->index.temporary));
				xz->index.temporary = 0;
			}
			break;
		case HNY_EXTRACTION_XZ_STREAM_INDEX_RECORDS_LIST_UNCOMPRESSED_SIZE:
			if(hny_extraction_xz_multibyte_integer(&xz->index.temporary, &xz->multibyteindex, &buffers->input, indexend)) {
				xz->index.recordsleft--;
				xz->index.state = HNY_EXTRACTION_XZ_STREAM_INDEX_RECORDS_LIST_UNPADDED_SIZE;
				xz->index.indexcrc32 = hny_extraction_xz_crc32_update(xz->index.indexcrc32, (const uint8_t *)&xz->index.temporary, sizeof(xz->index.temporary));
			}
			break;
		case HNY_EXTRACTION_XZ_STREAM_INDEX_PADDING:
			if((xz->offset & 0x03) != 0) {
				if(*buffers->input != 0) {
					status = HNY_EXTRACTION_STATUS_ERROR_XZ_INDEX_INVALID_PADDING;
					goto hny_extraction_xz_decode_stream_index_end;
				}
				buffers->input++;
			} else {
				xz->offset = 0;
				xz->index.state = HNY_EXTRACTION_XZ_STREAM_INDEX_CRC32;
				xz->index.crc32 = hny_extraction_xz_crc32_end(xz->index.crc32);
			}
			break;
		case HNY_EXTRACTION_XZ_STREAM_INDEX_CRC32:
			if((uint8_t)*buffers->input != (uint8_t)(xz->index.crc32 >> (uint8_t)xz->offset * 8)) {
				status = HNY_EXTRACTION_STATUS_ERROR_XZ_INDEX_INVALID_CRC32;
				goto hny_extraction_xz_decode_stream_index_end;
			}

			buffers->input++;
			if(xz->offset++ == 3) {
				xz->state = HNY_EXTRACTION_XZ_STREAM_FOOTER;
				xz->offset = 0;
				xz->footer.readcrc32 = 0;
				xz->footer.crc32 = hny_extraction_xz_crc32_init;
				goto hny_extraction_xz_decode_stream_index_end;
			}
			continue;
		}

		size_t decoded = buffers->input - chunkbegin;
		xz->index.crc32 = hny_extraction_xz_crc32_update(xz->index.crc32,
			(const uint8_t *)chunkbegin, decoded);
		xz->offset += decoded;
	}

hny_extraction_xz_decode_stream_index_end:
	buffers->inputsize -= buffers->input - indexbegin;

	return status;
}

/********************
 * XZ Stream Footer *
 ********************/

static enum hny_extraction_status
hny_extraction_xz_decode_stream_footer(struct hny_extraction_xz *xz, struct hny_extraction_xz_io *buffers) {
	const char *footerbegin = buffers->input;
	const char *footerend = buffers->input + HNY_EXTRACTION_XZ_MIN(HNY_EXTRACTION_XZ_STREAM_FOOTER_SIZE - xz->offset, buffers->inputsize);
	enum hny_extraction_status status = HNY_EXTRACTION_STATUS_OK;

	while(buffers->input < footerend) {
		const uint8_t byte = *buffers->input;

		switch(xz->offset >> 2) {
		case 0: /* CRC32 */
			xz->footer.readcrc32 |= byte << 8 * xz->offset;
			break;
		case 1: /* Backward size */
			xz->footer.backwardsize |= byte << 8 * (xz->offset - 4);
			xz->footer.crc32 = hny_extraction_xz_crc32_update(xz->footer.crc32, &byte, 1);
			break;
		case 2:
			switch(xz->offset & 0x03) {
			case 0: /* Stream flags 0 */
				if(byte != 0) {
					status = HNY_EXTRACTION_STATUS_ERROR_XZ_FOOTER_INVALID_STREAM_FLAGS;
					goto hny_extraction_xz_decode_stream_footer_end;
				}
				xz->footer.crc32 = hny_extraction_xz_crc32_update(xz->footer.crc32, &byte, 1);
				break;
			case 1: /* Stream flags 1 */
				if(byte != xz->header.flags) {
					status = HNY_EXTRACTION_STATUS_ERROR_XZ_FOOTER_INVALID_STREAM_FLAGS;
					goto hny_extraction_xz_decode_stream_footer_end;
				}
				xz->footer.crc32 = hny_extraction_xz_crc32_update(xz->footer.crc32, &byte, 1);
				break;
			case 2: /* Footer magic bytes 1 */
				if(byte != 'Y') {
					status = HNY_EXTRACTION_STATUS_ERROR_XZ_FOOTER_INVALID_MAGIC;
					goto hny_extraction_xz_decode_stream_footer_end;
				}
				break;
			case 3: /* Footer magic bytes 2 */
				if(byte != 'Z') {
					status = HNY_EXTRACTION_STATUS_ERROR_XZ_FOOTER_INVALID_MAGIC;
					goto hny_extraction_xz_decode_stream_footer_end;
				}

				if(xz->footer.readcrc32 != hny_extraction_xz_crc32_end(xz->footer.crc32)) {
					status = HNY_EXTRACTION_STATUS_ERROR_XZ_FOOTER_INVALID_CRC32;
					goto hny_extraction_xz_decode_stream_footer_end;
				}

				status = HNY_EXTRACTION_STATUS_END;
				goto hny_extraction_xz_decode_stream_footer_end;
			}
			break;
		}

		xz->offset++;
		buffers->input++;
	}

hny_extraction_xz_decode_stream_footer_end:
	buffers->inputsize -= buffers->input - footerbegin;

	return status;
}

