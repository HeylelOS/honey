/* SPDX-License-Identifier: BSD-3-Clause */
/*
	Derivated from an original work of Lasse Collin and Igor Pavlov
	put into public domain.
*/
#include "lzma2_decoder.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define RANGE_DECODER_SHIFT_BITS 8
#define RANGE_DECODER_TOP_BITS 24
#define RANGE_DECODER_TOP_VALUE (1 << RANGE_DECODER_TOP_BITS)
#define RANGE_DECODER_BIT_MODEL_TOTAL_BITS 11
#define RANGE_DECODER_BIT_MODEL_TOTAL (1 << RANGE_DECODER_BIT_MODEL_TOTAL_BITS)
#define RANGE_DECODER_MOVE_BITS 5

#define RANGE_DECODER_INIT_BYTES 5

#define MIN(x, y) ((x) < (y) ? (x) : (y))

static inline void
lzma_state_literal(enum lzma_state *state) {
	if (*state <= LZMA_STATE_SHORTREP_LIT_LIT) {
		*state = LZMA_STATE_LIT_LIT;
	} else if (*state <= LZMA_STATE_LIT_SHORTREP) {
		*state -= 3;
	} else {
		*state -= 6;
	}
}

static inline void
lzma_state_match(enum lzma_state *state) {
	*state = *state < LIT_STATES ? LZMA_STATE_LIT_MATCH : LZMA_STATE_NONLIT_MATCH;
}

static inline void
lzma_state_long_rep(enum lzma_state *state) {
	*state = *state < LIT_STATES ? LZMA_STATE_LIT_LONGREP : LZMA_STATE_NONLIT_REP;
}

static inline void
lzma_state_short_rep(enum lzma_state *state) {
	*state = *state < LIT_STATES ? LZMA_STATE_LIT_SHORTREP : LZMA_STATE_NONLIT_REP;
}

static inline bool
lzma_state_is_literal(enum lzma_state state) {
	return state < LIT_STATES;
}

static inline uint32_t
lzma_get_dist_state(uint32_t length) {
	return length < DIST_STATES + MATCH_LEN_MIN ? length - MATCH_LEN_MIN : DIST_STATES - 1;
}

/**************
 * Dictionary *
 **************/

static void
dictionary_reset(struct dictionary *dictionary, struct lzma2_stream *stream) {
	dictionary->start = 0;
	dictionary->position = 0;
	dictionary->limit = 0;
	dictionary->full = 0;
}

static void
dictionary_limit(struct dictionary *dictionary, size_t limit) {
	if (dictionary->end - dictionary->position <= limit) {
		dictionary->limit = dictionary->end;
	} else {
		dictionary->limit = dictionary->position + limit;
	}
}

static inline bool
dictionary_has_space(const struct dictionary *dictionary) {
	return dictionary->position < dictionary->limit;
}

static inline uint32_t
dictionary_get(const struct dictionary *dictionary, uint32_t dist) {
	size_t offset = dictionary->position - dist - 1;

	if (dist >= dictionary->position) {
		offset += dictionary->end;
	}

	return dictionary->full > 0 ? dictionary->buffer[offset] : 0;
}

static inline void
dictionary_put(struct dictionary *dictionary, uint8_t byte) {
	dictionary->buffer[dictionary->position++] = byte;

	if (dictionary->full < dictionary->position) {
		dictionary->full = dictionary->position;
	}
}

static bool
dictionary_repeat(struct dictionary *dictionary, uint32_t *len, uint32_t dist) {
	size_t back;
	uint32_t left;

	if (dist >= dictionary->full || dist >= dictionary->size) {
		return false;
	}

	left = MIN(dictionary->limit - dictionary->position, *len);
	*len -= left;

	back = dictionary->position - dist - 1;
	if (dist >= dictionary->position) {
		back += dictionary->end;
	}

	do {
		dictionary->buffer[dictionary->position++] = dictionary->buffer[back++];
		if (back == dictionary->end) {
			back = 0;
		}
	} while (--left > 0);

	if (dictionary->full < dictionary->position) {
		dictionary->full = dictionary->position;
	}

	return true;
}

