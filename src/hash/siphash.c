#include <clod/hash.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * I thank those who have researched and developed SipHash,
 * and the original designers Jean-Philippe Aumasson and Daniel J. Bernstein.
 * https://www.aumasson.jp/siphash/siphash.pdf
 *
 * This implementation is my own and is designed to support streaming.
 * Surprisingly, this streaming variant outperforms the SIMD-optimised SipHash 2-4 implementation found in SMHasher3.
 */

#define data_size(state) ((state)._size >> 3)
#define remaining(state) ((state)._size & 0b00000111)
#define set_data_size(state, size) ((state)._size = (uint8_t)(((state)._size & 0b00000111) | ((uint8_t)(size) << 3)))
#define set_remaining(state, size) ((state)._size = (uint8_t)(((state)._size & 0b11111000) | ((uint8_t)(size) & 0b00000111)))

CLOD_INLINE static inline void
sip_round(clod_sip64_state *state) {
	state->_v0 += state->_v1;
	state->_v1 = state->_v1 << 13 | state->_v1 >> 51;
	state->_v1 ^= state->_v0;
	state->_v0 = state->_v0 << 32 | state->_v0 >> 32;
	state->_v2 += state->_v3;
	state->_v3 = state->_v3 << 16 | state->_v3 >> 48;
	state->_v3 ^= state->_v2;
	state->_v0 += state->_v3;
	state->_v3 = state->_v3 << 21 | state->_v3 >> 43;
	state->_v3 ^= state->_v0;
	state->_v2 += state->_v1;
	state->_v1 = state->_v1 << 17 | state->_v1 >> 47;
	state->_v1 ^= state->_v2;
	state->_v2 = state->_v2 << 32 | state->_v2 >> 32;
}
CLOD_INLINE static inline uint64_t
read_uint64(const unsigned char *restrict data, const size_t data_size) {
	uint64_t r = 0;
	switch (data_size) {
		default: r |= (uint64_t)data[7] << 7 * 8; __attribute__((fallthrough));
		case 7:  r |= (uint64_t)data[6] << 6 * 8; __attribute__((fallthrough));
		case 6:  r |= (uint64_t)data[5] << 5 * 8; __attribute__((fallthrough));
		case 5:  r |= (uint64_t)data[4] << 4 * 8; __attribute__((fallthrough));
		case 4:  r |= (uint64_t)data[3] << 3 * 8; __attribute__((fallthrough));
		case 3:  r |= (uint64_t)data[2] << 2 * 8; __attribute__((fallthrough));
		case 2:  r |= (uint64_t)data[1] << 1 * 8; __attribute__((fallthrough));
		case 1:  r |= (uint64_t)data[0] << 0 * 8; __attribute__((fallthrough));
		case 0: return r;
	}
}
clod_sip64_state clod_sip64_add(clod_sip64_state state, const void *data, const size_t size) {
	if (size == 0) return state;
	const unsigned char* restrict in = data;
	set_data_size(state, data_size(state) + size);

	if (remaining(state) + size < 8) {
		memcpy(state._buf + remaining(state), in, size);
		set_remaining(state, remaining(state) + size);
		return state;
	}

	uint64_t d, off = 0;
	if (remaining(state) > 0) {
		d = read_uint64(state._buf, remaining(state));
		d |= read_uint64(in, 8 - remaining(state)) << remaining(state) * 8;
		off = 8 - remaining(state);
		set_remaining(state, 0);
	} else {
		d = read_uint64(in, 8);
		off = 8;
	}

	state._v3 ^= d;
	sip_round(&state);
	sip_round(&state);
	state._v0 ^= d;

	while (off + 8 <= size) {
		d = read_uint64(in + off, 8);
		off += 8;

		state._v3 ^= d;
		sip_round(&state);
		sip_round(&state);
		state._v0 ^= d;
	}

	if (off < size) {
		memcpy(state._buf, in + off, size - off);
		set_remaining(state, size - off);
	}

	return state;
}
uint64_t clod_sip64_finalise(clod_sip64_state state) {
	uint64_t d = read_uint64(state._buf, remaining(state));
	d |= (uint64_t)(data_size(state)) << 56;
	state._v3 ^= d;
	sip_round(&state);
	sip_round(&state);
	state._v0 ^= d;
	state._v2 ^= 0xee;
	sip_round(&state);
	sip_round(&state);
	sip_round(&state);
	sip_round(&state);
	return state._v0 ^ state._v1 ^ state._v2 ^ state._v3;
}
