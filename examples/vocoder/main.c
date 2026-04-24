#include <clod/math/fft.h>
#include <clod/stream/file.h>
#include <clod/stream/audio.h>

#define NUM_SAMPLES 256

void apply_effect(float *fft) {

}

void slide(float *data) {
	for (size_t i = 0; i < NUM_SAMPLES; i++) {
		data[i + NUM_SAMPLES] = data[i];
	}
}

int main(int argc, char **argv) {
	clod_file in;
	clod_audio out;

	char *file_path = "../../../examples/vocoder/out.pcm";
	if (argv[1]) file_path = argv[1];

	int res = clod_stream_file(&in, nullptr, file_path, CLOD_FILE_READ);
	if (res != 0) {
		return res;
	}

	res = clod_stream_audio(&out, CLOD_AUDIO_OUT);
	if (res != 0) {
		return res;
	}

	float input[NUM_SAMPLES * 2];
	float output[NUM_SAMPLES * 2];
	for (size_t i = 0; i < NUM_SAMPLES * 2; i++) {
		input[i] = 0.0f;
		output[i] = 0.0f;
	}

	while (1) {
		slide(input);
		slide(output);

		int res = in.stream.read(&in.stream, &(struct clod_string){ .ptr = (char*)input, .len = 0, .cap = NUM_SAMPLES * sizeof(float) });
		if (res == CLOD_ERR_EOF) break;
		if (res != CLOD_ERR_OK) return res;

		float fft[NUM_SAMPLES * 4];
		for (size_t i = 0; i < NUM_SAMPLES * 2; i++) {
			fft[i] = input[i];
		}

		clod_fft(fft, NUM_SAMPLES * 2, CLOD_FFT_INPUT * CLOD_FFT_TIME_MAG_PACKED + CLOD_FFT_OUTPUT * CLOD_FFT_FREQ_MAG);

		apply_effect(fft);

		clod_fft(fft, NUM_SAMPLES * 2, CLOD_FFT_INPUT * CLOD_FFT_FREQ_MAG + CLOD_FFT_OUTPUT * CLOD_FFT_TIME_MAG_PACKED);

		for (size_t i = 0; i < NUM_SAMPLES; i++) {
			output[i] = fft[i] * (float)i / (float) NUM_SAMPLES;
		}
		for (size_t i = NUM_SAMPLES; i < NUM_SAMPLES * 2; i++) {
			output[i] += fft[i] * (float)(NUM_SAMPLES * 2 - i) / (float) NUM_SAMPLES;
		}

		res = out.stream.write(&out.stream, &(struct clod_string){ .ptr = (char*)(output + NUM_SAMPLES), .len = NUM_SAMPLES * sizeof(float), .cap = 0 });
		if (res != CLOD_ERR_OK) {
			return res;
		}
	}

	in.stream.close(&in.stream);
	out.stream.close(&out.stream);

	return 0;
}