static void
dictionary_uncompressed(struct dictionary *dictionary, struct lzma2_stream *stream, uint32_t *left) {
	size_t copysize;

	while (*left > 0 && stream->input.position < stream->input.size && stream->output.position < stream->output.size) {

		copysize = MIN(stream->input.size - stream->input.position, stream->output.size - stream->output.position);

		if (copysize > dictionary->end - dictionary->position) {
			copysize = dictionary->end - dictionary->position;
		}

		if (copysize > *left) {
			copysize = *left;
		}

		*left -= copysize;

		memcpy(dictionary->buffer + dictionary->position, stream->input.buffer + stream->input.position, copysize);
		dictionary->position += copysize;

		if (dictionary->full < dictionary->position) {
			dictionary->full = dictionary->position;
		}

		if (dictionary->position == dictionary->end) {
			dictionary->position = 0;
		}

		memcpy(stream->output.buffer + stream->output.position, stream->input.buffer + stream->input.position, copysize);

		dictionary->start = dictionary->position;

		stream->output.position += copysize;
		stream->input.position += copysize;
	}
}

static uint32_t
dictionary_flush(struct dictionary *dictionary, struct lzma2_stream *stream) {
	const size_t copysize = dictionary->position - dictionary->start;

	if (dictionary->position == dictionary->end) {
		dictionary->position = 0;
	}

	memcpy(stream->output.buffer + stream->output.position, dictionary->buffer + dictionary->start, copysize);

	dictionary->start = dictionary->position;
	stream->output.position += copysize;

	return copysize;
}

/*****************
 * Range decoder *
 *****************/

static void
range_decoder_reset(struct range_decoder *rangedecoder) {
	rangedecoder->range = (uint32_t)-1;
	rangedecoder->code = 0;
	rangedecoder->initbytesleft = RANGE_DECODER_INIT_BYTES;
}

static bool
range_decoder_read_init(struct range_decoder *rangedecoder, struct lzma2_stream *stream) {

	while (rangedecoder->initbytesleft > 0) {

		if (stream->input.position == stream->input.size) {
			return false;
		}

		rangedecoder->code = (rangedecoder->code << 8) + stream->input.buffer[stream->input.position++];
		rangedecoder->initbytesleft--;
	}

	return true;
}

static inline bool
range_decoder_limit_exceeded(const struct range_decoder *rangedecoder) {
	return rangedecoder->input.position > rangedecoder->input.limit;
}

static inline bool
range_decoder_is_finished(const struct range_decoder *rangedecoder) {
	return rangedecoder->code == 0;
}

static inline void
range_decoder_normalize(struct range_decoder *rangedecoder) {
	if (rangedecoder->range < RANGE_DECODER_TOP_VALUE) {
		rangedecoder->range <<= RANGE_DECODER_SHIFT_BITS;
		rangedecoder->code = (rangedecoder->code << RANGE_DECODER_SHIFT_BITS) + rangedecoder->input.buffer[rangedecoder->input.position++];
	}
}

static inline int
range_decoder_bit(struct range_decoder *rangedecoder, uint16_t *prob) {
	uint32_t bound;
	int bit;

	range_decoder_normalize(rangedecoder);
	bound = (rangedecoder->range >> RANGE_DECODER_BIT_MODEL_TOTAL_BITS) * *prob;

	if (rangedecoder->code < bound) {
		rangedecoder->range = bound;
		*prob += (RANGE_DECODER_BIT_MODEL_TOTAL - *prob) >> RANGE_DECODER_MOVE_BITS;
		bit = 0;
	} else {
		rangedecoder->range -= bound;
		rangedecoder->code -= bound;
		*prob -= *prob >> RANGE_DECODER_MOVE_BITS;
		bit = 1;
	}

	return bit;
}

static inline uint32_t
range_decoder_bittree(struct range_decoder *rangedecoder, uint16_t *probs, uint32_t limit) {
	uint32_t symbol = 1;

	do {
		if (range_decoder_bit(rangedecoder, probs + symbol)) {
			symbol = (symbol << 1) + 1;
		} else {
			symbol <<= 1;
		}
	} while (symbol < limit);

	return symbol;
}

