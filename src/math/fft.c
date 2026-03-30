#include <signal.h>
#include <stdbool.h>

#include "config.h"
#include "debug.h"
#include <clod/math/fft.h>

#define PI 3.141592653589793238462643383279502884197169399375105820974944592307f

#define R 0
#define I 1


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
static void swap(float *restrict a, float *restrict b) {
	float tmp[2];
	tmp[R] = a[R];
	tmp[I] = a[I];
	a[R] = b[R];
	a[I] = b[I];
	b[R] = tmp[R];
	b[I] = tmp[I];
}
static void mul(float a[2], const float b[2]) {
	const float tmp = a[R] * b[R] - a[I] * b[I];
	a[I] = a[R] * b[I] + a[I] * b[R];
	a[R] = tmp;
}

void clod_fft(float *restrict data, size_t len, bool invert) {
	assert_fatal(CLOD_DEBUG, len > 0 && (len & (len - 1)) == 0,
		"Length for fast fourier transform array must be power of two. Given %size.", len);

	for (size_t i = 1, rev = 0; i < len; i++) {
		size_t b = len >> 1;
		while (rev & b) {
			rev ^= b;
			b >>= 1;
		}
		rev ^= b;
		if (i < rev) {
			swap(data + i * 2, data + rev * 2);
		}
	}

	if (invert) {
		for (size_t i = 0; i < len; i++) {
			data[i * 2 + 1] *= -1.0f;
		}
	}

	for (size_t s = 2; s <= len; s <<= 1) {
		const float wn[2] = {
			fft_cos(2.0f * PI / (float)s),
			fft_sin(2.0f * PI / (float)s)
		};

		for (size_t i = 0; i < len; i += s) {
			float w[2] = {1.0f, 0.0f};
			for (size_t j = 0; j < s / 2; j++) {
				const size_t index1 = 2 * (i + j);
				const size_t index2 = 2 * (i + j + s / 2);

				float u[2] = {data[index1], data[index1 + 1]};
				float v[2] = {data[index2], data[index2 + 1]};
				mul(v, w);

				data[index1] = u[R] + v[R];
				data[index1 + 1] = u[I] + v[I];

				data[index2] = u[R] - v[R];
				data[index2 + 1] = u[I] - v[I];
				mul(w, wn);
			}
		}
	}

	if (invert) {
		for (size_t i = 0; i < len; i++) {
			data[i * 2 + 1] *= -1.0f;
			mul(&data[i * 2], (float[2]){ 1.0f / (float)len, 0.0f });
		}
	}
}
