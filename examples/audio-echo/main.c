#include <stdlib.h>
#include <clod/audio.h>
#include <clod/stream.h>
#include <clod/sys/sys.h>

void *stdlib_allocate(void*, size_t size) {
	return malloc(size);
}

void stdlib_free(void *, void *ptr) {
	free(ptr);
}

int main() {
	clod_stream *audio;
	int res = clod_audio(&audio, CLOD_AUDIO_IN | CLOD_AUDIO_OUT, &(clod_allocator){
		.allocate = stdlib_allocate,
		.free = stdlib_free
	});

	if (res != 0) {
		return res;
	}

	void *buffer = malloc(8192);
	res = clod_stream_copy(audio, audio, buffer, 8192, nullptr);
	free(buffer);
	return res;
}