static inline void
range_decoder_bittree_reverse(struct range_decoder *rangedecoder, uint16_t *probs, uint32_t *dest, uint32_t limit) {
	uint32_t symbol = 1;
	uint32_t i = 0;

	do {
		if (range_decoder_bit(rangedecoder, &probs[symbol])) {
			symbol = (symbol << 1) + 1;
			*dest += 1 << i;
		} else {
			symbol <<= 1;
		}
	} while (++i < limit);
}

static inline void
range_decoder_direct(struct range_decoder *rangedecoder, uint32_t *dest, uint32_t limit) {
	uint32_t mask;

	do {
		range_decoder_normalize(rangedecoder);
		rangedecoder->range >>= 1;
		rangedecoder->code -= rangedecoder->range;
		mask = (uint32_t)0 - (rangedecoder->code >> 31);
		rangedecoder->code += rangedecoder->range & mask;
		*dest = (*dest << 1) + (mask + 1);
	} while (--limit > 0);
}

/********
 * LZMA *
 ********/

static uint16_t *
lzma_literal_probs(struct lzma2_decoder *decoder) {
	const uint32_t previousbyte = dictionary_get(&decoder->dictionary, 0);
	const uint32_t low = previousbyte >> (8 - decoder->lzma.lc);
	const uint32_t high = (decoder->dictionary.position & decoder->lzma.literal_pos_mask) << decoder->lzma.lc;

	return decoder->lzma.literal[low + high];
}

static void
lzma_literal(struct lzma2_decoder *decoder) {
	uint16_t * const probs = lzma_literal_probs(decoder);
	uint32_t symbol;
	uint32_t matchbyte;
	uint32_t matchbit;
	uint32_t offset;
	uint32_t i;

	if (lzma_state_is_literal(decoder->lzma.state)) {
		symbol = range_decoder_bittree(&decoder->rangedecoder, probs, 0x100);
	} else {
		symbol = 1;
		matchbyte = dictionary_get(&decoder->dictionary, decoder->lzma.rep0) << 1;
		offset = 0x100;

		do {
			matchbit = matchbyte & offset;
			matchbyte <<= 1;
			i = offset + matchbit + symbol;

			if (range_decoder_bit(&decoder->rangedecoder, probs + i)) {
				symbol = (symbol << 1) + 1;
				offset &= matchbit;
			} else {
				symbol <<= 1;
				offset &= ~matchbit;
			}
		} while (symbol < 0x100);
	}

	dictionary_put(&decoder->dictionary, (uint8_t)symbol);
	lzma_state_literal(&decoder->lzma.state);
}

static void
lzma_length_decode(struct lzma2_decoder *decoder, struct length_decoder *l, uint32_t posstate) {
	uint16_t *probs;
	uint32_t limit;

	if (!range_decoder_bit(&decoder->rangedecoder, &l->choice)) {
		probs = l->low[posstate];
		limit = LEN_LOW_SYMBOLS;
		decoder->lzma.len = MATCH_LEN_MIN;
	} else {
		if (!range_decoder_bit(&decoder->rangedecoder, &l->choice2)) {
			probs = l->mid[posstate];
			limit = LEN_MID_SYMBOLS;
			decoder->lzma.len = MATCH_LEN_MIN + LEN_LOW_SYMBOLS;
		} else {
			probs = l->high;
			limit = LEN_HIGH_SYMBOLS;
			decoder->lzma.len = MATCH_LEN_MIN + LEN_LOW_SYMBOLS + LEN_MID_SYMBOLS;
		}
	}

	decoder->lzma.len += range_decoder_bittree(&decoder->rangedecoder, probs, limit) - limit;
}

