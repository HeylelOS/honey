/* SPDX-License-Identifier: BSD-3-Clause */
#include "xz_decoder.h"

#include <stdlib.h>
#include <stdbool.h>

#define XZ_STREAM_HEADER_SIZE 12
#define XZ_STREAM_FOOTER_SIZE 12
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/*********
 * CRC32 *
 *********/

#define CRC32_INIT ((uint32_t)-1)

static const uint32_t crc32_table[256] = {
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

static uint32_t
crc32_update(uint32_t crc32, const uint8_t *bytes, size_t size) {
	const uint8_t * const end = bytes + size;

	while (bytes != end) {
		const uint8_t index = crc32 ^ *bytes;

		crc32 = (crc32 >> 8) ^ crc32_table[index];

		bytes++;
	}

	return crc32;
}

static inline uint32_t
crc32_end(uint32_t crc32) {
	return ~crc32;
}

/*****************
 * XZ Multibytes *
 *****************/

static inline bool
xz_decode_multibyte_integer(uint64_t *value, size_t *index, const char **buffer, const char *bufferend) {
	uint8_t byte;

	do {
		byte = **buffer;
		*value |= (byte & 0x7F) << (*index * 7);
		++*index;
		++*buffer;
	} while ((byte & 0x80) != 0 && *index < 9 && *buffer < bufferend);

	return (byte & 0x80) == 0 || *index >= 9;
}

/********************
 * XZ Stream Header *
 ********************/


static enum xz_decoder_status
xz_decoder_decode_stream_header(struct xz_decoder *xz, struct xz_stream *stream) {
	static const uint8_t xz_stream_header_magic[] = { 0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00 };
	const char * const begin = stream->input.next, * const end = stream->input.next + MIN(XZ_STREAM_HEADER_SIZE - xz->offset, stream->input.available);
	enum xz_decoder_status status = XZ_DECODER_STATUS_OK;

	while (stream->input.next < end) {
		const uint8_t byte = *stream->input.next;

		if ((xz->offset & ~0x07) == 0) {
			switch (xz->offset) {
			default: /* magic number check */
				if (byte != xz_stream_header_magic[xz->offset]) {
					status = XZ_DECODER_STATUS_ERROR_HEADER_INVALID_MAGIC;
					goto xz_decoder_decode_stream_header_end;
				}
				break;
			case 6:
				if (byte != 0) {
					status = XZ_DECODER_STATUS_ERROR_HEADER_UNSUPPORTED_CHECK;
					goto xz_decoder_decode_stream_header_end;
				}
				break;
			case 7:
				if ((byte & 0xF0) != 0 || (byte != 0x00 && byte != 0x01)) { /* Doesn't support None or CRC32 check */
					status = XZ_DECODER_STATUS_ERROR_HEADER_UNSUPPORTED_CHECK;
					goto xz_decoder_decode_stream_header_end;
				}
				xz->header.flags = byte;
				/* The following constant is a precomputed crc32 from the static header */
				xz->header.crc32 = crc32_end(crc32_update(0x2DFD1072, &byte, 1));
				break;
			}
			xz->offset++;
		} else {
			/* Checking CRC32 one byte at a time */
			if (byte != (uint8_t)(xz->header.crc32 >> (uint8_t)(xz->offset - 8) * 8)) {
				status = XZ_DECODER_STATUS_ERROR_HEADER_INVALID_CRC32;
				goto xz_decoder_decode_stream_header_end;
			}

			xz->offset++;
			if (xz->offset == XZ_STREAM_HEADER_SIZE) {
				xz->state = XZ_DECODER_STATE_STREAM_BLOCK_OR_INDEX;
				xz->offset = 0;
				xz->recordscount = 0;
				xz->indexcrc32 = CRC32_INIT;
			}
		}
		stream->input.next++;
	}

xz_decoder_decode_stream_header_end:
	stream->input.available -= stream->input.next - begin;

	return status;
}

/****************************
 * XZ Stream Block or Index *
 ****************************/

static inline enum xz_decoder_status
xz_decoder_decode_stream_block_or_index(struct xz_decoder *xz, struct xz_stream *stream) {
	uint8_t encodedsize = *stream->input.next;

	if (encodedsize != 0) {
		xz->state = XZ_DECODER_STATE_STREAM_BLOCK;
		xz->block.state = XZ_DECODER_STATE_STREAM_BLOCK_HEADER;
		xz->block.header.state = XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FLAGS;
		xz->block.header.crc32 = crc32_update(CRC32_INIT, &encodedsize, 1);
		xz->block.header.realsize = (encodedsize + 1) * 4;
		xz->block.uncompressedsize = 0;
		xz->block.compressedsize = 0;
	} else {
		xz->state = XZ_DECODER_STATE_STREAM_INDEX;
		xz->multibyteindex = 0;
		xz->indexcrc32 = crc32_end(xz->indexcrc32);
		xz->index.state = XZ_DECODER_STATE_STREAM_INDEX_RECORDS_COUNT;
		xz->index.crc32 = crc32_update(CRC32_INIT, &encodedsize, 1);
		xz->index.recordscount = 0;
		xz->index.indexcrc32 = CRC32_INIT;
	}

	xz->offset++;
	stream->input.next++;
	stream->input.available--;

	return XZ_DECODER_STATUS_OK;
}

/*******************
 * XZ Stream Block *
 *******************/

static int
xz_decoder_decode_stream_block_header(struct xz_decoder *xz, struct xz_stream *stream) {
	const char * const begin = stream->input.next, * const end = stream->input.next + MIN(xz->block.header.realsize - xz->offset, stream->input.available);
	int retval = XZ_DECODER_STATUS_OK;

	while (stream->input.next < end) {
		const char *chunkbegin = stream->input.next;

		switch (xz->block.header.state) {
		case XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FLAGS:
			xz->block.header.flags = *stream->input.next;

			/* Only LZMA2 is supported here so we add 0x03 to limit to one filter */
			if ((xz->block.header.flags & (0x3C | 0x03)) != 0) {
				retval = XZ_DECODER_STATUS_ERROR_BLOCK_UNSUPPORTED_FLAG;
				goto xz_decoder_decode_stream_block_end;
			}

			switch (xz->block.header.flags & 0xC0) {
			case 0x00:
				xz->block.header.state = XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FILTER_FLAGS;
				break;
			case 0x80:
				xz->block.header.state = XZ_DECODER_STATE_STREAM_BLOCK_HEADER_UNCOMPRESSED_SIZE;
				break;
			default: /* 0x40, 0xC0 */
				xz->block.header.state = XZ_DECODER_STATE_STREAM_BLOCK_HEADER_COMPRESSED_SIZE;
				break;
			}

			xz->multibyteindex = 0;
			xz->block.header.compressedsize = 0;
			xz->block.header.uncompressedsize = 0;
			/* From this point, XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FILTER_FLAGS will be reached, initializing here */
			xz->block.header.filters.state = XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FILTER_FLAGS_ID;
			stream->input.next++;
			break;
		case XZ_DECODER_STATE_STREAM_BLOCK_HEADER_COMPRESSED_SIZE:
			if (xz_decode_multibyte_integer(&xz->block.header.compressedsize, &xz->multibyteindex, &stream->input.next, end)) {
				xz->multibyteindex = 0;
				if ((xz->block.header.flags & 0x80) != 0) {
					xz->block.header.state = XZ_DECODER_STATE_STREAM_BLOCK_HEADER_UNCOMPRESSED_SIZE;
				} else {
					xz->block.header.state = XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FILTER_FLAGS;
				}
			}
			break;
		case XZ_DECODER_STATE_STREAM_BLOCK_HEADER_UNCOMPRESSED_SIZE:
			if (xz_decode_multibyte_integer(&xz->block.header.uncompressedsize, &xz->multibyteindex, &stream->input.next, end)) {
				xz->block.header.state = XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FILTER_FLAGS;
			}
			break;
		case XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FILTER_FLAGS:
			switch (xz->block.header.filters.state) {
			case XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FILTER_FLAGS_ID:
				/* Only LZMA2 is supported here, so we only check for its flags */
				if ((uint8_t)*stream->input.next == 0x21) {
					xz->block.header.filters.state = XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FILTER_PROPERTIES_SIZE;
					stream->input.next++;
					break;
				} else {
					retval = XZ_DECODER_STATUS_ERROR_BLOCK_UNSUPPORTED_FILTER_FLAG;
					goto xz_decoder_decode_stream_block_end;
				}
			case XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FILTER_PROPERTIES_SIZE:
				/* Only LZMA2 is supported here, so we only check its properties size */
				if ((uint8_t)*stream->input.next == 0x01) {
					xz->block.header.filters.state = XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FILTER_PROPERTIES;
					stream->input.next++;
					break;
				} else {
					retval = XZ_DECODER_STATUS_ERROR_BLOCK_UNSUPPORTED_PROPERTIES_SIZE;
					goto xz_decoder_decode_stream_block_end;
				}
			case XZ_DECODER_STATE_STREAM_BLOCK_HEADER_FILTER_PROPERTIES: {
				/* Only LZMA2 is supported here, so we acquire dictionary size */
				const uint8_t byte = *stream->input.next;

				if ((byte & 0xC0) == 0x00) {
					xz->block.header.state = XZ_DECODER_STATE_STREAM_BLOCK_HEADER_PADDING;
					xz->block.header.filters.dictionarybits = byte & 0x3F;
					stream->input.next++;
					break;
				} else {
					retval = XZ_DECODER_STATUS_ERROR_BLOCK_UNSUPPORTED_PROPERTY;
					goto xz_decoder_decode_stream_block_end;
				}
			} break;
			default:
				abort();
			}
			break;
		case XZ_DECODER_STATE_STREAM_BLOCK_HEADER_PADDING:
			if (xz->offset < xz->block.header.realsize - sizeof (xz->block.header.crc32)) {
				if (*stream->input.next != 0) {
					retval = XZ_DECODER_STATUS_ERROR_BLOCK_INVALID_PADDING;
					goto xz_decoder_decode_stream_block_end;
				}
				stream->input.next++;
			} else {
				xz->block.header.state = XZ_DECODER_STATE_STREAM_BLOCK_HEADER_CRC32;
				xz->block.header.crc32 = crc32_end(xz->block.header.crc32);
			}
			break;
		case XZ_DECODER_STATE_STREAM_BLOCK_HEADER_CRC32:
			if ((uint8_t)*stream->input.next != (uint8_t)(xz->block.header.crc32 >> ((uint8_t)(xz->block.header.realsize - xz->offset - 1) ^ 0x03) * 8)) {
				retval = XZ_DECODER_STATUS_ERROR_BLOCK_INVALID_CRC32;
				goto xz_decoder_decode_stream_block_end;
			}

			stream->input.next++;
			xz->offset++;
			if (xz->offset == xz->block.header.realsize) {
				xz->block.state = XZ_DECODER_STATE_STREAM_BLOCK_DATA;
				if (xz->header.flags == 0x01) { /* CRC32 Check else None */
					xz->block.crc32 = CRC32_INIT;
				}

				if (lzma2_decoder_reset(&xz->lzma2, xz->block.header.filters.dictionarybits) != LZMA2_DECODER_STATUS_OK) {
					retval = XZ_DECODER_STATUS_ERROR_LZMA2_UNABLE_DICTIONARY_RESET;
				}

				goto xz_decoder_decode_stream_block_end;
			} else {
				continue;
			}
		}

		size_t decoded = stream->input.next - chunkbegin;
		xz->block.header.crc32 = crc32_update(xz->block.header.crc32, (const uint8_t *)chunkbegin, decoded);
		xz->offset += decoded;
	}

xz_decoder_decode_stream_block_end:
	stream->input.available -= stream->input.next - begin;

	return retval;
}

static inline int
xz_decoder_decode_stream_block_data(struct xz_decoder *xz, struct xz_stream *stream) {
	struct lzma2_stream lzma2stream = {
		.input = {
			.buffer = (const uint8_t *)stream->input.next,
			.size = stream->input.available,
		},
		.output = {
			.buffer = (uint8_t *)stream->output.next,
			.size = stream->output.available,
		},
	};
	enum lzma2_decoder_status lzma2status = lzma2_decoder_decode(&xz->lzma2, &lzma2stream);

	if (xz->header.flags == 0x01) { /* Check CRC32, else None */
		xz->block.crc32 = crc32_update(xz->block.crc32, lzma2stream.output.buffer, lzma2stream.output.position);
	}

	stream->input.next += lzma2stream.input.position;
	stream->input.available -= lzma2stream.input.position;
	stream->output.next += lzma2stream.output.position;
	stream->output.available -= lzma2stream.output.position;

	xz->block.compressedsize += lzma2stream.input.position;
	xz->block.uncompressedsize += lzma2stream.output.position;

	switch (lzma2status) {
	case LZMA2_DECODER_STATUS_END:
		if ((xz->block.header.flags & 0x40) != 0 && xz->block.header.compressedsize != xz->block.compressedsize) {
			return XZ_DECODER_STATUS_ERROR_BLOCK_INVALID_COMPRESSED_SIZE;
		}

		if ((xz->block.header.flags & 0x80) != 0 && xz->block.header.uncompressedsize != xz->block.uncompressedsize) {
			return XZ_DECODER_STATUS_ERROR_BLOCK_INVALID_UNCOMPRESSED_SIZE;
		}

		xz->block.state = XZ_DECODER_STATE_STREAM_BLOCK_PADDING;
		xz->offset = xz->block.compressedsize;
		if (xz->header.flags == 0x01) { /* Check CRC32, else None */
			xz->block.crc32 = crc32_end(xz->block.crc32);
		}
	case LZMA2_DECODER_STATUS_OK:
		/* fallthrough */
		return XZ_DECODER_STATUS_OK;
	case LZMA2_DECODER_STATUS_ERROR_MEMORY_EXHAUSTED:
		return XZ_DECODER_STATUS_ERROR_LZMA2_MEMORY_EXHAUSTED;
	case LZMA2_DECODER_STATUS_ERROR_MEMORY_LIMIT:
		return XZ_DECODER_STATUS_ERROR_LZMA2_MEMORY_LIMIT;
	case LZMA2_DECODER_STATUS_ERROR_INVALID_DICTIONARY_BITS:
		return XZ_DECODER_STATUS_ERROR_LZMA2_INVALID_DICTIONARY_BITS;
	case LZMA2_DECODER_STATUS_ERROR_CORRUPTED_DATA:
		return XZ_DECODER_STATUS_ERROR_LZMA2_CORRUPTED_DATA;
	default:
		abort();
	}
}

static enum xz_decoder_status
xz_decoder_decode_stream_block(struct xz_decoder *xz, struct xz_stream *stream) {

	switch (xz->block.state) {
	case XZ_DECODER_STATE_STREAM_BLOCK_HEADER:
		return xz_decoder_decode_stream_block_header(xz, stream);
	case XZ_DECODER_STATE_STREAM_BLOCK_DATA:
		return xz_decoder_decode_stream_block_data(xz, stream);
	case XZ_DECODER_STATE_STREAM_BLOCK_PADDING:
		if ((xz->offset & 0x03) == 0) {
			/* The following is computed instead of counted, check size is either 0 (none) or 4 (crc32) */
			uint64_t unpaddedsize = xz->block.header.realsize + xz->block.compressedsize + (xz->header.flags << 2);

			if (xz->header.flags == 0x01) {
				xz->block.state = XZ_DECODER_STATE_STREAM_BLOCK_CHECK;
			} else {
				xz->state = XZ_DECODER_STATE_STREAM_BLOCK_OR_INDEX;
			}
			xz->offset = 0;
			xz->recordscount++;
			xz->indexcrc32 = crc32_update(xz->indexcrc32, (const uint8_t *)&unpaddedsize, sizeof (unpaddedsize));
			xz->indexcrc32 = crc32_update(xz->indexcrc32, (const uint8_t *)&xz->block.uncompressedsize, sizeof (xz->block.uncompressedsize));
		} else {
			if (*stream->input.next != 0) {
				return XZ_DECODER_STATUS_ERROR_BLOCK_INVALID_PADDING;
			}
			xz->offset++;
			stream->input.next++;
			stream->input.available--;
		}
		return XZ_DECODER_STATUS_OK;
	case XZ_DECODER_STATE_STREAM_BLOCK_CHECK:
		if ((uint8_t)*stream->input.next != (uint8_t)(xz->block.crc32 >> (uint8_t)xz->offset * 8)) {
			return XZ_DECODER_STATUS_ERROR_BLOCK_INVALID_CRC32;
		}

		stream->input.next++;
		stream->input.available--;
		if (xz->offset++ == 3) {
			xz->state = XZ_DECODER_STATE_STREAM_BLOCK_OR_INDEX;
			xz->offset = 0;
		}

		return XZ_DECODER_STATUS_OK;
	default:
		abort();
	}
}

/*******************
 * XZ Stream Index *
 *******************/

static enum xz_decoder_status
xz_decoder_decode_stream_index(struct xz_decoder *xz, struct xz_stream *stream) {
	const char *indexbegin = stream->input.next;
	const char *indexend = stream->input.next + stream->input.available;
	enum xz_decoder_status status = XZ_DECODER_STATUS_OK;

	while (stream->input.next < indexend) {
		const char *chunkbegin = stream->input.next;

		switch (xz->index.state) {
		case XZ_DECODER_STATE_STREAM_INDEX_RECORDS_COUNT:
			if (xz_decode_multibyte_integer(&xz->index.recordscount, &xz->multibyteindex, &stream->input.next, indexend)) {
				if (xz->index.recordscount != xz->recordscount) {
					status = XZ_DECODER_STATUS_ERROR_INDEX_INVALID_RECORDS_COUNT;
					goto xz_decoder_decode_stream_index_end;
				}
				xz->multibyteindex = 0;
				xz->index.state = XZ_DECODER_STATE_STREAM_INDEX_RECORDS_LIST_UNPADDED_SIZE;
				xz->index.recordsleft = xz->index.recordscount;
				xz->index.temporary = 0;
			}
			break;
		case XZ_DECODER_STATE_STREAM_INDEX_RECORDS_LIST_UNPADDED_SIZE:
			if (xz->index.recordsleft == 0) {
				xz->index.indexcrc32 = crc32_end(xz->index.indexcrc32);
				if (xz->indexcrc32 != xz->index.indexcrc32) {
					status = XZ_DECODER_STATUS_ERROR_INDEX_INVALID;
					goto xz_decoder_decode_stream_index_end;
				}
				xz->index.state = XZ_DECODER_STATE_STREAM_INDEX_PADDING;
			} else if (xz_decode_multibyte_integer(&xz->index.temporary, &xz->multibyteindex, &stream->input.next, indexend)) {
				xz->multibyteindex = 0;
				xz->index.state = XZ_DECODER_STATE_STREAM_INDEX_RECORDS_LIST_UNCOMPRESSED_SIZE;
				xz->index.indexcrc32 = crc32_update(xz->index.indexcrc32, (const uint8_t *)&xz->index.temporary, sizeof (xz->index.temporary));
				xz->index.temporary = 0;
			}
			break;
		case XZ_DECODER_STATE_STREAM_INDEX_RECORDS_LIST_UNCOMPRESSED_SIZE:
			if (xz_decode_multibyte_integer(&xz->index.temporary, &xz->multibyteindex, &stream->input.next, indexend)) {
				xz->index.recordsleft--;
				xz->index.state = XZ_DECODER_STATE_STREAM_INDEX_RECORDS_LIST_UNPADDED_SIZE;
				xz->index.indexcrc32 = crc32_update(xz->index.indexcrc32, (const uint8_t *)&xz->index.temporary, sizeof (xz->index.temporary));
			}
			break;
		case XZ_DECODER_STATE_STREAM_INDEX_PADDING:
			if ((xz->offset & 0x03) != 0) {
				if (*stream->input.next != 0) {
					status = XZ_DECODER_STATUS_ERROR_INDEX_INVALID_PADDING;
					goto xz_decoder_decode_stream_index_end;
				}
				stream->input.next++;
			} else {
				xz->offset = 0;
				xz->index.state = XZ_DECODER_STATE_STREAM_INDEX_CRC32;
				xz->index.crc32 = crc32_end(xz->index.crc32);
			}
			break;
		case XZ_DECODER_STATE_STREAM_INDEX_CRC32:
			if ((uint8_t)*stream->input.next != (uint8_t)(xz->index.crc32 >> (uint8_t)xz->offset * 8)) {
				status = XZ_DECODER_STATUS_ERROR_INDEX_INVALID_CRC32;
				goto xz_decoder_decode_stream_index_end;
			}

			stream->input.next++;
			if (xz->offset++ == 3) {
				xz->state = XZ_DECODER_STATE_STREAM_FOOTER;
				xz->offset = 0;
				xz->footer.readcrc32 = 0;
				xz->footer.crc32 = CRC32_INIT;
				goto xz_decoder_decode_stream_index_end;
			}
			continue;
		}

		size_t decoded = stream->input.next - chunkbegin;
		xz->index.crc32 = crc32_update(xz->index.crc32,
			(const uint8_t *)chunkbegin, decoded);
		xz->offset += decoded;
	}

xz_decoder_decode_stream_index_end:
	stream->input.available -= stream->input.next - indexbegin;

	return status;
}

/********************
 * XZ Stream Footer *
 ********************/

static enum xz_decoder_status
xz_decoder_decode_stream_footer(struct xz_decoder *xz, struct xz_stream *stream) {
	const char *footerbegin = stream->input.next;
	const char *footerend = stream->input.next + MIN(XZ_STREAM_FOOTER_SIZE - xz->offset, stream->input.available);
	enum xz_decoder_status status = XZ_DECODER_STATUS_OK;

	while (stream->input.next < footerend) {
		const uint8_t byte = *stream->input.next;

		switch (xz->offset >> 2) {
		case 0: /* CRC32 */
			xz->footer.readcrc32 |= byte << 8 * xz->offset;
			break;
		case 1: /* Backward size */
			xz->footer.backwardsize |= byte << 8 * (xz->offset - 4);
			xz->footer.crc32 = crc32_update(xz->footer.crc32, &byte, 1);
			break;
		case 2:
			switch (xz->offset & 0x03) {
			case 0: /* Stream flags 0 */
				if (byte != 0) {
					status = XZ_DECODER_STATUS_ERROR_FOOTER_INVALID_STREAM_FLAGS;
					goto xz_decoder_decode_stream_footer_end;
				}
				xz->footer.crc32 = crc32_update(xz->footer.crc32, &byte, 1);
				break;
			case 1: /* Stream flags 1 */
				if (byte != xz->header.flags) {
					status = XZ_DECODER_STATUS_ERROR_FOOTER_INVALID_STREAM_FLAGS;
					goto xz_decoder_decode_stream_footer_end;
				}
				xz->footer.crc32 = crc32_update(xz->footer.crc32, &byte, 1);
				break;
			case 2: /* Footer magic bytes 1 */
				if (byte != 'Y') {
					status = XZ_DECODER_STATUS_ERROR_FOOTER_INVALID_MAGIC;
					goto xz_decoder_decode_stream_footer_end;
				}
				break;
			case 3: /* Footer magic bytes 2 */
				if (byte != 'Z') {
					status = XZ_DECODER_STATUS_ERROR_FOOTER_INVALID_MAGIC;
					goto xz_decoder_decode_stream_footer_end;
				}

				if (xz->footer.readcrc32 != crc32_end(xz->footer.crc32)) {
					status = XZ_DECODER_STATUS_ERROR_FOOTER_INVALID_CRC32;
					goto xz_decoder_decode_stream_footer_end;
				}

				status = XZ_DECODER_STATUS_END;
				goto xz_decoder_decode_stream_footer_end;
			}
			break;
		}

		xz->offset++;
		stream->input.next++;
	}

xz_decoder_decode_stream_footer_end:
	stream->input.available -= stream->input.next - footerbegin;

	return status;
}

int
xz_decoder_init(struct xz_decoder *xz, size_t dictionarymax) {

	xz->state = XZ_DECODER_STATE_STREAM_HEADER;
	xz->offset = 0;

	return lzma2_decoder_init(&xz->lzma2, LZMA2_DECODER_MODE_DYNAMIC, MIN(dictionarymax, UINT32_MAX));
}

void
xz_decoder_deinit(struct xz_decoder *xz) {
	lzma2_decoder_deinit(&xz->lzma2);
}

enum xz_decoder_status
xz_decoder_decode(struct xz_decoder *xz, struct xz_stream *stream) {
	enum xz_decoder_status status = XZ_DECODER_STATUS_OK;

	while (status == XZ_DECODER_STATUS_OK && stream->input.available != 0 && stream->output.available != 0) {
		switch (xz->state) {
		case XZ_DECODER_STATE_STREAM_HEADER:
			status = xz_decoder_decode_stream_header(xz, stream);
			break;
		case XZ_DECODER_STATE_STREAM_BLOCK_OR_INDEX:
			status = xz_decoder_decode_stream_block_or_index(xz, stream);
			break;
		case XZ_DECODER_STATE_STREAM_BLOCK:
			status = xz_decoder_decode_stream_block(xz, stream);
			break;
		case XZ_DECODER_STATE_STREAM_INDEX:
			status = xz_decoder_decode_stream_index(xz, stream);
			break;
		case XZ_DECODER_STATE_STREAM_FOOTER:
			status = xz_decoder_decode_stream_footer(xz, stream);
			break;
		case XZ_DECODER_STATE_STREAM_END:
			status = XZ_DECODER_STATUS_END;
			break;
		default:
			abort();
		}
	}

	return status;
}
