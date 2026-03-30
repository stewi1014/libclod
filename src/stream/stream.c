#include "config.h"
#include "debug.h"
#include <clod/stream.h>

int clod_stream_copy(clod_stream *dst, clod_stream *src, void *const buffer, const size_t buffer_size, size_t *total_transferred) {
	if (buffer_size == 0) {
		debug(CLOD_DEBUG, "Invalid buffer size for stream copy.");
		return CLOD_STREAM_INVALID;
	}

	if (dst->write == nullptr) {
		debug(CLOD_DEBUG, "Provided stream %ptr doesn't support writing.", (void*)dst);
		return CLOD_STREAM_INVALID;
	}

	if (src->read == nullptr) {
		debug(CLOD_DEBUG, "Provided stream %ptr doesn't support reading.", (void*)src);
		 return CLOD_STREAM_INVALID;
	}

	size_t transferred = 0;
	while (1) {
		struct clod_string buff = (struct clod_string){ .ptr = buffer, .len = 0, .cap = (ptrdiff_t)buffer_size};
		int read_res = src->read(src, &buff);

		int write_res = 0;
		if (buff.len > 0) {
			write_res = dst->write(dst, &buff);
			transferred += (size_t)buff.ptr - (size_t)buffer;
		}
		
		if (buff.len > 0) {
			debug(CLOD_DEBUG, "Short write to stream %ptr.", (void*)dst);
		}

		if (read_res == CLOD_STREAM_EOF) {
			if (total_transferred) *total_transferred = transferred;
			return 0;
		}

		if (read_res != CLOD_STREAM_OK) {
			if (total_transferred) *total_transferred = transferred;
			return read_res;
		}
		if (write_res != CLOD_STREAM_OK) {
			if (total_transferred) *total_transferred = transferred;
			return write_res;
		}
	}
}
