#include <clod/stream/audio.h>

#define NUM_SAMPLES 2048

int main() {
	clod_audio input;
	clod_audio output;

	int res = clod_stream_audio(&input, CLOD_AUDIO_IN);
	if (res != 0) {
		return res;
	}

	res = clod_stream_audio(&output, CLOD_AUDIO_OUT);
	if (res != 0) {
		return res;
	}

	float buffer[NUM_SAMPLES];
	clod_stream_copy(&output.stream, &input.stream, buffer, NUM_SAMPLES * sizeof(float), nullptr);

	input.stream.close(&input.stream);
	output.stream.close(&output.stream);
	return 0;
}
