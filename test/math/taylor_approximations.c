#include <stdio.h>
#include <math.h>

#define PI 3.141592653589793238462643383279502884197169399375105820974944592307f

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
static float fft_atan(const float x) {
	if (x > 1.0f) return PI / 2.0f - fft_atan(1.0f / x);
	if (x < -1.0f) return -PI / 2.0f - fft_atan(1.0f / x);

	float res = x;
	float s = x;
	for (int i = 1; i <= 10; i++) {
		res += s = -s * x * x * (float)(2 * i - 1) / (float)(2 * i + 1);
	}
	return res;
}
static float fft_sqrt(const float x) {
	if (x <= 0.0f) return 0.0f;
	float guess = x / 2.0f;
	for (int i = 0; i < 10; i++) {
		guess = (guess + x / guess) / 2.0f;
	}
	return guess;
}

int math_taylor_approximations() {
	float test_float = -4.0f;
	bool fail = false;

	for (int i = 0; i < 1000; i++) {
		test_float += 0.001f;

		float clod_sin = fft_sin(test_float);
		float clod_cos = fft_cos(test_float);
		float clod_atan = fft_atan(test_float);
		float clod_sqrt = fft_sqrt(test_float);

		float std_sin = sinf(test_float);
		float std_cos = cosf(test_float);
		float std_atan = atanf(test_float);
		float std_sqrt = sqrtf(test_float);

		float sin_diff = clod_sin - std_sin;
		float cos_diff = clod_cos - std_cos;
		float atan_diff = clod_atan - std_atan;
		float sqrt_diff = clod_sqrt - std_sqrt;

		if (sin_diff < 0.0f) sin_diff *= -1.0f;
		if (cos_diff < 0.0f) cos_diff *= -1.0f;
		if (atan_diff < 0.0f) atan_diff *= -1.0f;
		if (sqrt_diff < 0.0f) sqrt_diff *= -1.0f;

		/// TODO: replace with libclod methods when double formatting is added.
		if (sin_diff > 0.0001f) printf("\t[%d] Sin(%f): Got %f, wanted %f.\n", i, (double)test_float, (double)clod_sin, (double)std_sin), fail = true;
		if (cos_diff > 0.0001f) printf("\t[%d] Cos(%f): Got %f, wanted %f.\n", i, (double)test_float, (double)clod_cos, (double)std_cos), fail = true;
		if (atan_diff > 0.0001f) printf("\t[%d] Atan(%f): Got %f, wanted %f.\n", i, (double)test_float, (double)clod_atan, (double)std_atan), fail = true;
		if (sqrt_diff > 0.0001f) printf("\t[%d] Sqrt(%f): Got %f, wanted %f.\n", i, (double)test_float, (double)clod_sqrt, (double)std_sqrt), fail = true;
	}

	return fail;
}
