#include <clod/audio.h>

#define NUM_SAMPLES 2048

int main() {
	clod_stream in, out;

	int res = clod_stream_audio(&in, CLOD_AUDIO_IN);
	if (res != 0) {
		return res;
	}

	res = clod_stream_audio(&out, CLOD_AUDIO_OUT);
	if (res != 0) {
		return res;
	}

	float buffer[NUM_SAMPLES];
	clod_stream_copy(&out, &in, buffer, NUM_SAMPLES * sizeof(float), nullptr);

	in.close(&in);
	out.close(&out);
	return 0;
}
