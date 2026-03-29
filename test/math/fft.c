#include "debug.h"
#include <clod/hash.h>
#include <clod/math/fft.h>

#define BUFF_SIZE 2048

union conv {
	float f;
	uint32_t i;
};

int math_fft() {
	float original[BUFF_SIZE];
	float result[BUFF_SIZE];

	union conv rng;
	rng.i = 0;
	for (size_t i = 0; i < BUFF_SIZE; i += 2) {
		rng.i = clod_crc32_add(rng.i, &i, sizeof(i));
		original[i] = rng.f;
		original[i + 1] = 0.0f;
		result[i] = original[i];
		result[i + 1] = 0.0f;
	}

	clod_fft(result, BUFF_SIZE / 2, false);
	clod_fft(result, BUFF_SIZE / 2, true);
	for (size_t i = 0; i < BUFF_SIZE; i++) {
		float diff = result[i] - original[i];
		if (diff < 0.0f) diff *= -1.0f;

		float original_abs = original[i] < 0.0f ? -original[i] : original[i];
		assert_fatal(CLOD_TEST, diff < (original_abs / 1000) + 0.0001f,
			"Round trip fourier transform and inverse fourier transform resulted in values that differ too much.");
	}

	return 0;
}
