#include <stdlib.h>
#include <clod/audio.h>
#include <clod/stream.h>
#include <clod/sys/sys.h>
#include <clod/math/fft.h>

void *stdlib_allocate(void*, size_t size) {
	return malloc(size);
}

void stdlib_free(void *, void *ptr) {
	free(ptr);
}

#define NUM_SAMPLES 2048

int main() {
	clod_stream *audio;
	int res = clod_audio(&audio, CLOD_AUDIO_IN | CLOD_AUDIO_OUT, &(clod_allocator){
		.allocate = stdlib_allocate,
		.free = stdlib_free
	});

	if (res != 0) {
		return res;
	}

	float buffer[NUM_SAMPLES * 2];

	while (1) {
		struct clod_string buff;
		buff.ptr = (char*)buffer;
		buff.len = 0;
		buff.cap = NUM_SAMPLES * sizeof(float);

		const int read_res = audio->read(audio, &buff);
		if (read_res || buff.len != NUM_SAMPLES * sizeof(float)) {
			clod_exit(read_res);
		}

		for (int i = NUM_SAMPLES - 1; i >= 0; i--) {
			buffer[i * 2 + 1] = 0.0f;
			buffer[i * 2] = buffer[i];
		}

		clod_fft(buffer, NUM_SAMPLES, false);

		// vocoder

		clod_fft(buffer, NUM_SAMPLES, true);

		for (int i = 0; i < NUM_SAMPLES; i++) {
			buffer[i] = buffer[i * 2];
		}

		const int write_res = audio->write(audio, &buff);
		if (write_res) {
			clod_exit(write_res);
		}
	}
}
