#include <clod/stream.h>

int clod_stream_format(clod_stream *dst, struct clod_string fmt, ...) {
	va_list args;
	va_start(args, fmt);
	const int res = clod_stream_vformat(dst, fmt, args);
	va_end(args);
	return res;
}

int clod_stream_vformat(clod_stream *dst, struct clod_string fmt, va_list args) {
	struct clod_string buff = CLOD_STRING_NEW(256);
	clod_string_vformat(&buff, fmt, args);
	return dst->write(dst, &buff);
}