static void
lzma_match(struct lzma2_decoder *decoder, uint32_t posstate) {
	uint16_t *probs;
	uint32_t distslot;
	uint32_t limit;

	lzma_state_match(&decoder->lzma.state);

	decoder->lzma.rep3 = decoder->lzma.rep2;
	decoder->lzma.rep2 = decoder->lzma.rep1;
	decoder->lzma.rep1 = decoder->lzma.rep0;

	lzma_length_decode(decoder, &decoder->lzma.matchlength, posstate);

	probs = decoder->lzma.distslot[lzma_get_dist_state(decoder->lzma.len)];
	distslot = range_decoder_bittree(&decoder->rangedecoder, probs, DIST_SLOTS) - DIST_SLOTS;

	if (distslot < DIST_MODEL_START) {
		decoder->lzma.rep0 = distslot;
	} else {
		limit = (distslot >> 1) - 1;
		decoder->lzma.rep0 = 2 + (distslot & 1);

		if (distslot < DIST_MODEL_END) {
			decoder->lzma.rep0 <<= limit;
			probs = decoder->lzma.distspecial + decoder->lzma.rep0 - distslot - 1;
			range_decoder_bittree_reverse(&decoder->rangedecoder, probs, &decoder->lzma.rep0, limit);
		} else {
			range_decoder_direct(&decoder->rangedecoder, &decoder->lzma.rep0, limit - ALIGN_BITS);
			decoder->lzma.rep0 <<= ALIGN_BITS;
			range_decoder_bittree_reverse(&decoder->rangedecoder, decoder->lzma.distalign, &decoder->lzma.rep0, ALIGN_BITS);
		}
	}
}

static void
lzma_rep_match(struct lzma2_decoder *decoder, uint32_t posstate) {
	uint32_t tmp;

	if (!range_decoder_bit(&decoder->rangedecoder, decoder->lzma.isrep0 + decoder->lzma.state)) {
		if (!range_decoder_bit(&decoder->rangedecoder, decoder->lzma.isrep0long[decoder->lzma.state] + posstate)) {
			lzma_state_short_rep(&decoder->lzma.state);
			decoder->lzma.len = 1;
			return;
		}
	} else {
		if (!range_decoder_bit(&decoder->rangedecoder, decoder->lzma.isrep1 + decoder->lzma.state)) {
			tmp = decoder->lzma.rep1;
		} else {
			if (!range_decoder_bit(&decoder->rangedecoder, decoder->lzma.isrep2 + decoder->lzma.state)) {
				tmp = decoder->lzma.rep2;
			} else {
				tmp = decoder->lzma.rep3;
				decoder->lzma.rep3 = decoder->lzma.rep2;
			}

			decoder->lzma.rep2 = decoder->lzma.rep1;
		}

		decoder->lzma.rep1 = decoder->lzma.rep0;
		decoder->lzma.rep0 = tmp;
	}

	lzma_state_long_rep(&decoder->lzma.state);
	lzma_length_decode(decoder, &decoder->lzma.repeatedmatchlength, posstate);
}

static bool
lzma_main(struct lzma2_decoder *decoder) {
	uint32_t posstate;

	if (dictionary_has_space(&decoder->dictionary) && decoder->lzma.len > 0) {
		dictionary_repeat(&decoder->dictionary, &decoder->lzma.len, decoder->lzma.rep0);
	}

	while (dictionary_has_space(&decoder->dictionary) && !range_decoder_limit_exceeded(&decoder->rangedecoder)) {
		posstate = decoder->dictionary.position & decoder->lzma.pos_mask;

		if (!range_decoder_bit(&decoder->rangedecoder, decoder->lzma.ismatch[decoder->lzma.state] + posstate)) {
			lzma_literal(decoder);
		} else {
			if (range_decoder_bit(&decoder->rangedecoder, decoder->lzma.isrep + decoder->lzma.state)) {
				lzma_rep_match(decoder, posstate);
			} else {
				lzma_match(decoder, posstate);
			}

			if (!dictionary_repeat(&decoder->dictionary, &decoder->lzma.len, decoder->lzma.rep0)) {
				return false;
			}
		}
	}

	range_decoder_normalize(&decoder->rangedecoder);

	return true;
}

static void
lzma_reset(struct lzma2_decoder *decoder) {
	uint16_t * const probs = *decoder->lzma.ismatch;

	decoder->lzma.state = LZMA_STATE_LIT_LIT;
	decoder->lzma.rep0 = 0;
	decoder->lzma.rep1 = 0;
	decoder->lzma.rep2 = 0;
	decoder->lzma.rep3 = 0;

	for (unsigned int i = 0; i < PROBS_TOTAL; i++) {
		probs[i] = RANGE_DECODER_BIT_MODEL_TOTAL / 2;
	}

	range_decoder_reset(&decoder->rangedecoder);
}

