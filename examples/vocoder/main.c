#include <clod/audio.h>
#include <clod/math/fft.h>

#define NUM_SAMPLES 128

void apply_effect(float *fft) {
	for (size_t i = 0; i < NUM_SAMPLES; i++) {
		float real = fft[i * 2];
		float imag = fft[i * 2 + 1];
		
		float freq = (float)i / (float)NUM_SAMPLES;
		
		if (freq > 0.2f && freq < 0.8f) {
			real *= 3.0f;
			imag *= 3.0f;
		} else if (freq < 0.1f) {
			real *= 0.1f;
			imag *= 0.1f;
		}
		
		fft[i * 2] = real;
		fft[i * 2 + 1] = imag;
	}
}

void slide(float *data) {
	for (size_t i = 0; i < NUM_SAMPLES; i++) {
		data[i + NUM_SAMPLES] = data[i];
	}
}

int main() {
	clod_stream in, out;

	int res = clod_audio(&in, CLOD_AUDIO_IN);
	if (res != 0) {
		return res;
	}

	res = clod_audio(&out, CLOD_AUDIO_OUT);
	if (res != 0) {
		return res;
	}

	float input[NUM_SAMPLES * 2];
	float output[NUM_SAMPLES * 2];
	for (size_t i = 0; i < NUM_SAMPLES * 2; i++) output[i] = 0.0f;

	while (1) {
		slide(input);
		slide(output);

		in.read(&in, &(struct clod_string){ .ptr = (char*)input, .len = 0, .cap = NUM_SAMPLES * sizeof(float) });

		float fft[NUM_SAMPLES * 4];
		for (size_t i = 0; i < NUM_SAMPLES * 2; i++) {
			fft[i * 2] = input[i];
			fft[i * 2 + 1] = 0.0f;
		}

		clod_fft(fft, NUM_SAMPLES * 2, false);

		apply_effect(fft);

		clod_fft(fft, NUM_SAMPLES * 2, true);

		for (size_t i = 0; i < NUM_SAMPLES; i++) {
			output[i] = fft[i * 2] * (float)i / (float) NUM_SAMPLES;
		}
		for (size_t i = NUM_SAMPLES; i < NUM_SAMPLES * 2; i++) {
			output[i] += fft[i * 2] * (float)(NUM_SAMPLES * 2 - i) / (float) NUM_SAMPLES;
		}

		out.write(&out, &(struct clod_string){ .ptr = (char*)(output + NUM_SAMPLES), .len = NUM_SAMPLES * sizeof(float), .cap = 0 });
	}
}
