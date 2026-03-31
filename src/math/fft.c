#include <signal.h>
#include <stdbool.h>

#include "config.h"
#include "debug.h"
#include <clod/math/fft.h>

#define PI 3.141592653589793238462643383279502884197169399375105820974944592307f

#define R 0
#define I 1

#define INPUT_FORMAT(opt) (((opt) / CLOD_FFT_INPUT) & 0xFF)
#define OUTPUT_FORMAT(opt) (((opt) / CLOD_FFT_OUTPUT) & 0xFF)

static float fft_sin(float n) {
	while (n > PI) n -= 2.0f * PI;
	while (n < -PI) n += 2.0f * PI;

	float res = n;
	float s = n;
	for (int i = 1; i <= 12; i += 2) {
		res += s = -s * n * n / (float)((i + 1) * (i + 2));
	}
	return res;
}
static float fft_cos(float n) {
	while (n > PI) n -= 2.0f * PI;
	while (n < -PI) n += 2.0f * PI;

	float res = 1.0f;
	float s = 1.0f;
	for (int i = 0; i <= 12; i += 2) {
		res += s = -s * n * n / (float)((i + 1) * (i + 2));
	}
	return res;
}
static float fft_atan(const float x) {
	if (x > 1.0f) return PI / 2.0f - fft_atan(1.0f / x);
	if (x < -1.0f) return -PI / 2.0f - fft_atan(1.0f / x);

	float res = x;
	float s = x;
	for (int i = 1; i <= 36; i++) {
		res += s = -s * x * x * (float)(2 * i - 1) / (float)(2 * i + 1);
	}
	return res;
}
static float fft_sqrt(const float x) {
	if (x <= 0.0f) return 0.0f;
	float guess = x / 2.0f;
	for (int i = 0; i < 8; i++) {
		guess = (guess + x / guess) / 2.0f;
	}
	return guess;
}
static float fft_atan2(const float y, const float x) {
	if (x > 0.0f) return fft_atan(y / x);
	if (x == 0.0f) {
		if (y > 0.0f) return PI / 2.0f;
		if (y < 0.0f) return -PI / 2.0f;
		return 0.0f;
	}
	if (y >= 0.0f) return fft_atan(y / x) + PI;
	return fft_atan(y / x) - PI;
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
static void complex_to_mag(float *restrict data, const size_t len) {
	for (size_t i = 0; i < len; i++) {
		const float real = data[i * 2 + R];
		const float imag = data[i * 2 + I];
		data[i * 2 + R] = fft_sqrt(real * real + imag * imag);
		data[i * 2 + I] = fft_atan2(imag, real);
	}
}
/*
static void complex_inverted_to_mag(float *restrict data, const size_t len) {
	for (size_t i = 0; i < len; i++) {
		float n[2] = { data[i * 2 + R], data[i * 2 + I] };
		n[I] *= -1.0f;
		mul(n, (float[2]){ 1.0f / (float)len, 0.0f });
		data[i * 2 + R] = fft_sqrt(n[R] * n[R] + n[I] * n[I]);
		data[i * 2 + I] = fft_atan2(n[I], n[R]);
	}
}
*/
static void mag_to_complex(float *restrict data, const size_t len) {
	for (size_t i = 0; i <= len / 2; i++) {
		const float mag = data[i * 2 + R];
		const float phase = data[i * 2 + I];
		const float real = mag * fft_cos(phase);
		const float imag = mag * fft_sin(phase);
		data[i * 2 + R] = real;
		data[i * 2 + I] = imag;

		if (i > 0 && i < len / 2) {
			const size_t conjugate = len - i;
			data[conjugate * 2 + R] = real;
			data[conjugate * 2 + I] = -imag;
		}
	}
}
/*
static void mag_to_complex_inverted(float *restrict data, const size_t len) {
	for (size_t i = 0; i < len; i++) {
		float n[2] = { data[i * 2 + R], data[i * 2 + I] };
		n[I] *= -1.0f;
		data[i * 2 + R] = n[R] * fft_cos(n[I]);
		data[i * 2 + I] = n[R] * fft_sin(n[I]);
	}
}
*/
static void pack(float *restrict data, const size_t len, const float padding) {
	size_t i = 0;
	while (i < len / 2) {
		data[i] = data[i * 2];
		i++;
	}
	while (i < len) {
		data[i] = data[i * 2];
		data[i * 2] = padding;
		data[i * 2 + 1] = padding;
		i++;
	}
}
static void invert_imag(float *restrict data, const size_t len) {
	for (size_t i = 0; i < len; i++) {
		data[i * 2 + 1] *= -1.0f;
	}
}
static void invert_scale(float *restrict data, const size_t len) {
	for (size_t i = 0; i < len; i++) {
		mul(&data[i * 2], (float[2]){ 1.0f / (float)len, 0.0f });
	}
}
static void invert_scale_pack(float *restrict data, const size_t len, const float padding) {
	size_t i = 0;
	while (i < len / 2) {
		float n[2] = { data[i * 2 + R], data[i * 2 + I] };
		n[I] *= -1.0f;
		mul(n, (float[2]){ 1.0f / (float)len, 0.0f });
		data[i] = n[R];
		i++;
	}
	while (i < len) {
		float n[2] = { data[i * 2 + R], data[i * 2 + I] };
		n[I] *= -1.0f;
		mul(n, (float[2]){ 1.0f / (float)len, 0.0f });
		data[i] = n[R];
		data[i * 2 + R] = padding;
		data[i * 2 + I] = padding;
		i++;
	}
}

static void fft(float *data, const size_t len) {
	for (size_t i = 1, rev = 0; i < len; i++) {
		size_t b = len >> 1;
		while (rev & b) {
			rev ^= b;
			b >>= 1;
		}
		rev ^= b;
		if (data[i * 2] != data[i * 2]) {
			data[i * 2] = 0.0f;
			debug(CLOD_DEBUG, "NaN detected in FFT input data.");
		}
		if (data[i * 2 + 1] != data[i * 2 + 1]) {
			data[i * 2 + 1] = 0.0f;
			debug(CLOD_DEBUG, "NaN detected in FFT input data.");
		}
		if (i < rev) {
			swap(data + i * 2, data + rev * 2);
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
}

void clod_fft(float *restrict data, const size_t len, const int opt) {
	if (len == 0) goto error;

	switch (INPUT_FORMAT(opt)) {

		case CLOD_FFT_TIME_MAG_PACKED:
			switch (OUTPUT_FORMAT(opt)) {

				case CLOD_FFT_TIME_COMPLEX:
					for (size_t i = len; i > 0; i--) {
						data[(i - 1) * 2] = data[i - 1];
						data[(i - 1) * 2 + 1] = 0.0f;
					}
					return;

				case CLOD_FFT_TIME_MAG_PACKED:
					for (size_t i = len; i < len * 2; i++)
						data[i] = 0.0f;
					return;

				case CLOD_FFT_FREQ_COMPLEX:
					for (size_t i = len; i > 0; i--) {
						data[(i - 1) * 2] = data[i - 1];
						data[(i - 1) * 2 + 1] = 0.0f;
					}
					fft(data, len);
					return;

				case CLOD_FFT_FREQ_MAG:
					for (size_t i = len; i > 0; i--) {
						data[(i - 1) * 2] = data[i - 1];
						data[(i - 1) * 2 + 1] = 0.0f;
					}
					fft(data, len);
					complex_to_mag(data, len);
					return;

				default: goto error;
			}

		case CLOD_FFT_TIME_COMPLEX:
			switch (OUTPUT_FORMAT(opt)) {

				case CLOD_FFT_TIME_COMPLEX:
					return;

				case CLOD_FFT_TIME_MAG_PACKED:
					pack(data, len, 0.0f);
					return;

				case CLOD_FFT_FREQ_COMPLEX:
					fft(data, len);
					return;

				case CLOD_FFT_FREQ_MAG:
					fft(data, len);
					complex_to_mag(data, len);
					return;

				default: goto error;
			}

		case CLOD_FFT_FREQ_COMPLEX:
			switch (OUTPUT_FORMAT(opt)) {

				case CLOD_FFT_TIME_COMPLEX:
					invert_imag(data, len);
					fft(data, len);
					invert_scale(data, len);
					return;

				case CLOD_FFT_TIME_MAG_PACKED:
					invert_imag(data, len);
					fft(data, len);
					invert_scale_pack(data, len, 0.0f);
					return;

				case CLOD_FFT_FREQ_COMPLEX:
					return;

				case CLOD_FFT_FREQ_MAG:
					complex_to_mag(data, len);
					return;

				default: goto error;
			}

		case CLOD_FFT_FREQ_MAG:
			switch (OUTPUT_FORMAT(opt)) {

				case CLOD_FFT_TIME_COMPLEX:
					mag_to_complex(data, len);
					invert_imag(data, len);
					fft(data, len);
					invert_scale(data, len);
					return;

				case CLOD_FFT_TIME_MAG_PACKED:
					mag_to_complex(data, len);
					invert_imag(data, len);
					fft(data, len);
					invert_scale_pack(data, len, 0.0f);
					return;

				case CLOD_FFT_FREQ_COMPLEX:
					mag_to_complex(data, len);
					return;

				case CLOD_FFT_FREQ_MAG:
					return;

				default: goto error;
			}

		default: goto error;
	}

error:
	debug(CLOD_DEBUG, "Ignoring invalid FFT arguments (%ptr, %size, %bi).", (void*)data, len, opt);
}