static bool
lzma_props(struct lzma2_decoder *decoder, uint8_t props) {

	if (props > (4 * 5 + 4) * 9 + 8) {
		return false;
	}

	decoder->lzma.pos_mask = 0;
	while (props >= 9 * 5) {
		props -= 9 * 5;
		decoder->lzma.pos_mask++;
	}

	decoder->lzma.pos_mask = (1 << decoder->lzma.pos_mask) - 1;

	decoder->lzma.literal_pos_mask = 0;
	while (props >= 9) {
		props -= 9;
		decoder->lzma.literal_pos_mask++;
	}

	decoder->lzma.lc = props;

	if (decoder->lzma.lc + decoder->lzma.literal_pos_mask > 4) {
		return false;
	}

	decoder->lzma.literal_pos_mask = (1 << decoder->lzma.literal_pos_mask) - 1;

	lzma_reset(decoder);

	return true;
}

/*********
 * LZMA2 *
 *********/

static bool
lzma2_decoder_lzma(struct lzma2_decoder *decoder, struct lzma2_stream *stream) {
	size_t available;
	uint32_t tmp;

	available = stream->input.size - stream->input.position;
	if (decoder->temporary.size > 0 || decoder->lzma2.compressed == 0) {
		tmp = 2 * LZMA_IN_REQUIRED - decoder->temporary.size;

		if (tmp > decoder->lzma2.compressed - decoder->temporary.size) {
			tmp = decoder->lzma2.compressed - decoder->temporary.size;
		}

		if (tmp > available) {
			tmp = available;
		}

		memcpy(decoder->temporary.buffer + decoder->temporary.size, stream->input.buffer + stream->input.position, tmp);

		if (decoder->temporary.size + tmp == decoder->lzma2.compressed) {
			memset(decoder->temporary.buffer + decoder->temporary.size + tmp, 0, sizeof (decoder->temporary.buffer) - decoder->temporary.size - tmp);
			decoder->rangedecoder.input.limit = decoder->temporary.size + tmp;
		} else if (decoder->temporary.size + tmp < LZMA_IN_REQUIRED) {
			decoder->temporary.size += tmp;
			stream->input.position += tmp;
			return true;
		} else {
			decoder->rangedecoder.input.limit = decoder->temporary.size + tmp - LZMA_IN_REQUIRED;
		}

		decoder->rangedecoder.input.buffer = decoder->temporary.buffer;
		decoder->rangedecoder.input.position = 0;

		if (!lzma_main(decoder) || decoder->rangedecoder.input.position > decoder->temporary.size + tmp) {
			return false;
		}

		decoder->lzma2.compressed -= decoder->rangedecoder.input.position;

		if (decoder->rangedecoder.input.position < decoder->temporary.size) {
			decoder->temporary.size -= decoder->rangedecoder.input.position;
			memmove(decoder->temporary.buffer, decoder->temporary.buffer + decoder->rangedecoder.input.position, decoder->temporary.size);
			return true;
		}

		stream->input.position += decoder->rangedecoder.input.position - decoder->temporary.size;
		decoder->temporary.size = 0;
	}

	available = stream->input.size - stream->input.position;
	if (available >= LZMA_IN_REQUIRED) {
		decoder->rangedecoder.input.buffer = stream->input.buffer;
		decoder->rangedecoder.input.position = stream->input.position;

		if (available >= decoder->lzma2.compressed + LZMA_IN_REQUIRED) {
			decoder->rangedecoder.input.limit = stream->input.position + decoder->lzma2.compressed;
		} else {
			decoder->rangedecoder.input.limit = stream->input.size - LZMA_IN_REQUIRED;
		}

		if (!lzma_main(decoder)) {
			return false;
		}

		available = decoder->rangedecoder.input.position - stream->input.position;
		if (available > decoder->lzma2.compressed) {
			return false;
		}

		decoder->lzma2.compressed -= available;
		stream->input.position = decoder->rangedecoder.input.position;
	}

	available = stream->input.size - stream->input.position;
	if (available < LZMA_IN_REQUIRED) {
		if (available > decoder->lzma2.compressed) {
			available = decoder->lzma2.compressed;
		}

		memcpy(decoder->temporary.buffer, stream->input.buffer + stream->input.position, available);
		decoder->temporary.size = available;
		stream->input.position += available;
	}

	return true;
}

