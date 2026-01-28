#include "test.h"
#include <clod/hash.h>
#include <stdlib.h>
#include <string.h>


int main() {
	clod_sip64_state sip64 = clod_sip64_init(0);
	sip64 = clod_sip64_add(sip64, "abcd", 4);
	sip64 = clod_sip64_add(sip64, "efg", 3);
	sip64 = clod_sip64_add(sip64,
		"p29585phsw-fg9hspfipipwqeiorhgposdfg-w[04565432",
		strlen("p29585phsw-fg9hspfipipwqeiorhgposdfg-w[04565432"));
	sip64 = clod_sip64_add(sip64, "d", 1);
	sip64 = clod_sip64_add(sip64, "b", 1);
	sip64 = clod_sip64_add(sip64, "c", 1);
	sip64 = clod_sip64_add(sip64, "a", 1);
	sip64 = clod_sip64_add(sip64, "e", 1);
	sip64 = clod_sip64_add(sip64, "f", 1);
	sip64 = clod_sip64_add(sip64, "g", 1);
	sip64 = clod_sip64_add(sip64, "h", 1);
	check(clod_sip64_finalise(sip64) == clod_sip64(0,
		"abcdefgp29585phsw-fg9hspfipipwqeiorhgposdfg-w[04565432dbcaefgh",
		strlen("abcdefgp29585phsw-fg9hspfipipwqeiorhgposdfg-w[04565432dbcaefgh")));

	uint64_t crc64 = clod_crc64_init();
	crc64 = clod_crc64_add(crc64, "abcd", 4);
	crc64 = clod_crc64_add(crc64, "efg", 3);
	check(clod_crc64_finalise(crc64) == clod_crc64("abcdefg", 7));

	uint32_t crc32 = clod_crc32_init();
	crc32 = clod_crc32_add(crc32, "abcd", 4);
	crc32 = clod_crc32_add(crc32, "efg", 3);
	check(clod_crc32_finalise(crc32) == clod_crc32("abcdefg", 7));

	uint32_t crc24 = clod_crc24_init();
	crc24 = clod_crc24_add(crc24, "abcd", 4);
	crc24 = clod_crc24_add(crc24, "efg", 3);
	check(clod_crc24_finalise(crc24) == clod_crc24("abcdefg", 7));

	uint16_t crc16 = clod_crc16_init();
	crc16 = clod_crc16_add(crc16, "abcd", 4);
	crc16 = clod_crc16_add(crc16, "efg", 3);
	check(clod_crc16_finalise(crc16) == clod_crc16("abcdefg", 7));

	uint8_t crc8 = clod_crc8_init();
	crc8 = clod_crc8_add(crc8, "abcd", 4);
	crc8 = clod_crc8_add(crc8, "efg", 3);
	check(clod_crc8_finalise(crc8) == clod_crc8("abcdefg", 7));

	return EXIT_SUCCESS;
}
