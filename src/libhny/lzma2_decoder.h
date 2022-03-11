/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LZMA2_DECODER_H
#define LZMA2_DECODER_H

#include <stddef.h>
#include <stdint.h>

enum lzma2_decoder_status {
	LZMA2_DECODER_STATUS_OK,
	LZMA2_DECODER_STATUS_END,
	LZMA2_DECODER_STATUS_ERROR_MEMORY_EXHAUSTED,
	LZMA2_DECODER_STATUS_ERROR_MEMORY_LIMIT,
	LZMA2_DECODER_STATUS_ERROR_INVALID_DICTIONARY_BITS,
	LZMA2_DECODER_STATUS_ERROR_CORRUPTED_DATA
};

enum lzma2_decoder_mode {
	LZMA2_DECODER_MODE_PREALLOC,
	LZMA2_DECODER_MODE_DYNAMIC
};

struct lzma2_stream {
	struct {
		const uint8_t *buffer;
		size_t position;
		size_t size;
	} input;
	struct {
		uint8_t *buffer;
		size_t position;
		size_t size;
	} output;
};

#define POS_STATES_MAX (1 << 4)

#define STATES 12

#define LIT_STATES 7

#define LITERAL_CODER_SIZE 0x300

#define LITERAL_CODERS_MAX (1 << 4)

#define MATCH_LEN_MIN 2

#define LEN_LOW_BITS 3
#define LEN_LOW_SYMBOLS (1 << LEN_LOW_BITS)
#define LEN_MID_BITS 3
#define LEN_MID_SYMBOLS (1 << LEN_MID_BITS)
#define LEN_HIGH_BITS 8
#define LEN_HIGH_SYMBOLS (1 << LEN_HIGH_BITS)
#define LEN_SYMBOLS (LEN_LOW_SYMBOLS + LEN_MID_SYMBOLS + LEN_HIGH_SYMBOLS)

#define MATCH_LEN_MAX (MATCH_LEN_MIN + LEN_SYMBOLS - 1)

#define DIST_STATES 4

#define DIST_SLOT_BITS 6
#define DIST_SLOTS (1 << DIST_SLOT_BITS)

#define DIST_MODEL_START 4

#define DIST_MODEL_END 14

#define FULL_DISTANCES_BITS (DIST_MODEL_END / 2)
#define FULL_DISTANCES (1 << FULL_DISTANCES_BITS)

#define ALIGN_BITS 4
#define ALIGN_SIZE (1 << ALIGN_BITS)
#define ALIGN_MASK (ALIGN_SIZE - 1)

#define PROBS_TOTAL (1846 + LITERAL_CODERS_MAX * LITERAL_CODER_SIZE)

#define REPS 4

#define LZMA_IN_REQUIRED 21

struct dictionary {
	uint8_t *buffer;
	size_t start;
	size_t position;
	size_t full;
	size_t limit;
	size_t end;
	uint32_t size;
	uint32_t sizelimit;
	uint32_t allocated;
	enum lzma2_decoder_mode mode;
};

struct range_decoder {
	uint32_t range;
	uint32_t code;
	uint32_t initbytesleft;
	struct {
		const uint8_t *buffer;
		size_t position;
		size_t limit;
	} input;
};

struct length_decoder {
	uint16_t choice;
	uint16_t choice2;
	uint16_t low[POS_STATES_MAX][LEN_LOW_SYMBOLS];
	uint16_t mid[POS_STATES_MAX][LEN_MID_SYMBOLS];
	uint16_t high[LEN_HIGH_SYMBOLS];
};

struct lzma {
	uint32_t rep0;
	uint32_t rep1;
	uint32_t rep2;
	uint32_t rep3;

	enum lzma_state {
		LZMA_STATE_LIT_LIT,
		LZMA_STATE_MATCH_LIT_LIT,
		LZMA_STATE_REP_LIT_LIT,
		LZMA_STATE_SHORTREP_LIT_LIT,
		LZMA_STATE_MATCH_LIT,
		LZMA_STATE_REP_LIT,
		LZMA_STATE_SHORTREP_LIT,
		LZMA_STATE_LIT_MATCH,
		LZMA_STATE_LIT_LONGREP,
		LZMA_STATE_LIT_SHORTREP,
		LZMA_STATE_NONLIT_MATCH,
		LZMA_STATE_NONLIT_REP
	} state;
	uint32_t len;
	uint32_t lc;
	uint32_t literal_pos_mask;
	uint32_t pos_mask;
	uint16_t ismatch[STATES][POS_STATES_MAX];
	uint16_t isrep[STATES];
	uint16_t isrep0[STATES];
	uint16_t isrep1[STATES];
	uint16_t isrep2[STATES];
	uint16_t isrep0long[STATES][POS_STATES_MAX];
	uint16_t distslot[DIST_STATES][DIST_SLOTS];
	uint16_t distspecial[FULL_DISTANCES - DIST_MODEL_END];
	uint16_t distalign[ALIGN_SIZE];
	struct length_decoder matchlength;
	struct length_decoder repeatedmatchlength;
	uint16_t literal[LITERAL_CODERS_MAX][LITERAL_CODER_SIZE];
};

struct lzma2_decoder {
	struct range_decoder rangedecoder;
	struct dictionary dictionary;
	struct {
		enum {
			LZMA2_CONTROL,
			LZMA2_UNCOMPRESSED_1,
			LZMA2_UNCOMPRESSED_2,
			LZMA2_COMPRESSED_0,
			LZMA2_COMPRESSED_1,
			LZMA2_PROPERTIES,
			LZMA2_LZMA_PREPARE,
			LZMA2_LZMA_RUN,
			LZMA2_COPY
		} sequence, next;
		uint32_t uncompressed;
		uint32_t compressed;
		struct {
			unsigned dictionaryreset : 1;
			unsigned properties : 1;
		} needed;
	} lzma2;
	struct lzma lzma;
	struct {
		uint32_t size;
		uint8_t buffer[3 * LZMA_IN_REQUIRED];
	} temporary;
};

int
lzma2_decoder_init(struct lzma2_decoder *decoder,
	enum lzma2_decoder_mode mode, uint32_t dictionarymax);

void
lzma2_decoder_deinit(struct lzma2_decoder *decoder);

int
lzma2_decoder_reset(struct lzma2_decoder *decoder, uint8_t properties);

enum lzma2_decoder_status
lzma2_decoder_decode(struct lzma2_decoder *decoder, struct lzma2_stream *stream);

/* LZMA2_DECODER_H */
#endif
