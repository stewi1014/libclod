#include "clod/debug.h"
#include <clod/hash.h>

int hash_expected(int, char[]) {
	assert_fatal(CLOD_TEST, clod_crc64((uint8_t*)"abcdefg", 7) == 0xe94be4086bedae1d,
		"Want 0x%Xu64. Got 0x%Xu64.",
		UINT64_C(0xe94be4086bedae1d), clod_crc64((uint8_t*)"abcdefg", 7));

	assert_fatal(CLOD_TEST, clod_crc32((uint8_t*)"abcdefg", 7) == 0xE627F441,
		"Want 0x%xu32. Got 0x%xu32.",
		UINT32_C(0xE627F441), clod_crc32((uint8_t*)"abcdefg", 7));

	assert_fatal(CLOD_TEST, clod_crc24((uint8_t*)"abcdefg", 7) == 0x7d29d5,
		"Want 0x%Xu32. Got 0x%Xu32.",
		UINT32_C(0x7d29d5), clod_crc24((uint8_t*)"abcdefg", 7));

	assert_fatal(CLOD_TEST, clod_crc16((uint8_t*)"abcdefg", 7) == 0x757c,
		"Want 0x%Xu. Got 0x%Xu.",
		0x757c, clod_crc16((uint8_t*)"abcdefg", 7));

	assert_fatal(CLOD_TEST, clod_crc8((uint8_t*)"abcdefg", 7) == 0x24,
		"Want 0x%Xu. Got 0x%Xu.",
		0x24, clod_crc8((uint8_t*)"abcdefg", 7));

	return 0;
}
