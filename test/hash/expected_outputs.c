#include "test.h"
#include <clod/hash.h>
#include <stdlib.h>

int main() {
	check("clod_crc64 matches expected output", clod_crc64((uint8_t*)"abcdefg", 7) == 0xe94be4086bedae1d);
	check("clod_crc32 matches expected output", clod_crc32((uint8_t*)"abcdefg", 7) == 0x312a6aa6);
	check("clod_crc24 matches expected output", clod_crc24((uint8_t*)"abcdefg", 7) == 0x7d29d5);
	check("clod_crc16 matches expected output", clod_crc16((uint8_t*)"abcdefg", 7) == 0x757c);
	check("clod_crc8 matches expected output", clod_crc8((uint8_t*)"abcdefg", 7) == 0x24);

	return EXIT_SUCCESS;
}
