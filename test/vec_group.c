#include <clod/vmath.h>
#include "test.h"

int64_t vec[64];



int main() {
	vec[0] = 31;
	vec[1] = 31;
	check("vec_group has correct output", vec_group(vec, 2, 10) == 1023);

	vec[0] = 31;
	vec[1] = 0;
	check("vec_group has correct output", vec_group(vec, 2, 10) == 31);
	
	vec[0] = ~0;
	vec[1] = ~0;
	vec[2] = ~0;
	check("vec_group has correct output", vec_group(vec, 3, 10) == 1023);

	uint64_t res = 0;
	for (int i = 0; i < 64; i++) {
		vec[i] = i & 1;
		res |= i & 1;
		res <<= 1;
	}
	check("vec_group has correct output", vec_group(vec, 64, 64) == res);

	vec[0] = -1;
	vec[1] = -1;
	check("vec_group has correct output", vec_group(vec, 2, 10) == 1023);

	vec[0] = -32;
	vec[1] = -32;
	check("vec_group has correct output", vec_group(vec, 2, 10) == 0);
}
