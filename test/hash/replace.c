#include "test.h"
#include <clod/hash.h>
#include <stdlib.h>
#include <string.h>

int main() {
	uint64_t crc64 = clod_crc64("abcdefg", 7);
	uint64_t state64 = clod_crc64_finalise(crc64);
	state64 = clod_crc64_add_at(state64, "cd", 2, 3);
	state64 = clod_crc64_add_at(state64, "hi", 2, 3);
	check("Replacing section of crc64 is correct", clod_crc64("abhiefg", 7) == clod_crc64_finalise(state64));

	uint32_t crc32 = clod_crc32("abcdefg", 7);
	uint32_t state32 = clod_crc32_finalise(crc32);
	state32 = clod_crc32_add_at(state32, "cd", 2, 3);
	state32 = clod_crc32_add_at(state32, "hi", 2, 3);
	check("Replacing section of crc32 is correct", clod_crc32("abhiefg", 7) == clod_crc32_finalise(state32));

	uint32_t crc24 = clod_crc24("abcdefg", 7);
	uint32_t state24 = clod_crc24_finalise(crc24);
	state24 = clod_crc24_add_at(state24, "cd", 2, 3);
	state24 = clod_crc24_add_at(state24, "hi", 2, 3);
	check("Replacing section of crc24 is correct", clod_crc24("abhiefg", 7) == clod_crc24_finalise(state24));

	uint16_t crc16 = clod_crc16("abcdefg", 7);
	uint16_t state16 = clod_crc16_finalise(crc16);
	state16 = clod_crc16_add_at(state16, "cd", 2, 3);
	state16 = clod_crc16_add_at(state16, "hi", 2, 3);
	check("Replacing section of crc16 is correct", clod_crc16("abhiefg", 7) == clod_crc16_finalise(state16));

	uint8_t crc8 = clod_crc8("abcdefg", 7);
	uint8_t state8 = clod_crc8_finalise(crc8);
	state8 = clod_crc8_add_at(state8, "cd", 2, 3);
	state8 = clod_crc8_add_at(state8, "hi", 2, 3);
	check("Replacing section of crc8 is correct", clod_crc8("abhiefg", 7) == clod_crc8_finalise(state8));
	return EXIT_SUCCESS;
}
