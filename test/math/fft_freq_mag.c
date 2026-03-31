#include <math.h>
#include <stdio.h>

#include "debug.h"
#include <clod/hash.h>
#include <clod/math/fft.h>

#define BUFF_SIZE 128

int math_fft_freq_mag() {
	float original[BUFF_SIZE * 2];
	float result[BUFF_SIZE * 2];

	uint32_t rng = 1;
	for (size_t i = 0; i < BUFF_SIZE * 2; i += 2) {
		rng = clod_crc32_add(rng, &i, sizeof(i));
		original[i] = ((float)(rng | 1)) / (float)UINT32_MAX;
		original[i + 1] = 0.0f;
		result[i] = original[i];
		result[i + 1] = 0.0f;

		assert_fatal(CLOD_TEST, original[i] == original[i], "Test case must not generate NaN values.");
		assert_fatal(CLOD_TEST, original[i + 1] == original[i + 1], "Test case must not generate NaN values.");
	}

	bool fail = false;

	clod_fft(result, BUFF_SIZE, CLOD_FFT_INPUT * CLOD_FFT_TIME_COMPLEX + CLOD_FFT_OUTPUT * CLOD_FFT_FREQ_MAG);
	clod_fft(result, BUFF_SIZE, CLOD_FFT_INPUT * CLOD_FFT_FREQ_MAG + CLOD_FFT_OUTPUT * CLOD_FFT_TIME_COMPLEX);
	for (size_t i = 0; i < BUFF_SIZE * 2; i += 2) {
		float diff = result[i] - original[i];
		if (diff < 0.0f) diff *= -1.0f;

		/// TODO: replace with libclod methods when double formatting is added.
		if (diff != diff || diff > 0.0001f) {
			fprintf(stderr, "\t[%d] Got %f, wanted %f. Difference %f larger than allowed %f\n",
				(int)i, (double)result[i], (double)original[i], (double)diff, 0.0001);
			fail = true;
		}
	}

	return fail;
}