int
lzma2_decoder_init(struct lzma2_decoder *decoder, enum lzma2_decoder_mode mode, uint32_t dictionarymax) {
	decoder->dictionary.mode = mode;
	decoder->dictionary.sizelimit = dictionarymax;

	if (mode == LZMA2_DECODER_MODE_PREALLOC) {
		decoder->dictionary.buffer = malloc(dictionarymax);
		if (decoder->dictionary.buffer == NULL) {
			free(decoder);
			return -1;
		}
	} else if (mode == LZMA2_DECODER_MODE_DYNAMIC) {
		decoder->dictionary.buffer = NULL;
		decoder->dictionary.allocated = 0;
	}

	return 0;
}

void
lzma2_decoder_deinit(struct lzma2_decoder *decoder) {
	free(decoder->dictionary.buffer);
}

int
lzma2_decoder_reset(struct lzma2_decoder *decoder, uint8_t props) {

	if (props > 40) {
		return LZMA2_DECODER_STATUS_ERROR_INVALID_DICTIONARY_BITS;
	}

	if (props == 40) {
		decoder->dictionary.size = UINT32_MAX;
	} else {
		decoder->dictionary.size = 2 + (props & 1);
		decoder->dictionary.size <<= (props >> 1) + 11;
	}

	if (decoder->dictionary.size > decoder->dictionary.sizelimit) {
		return LZMA2_DECODER_STATUS_ERROR_MEMORY_LIMIT;
	}

	decoder->dictionary.end = decoder->dictionary.size;

	if (decoder->dictionary.mode == LZMA2_DECODER_MODE_DYNAMIC) {
		if (decoder->dictionary.allocated < decoder->dictionary.size) {
			uint8_t * const newbuffer = realloc(decoder->dictionary.buffer, decoder->dictionary.size);

			if (newbuffer == NULL) {
				free(decoder->dictionary.buffer);
				decoder->dictionary.allocated = 0;
				decoder->dictionary.buffer = NULL;
				return LZMA2_DECODER_STATUS_ERROR_MEMORY_EXHAUSTED;
			}

			decoder->dictionary.buffer = newbuffer;
		}
	}

	decoder->lzma.len = 0;

	decoder->lzma2.sequence = LZMA2_CONTROL;
	decoder->lzma2.needed.dictionaryreset = 1;

	decoder->temporary.size = 0;

	return LZMA2_DECODER_STATUS_OK;
}

