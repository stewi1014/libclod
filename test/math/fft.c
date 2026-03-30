#include <math.h>
#include <stdio.h>

#include "debug.h"
#include <clod/hash.h>
#include <clod/math/fft.h>

static float fft_sin(const float n) {
	float res = n;
	float s = n;
	for (int i = 1; i <= 17; i += 2) {
		res += s = -s * n * n / (float)((i + 1) * (i + 2));
	}
	return res;
}
static float fft_cos(const float n) {
	float res = 1.0f;
	float s = 1.0f;
	for (int i = 0; i <= 16; i += 2) {
		res += s = -s * n * n / (float)((i + 1) * (i + 2));
	}
	return res;
}

#define BUFF_SIZE 2048

int math_fft() {
	float sin_test = -4.0f;
	for (int i = 0; i < 1000; i++) {
		sin_test += 0.001f;

		float clod_sin = fft_sin(sin_test);
		float clod_cos = fft_cos(sin_test);

		float std_sin = sinf(sin_test);
		float std_cos = cosf(sin_test);

		float sin_diff = clod_sin - std_sin;
		float cos_diff = clod_cos - std_cos;
		if (sin_diff < 0.0f) sin_diff *= -1.0f;
		if (cos_diff < 0.0f) cos_diff *= -1.0f;

		if (sin_diff > 0.0001f) printf("\t[%d] Sin(%f): Got %f, wanted %f.\n", i, (double)sin_test, (double)clod_sin, (double)std_sin);
		if (cos_diff > 0.0001f) printf("\t[%d] Cos(%f): Got %f, wanted %f.\n", i, (double)sin_test, (double)clod_cos, (double)std_cos);
	}

	float original[BUFF_SIZE];
	float result[BUFF_SIZE];

	uint32_t rng = 1;
	for (size_t i = 0; i < BUFF_SIZE; i += 2) {
		rng = clod_crc32_add(rng, &i, sizeof(i));
		original[i] = (float)rng;
		original[i + 1] = 0.0f;
		result[i] = original[i];
		result[i + 1] = 0.0f;
	}

	clod_fft(result, BUFF_SIZE / 2, false);
	clod_fft(result, BUFF_SIZE / 2, true);
	for (size_t i = 0; i < BUFF_SIZE; i += 2) {
		float diff = result[i] - original[i];
		if (diff < 0.0f) diff *= -1.0f;

		float original_abs = original[i] < 0.0f ? -original[i] : original[i];
		assert_fatal(CLOD_TEST, diff < (original_abs / 1000) + 0.0001f,
			"Round trip fourier transform and inverse fourier transform resulted in values that differ too much.");
	}

	return 0;
}