enum lzma2_decoder_status
lzma2_decoder_decode(struct lzma2_decoder *decoder, struct lzma2_stream *stream) {
	uint32_t byte;

	while (stream->input.position < stream->input.size || decoder->lzma2.sequence == LZMA2_LZMA_RUN) {
		switch (decoder->lzma2.sequence) {
		case LZMA2_CONTROL:
			byte = stream->input.buffer[stream->input.position++];

			if (byte >= 0xE0 || byte == 0x01) {
				decoder->lzma2.needed.properties = 1;
				decoder->lzma2.needed.dictionaryreset = 0;
				dictionary_reset(&decoder->dictionary, stream);
			} else if (decoder->lzma2.needed.dictionaryreset == 1) {
				return LZMA2_DECODER_STATUS_ERROR_CORRUPTED_DATA;
			}

			if (byte >= 0x80) {
				decoder->lzma2.uncompressed = (byte & 0x1F) << 16;
				decoder->lzma2.sequence = LZMA2_UNCOMPRESSED_1;

				if (byte >= 0xC0) {
					decoder->lzma2.needed.properties = 0;
					decoder->lzma2.next = LZMA2_PROPERTIES;
				} else if (decoder->lzma2.needed.properties == 1) {
					return LZMA2_DECODER_STATUS_ERROR_CORRUPTED_DATA;
				} else {
					decoder->lzma2.next = LZMA2_LZMA_PREPARE;
					if (byte >= 0xA0) {
						lzma_reset(decoder);
					}
				}
			} else {
				if (byte == 0x00) {
					return LZMA2_DECODER_STATUS_END;
				}

				if (byte > 0x02) {
					return LZMA2_DECODER_STATUS_ERROR_CORRUPTED_DATA;
				}

				decoder->lzma2.sequence = LZMA2_COMPRESSED_0;
				decoder->lzma2.next = LZMA2_COPY;
			}
			break;
		case LZMA2_UNCOMPRESSED_1:
			decoder->lzma2.uncompressed += (uint32_t)stream->input.buffer[stream->input.position++] << 8;
			decoder->lzma2.sequence = LZMA2_UNCOMPRESSED_2;
			break;
		case LZMA2_UNCOMPRESSED_2:
			decoder->lzma2.uncompressed += (uint32_t)stream->input.buffer[stream->input.position++] + 1;
			decoder->lzma2.sequence = LZMA2_COMPRESSED_0;
			break;
		case LZMA2_COMPRESSED_0:
			decoder->lzma2.compressed = (uint32_t)stream->input.buffer[stream->input.position++] << 8;
			decoder->lzma2.sequence = LZMA2_COMPRESSED_1;
			break;
		case LZMA2_COMPRESSED_1:
			decoder->lzma2.compressed += (uint32_t)stream->input.buffer[stream->input.position++] + 1;
			decoder->lzma2.sequence = decoder->lzma2.next;
			break;
		case LZMA2_PROPERTIES:
			if (!lzma_props(decoder, stream->input.buffer[stream->input.position++])) {
				return LZMA2_DECODER_STATUS_ERROR_CORRUPTED_DATA;
			}

			decoder->lzma2.sequence = LZMA2_LZMA_PREPARE;
			/* fallthrough */
		case LZMA2_LZMA_PREPARE:
			if (decoder->lzma2.compressed < RANGE_DECODER_INIT_BYTES) {
				return LZMA2_DECODER_STATUS_ERROR_CORRUPTED_DATA;
			}

			if (!range_decoder_read_init(&decoder->rangedecoder, stream)) {
				return LZMA2_DECODER_STATUS_OK;
			}

			decoder->lzma2.compressed -= RANGE_DECODER_INIT_BYTES;
			decoder->lzma2.sequence = LZMA2_LZMA_RUN;
			/* fallthrough */
		case LZMA2_LZMA_RUN:
			dictionary_limit(&decoder->dictionary, MIN(stream->output.size - stream->output.position, decoder->lzma2.uncompressed));
			if (!lzma2_decoder_lzma(decoder, stream)) {
				return LZMA2_DECODER_STATUS_ERROR_CORRUPTED_DATA;
			}

			decoder->lzma2.uncompressed -= dictionary_flush(&decoder->dictionary, stream);

			if (decoder->lzma2.uncompressed == 0) {
				if (decoder->lzma2.compressed > 0 || decoder->lzma.len > 0 || !range_decoder_is_finished(&decoder->rangedecoder)) {
					return LZMA2_DECODER_STATUS_ERROR_CORRUPTED_DATA;
				}

				range_decoder_reset(&decoder->rangedecoder);
				decoder->lzma2.sequence = LZMA2_CONTROL;
			} else if (stream->output.position == stream->output.size || (stream->input.position == stream->input.size && decoder->temporary.size < decoder->lzma2.compressed)) {
				return LZMA2_DECODER_STATUS_OK;
			}
			break;
		case LZMA2_COPY:
			dictionary_uncompressed(&decoder->dictionary, stream, &decoder->lzma2.compressed);
			if (decoder->lzma2.compressed > 0) {
				return LZMA2_DECODER_STATUS_OK;
			}

			decoder->lzma2.sequence = LZMA2_CONTROL;
			break;
		}
	}

	return LZMA2_DECODER_STATUS_OK;
}
