#include <clod/stream.h>
#include "syscall.h"
#include "clod/file.h"
#include "clod/memory.h"

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0100
#define O_TRUNC 01000
#define O_APPEND 02000
#define O_DIRECTORY 0200000

int clod_file(
	clod_stream *stream_out,
	const clod_stream *directory,
	const char *path,
	const int flags
) {
	int o_flags = 0;
	switch (flags & (CLOD_FILE_READ | CLOD_FILE_WRITE | CLOD_FILE_TRUNCATE | CLOD_FILE_DIRECTORY)) {
		case CLOD_FILE_READ:
			o_flags |= O_RDONLY; break;
		case CLOD_FILE_READ | CLOD_FILE_DIRECTORY:
			o_flags |= O_RDONLY | O_DIRECTORY; break;
		case CLOD_FILE_READ | CLOD_FILE_WRITE:
			o_flags |= O_RDWR; break;
		case CLOD_FILE_READ | CLOD_FILE_WRITE | CLOD_FILE_TRUNCATE:
			o_flags |= O_RDWR | O_TRUNC; break;
		case CLOD_FILE_WRITE:
			o_flags |= O_WRONLY; break;
		case CLOD_FILE_WRITE | CLOD_FILE_TRUNCATE:
			o_flags |= O_WRONLY | O_TRUNC; break;
		case CLOD_FILE_DIRECTORY:
			o_flags |= O_DIRECTORY; break;

		default:
			return CLOD_STREAM_INVALID;
	}

	if (flags | CLOD_FILE_CREATE) {
		o_flags |= O_CREAT;
	}

	if (flags | CLOD_FILE_APPEND) {
		o_flags |= O_APPEND;
	}


	if (directory) {
		const long res = syscall(__NR_openat, (int)(long)directory->impl, (long)path, o_flags, 0664);
		if (res == -EINVAL || res == -EFAULT || res == -EBADF) return CLOD_STREAM_INVALID;
		if (res == -ENOENT) return CLOD_STREAM_EOF;
		if (res < 0) return (int)-res;
		stream_out->impl = (uintptr_t)res;

	} else {
		const long res = syscall(__NR_open, (long)path, o_flags, 0664);
		if (res == -EINVAL || res == -EFAULT || res == -EBADF) return CLOD_STREAM_INVALID;
		if (res == -ENOENT) return CLOD_STREAM_EOF;
		if (res < 0) return (int)-res;
		stream_out->impl = (uintptr_t)res;
	}

	return CLOD_STREAM_OK;
}

int clod_file_stream_read(clod_stream *self, struct clod_string *dst) {
	if (dst->cap <= dst->len)
		return CLOD_STREAM_OK;

	const long ret = syscall(__NR_read, (long)self->impl, (long)(dst->ptr + dst->len), dst->cap - dst->len);

	if (ret < 0) return (int)-ret;
	if (ret == 0) return CLOD_STREAM_EOF;
	dst->len += ret;
	return CLOD_STREAM_OK;
}

int clod_file_stream_write(clod_stream *self, struct clod_string *src) {
	if (src->len == 0)
		return CLOD_STREAM_OK;

	while (src->len) {
		const long ret = syscall(__NR_write, (long)self->impl, (long)src->ptr, src->len);

		if (ret > 0) {
			src->ptr += ret;
			src->len -= ret;
			if (src->cap) src->cap -= ret;
		}

		if (ret < 0) return (int)-ret;
	}

	return CLOD_STREAM_OK;
}

int clod_file_stream_close(clod_stream *self) {
	const long ret = syscall(__NR_close, (long)self->impl);
	if (ret < 0) return (int)-ret;
	return CLOD_STREAM_OK;
}

clod_stream *clod_stdin = &(clod_stream){
	.impl = 0,
	.read = clod_file_stream_read,
	.close = clod_file_stream_close,
};

clod_stream *clod_stdout = &(clod_stream){
	.impl = 1,
	.write = clod_file_stream_write,
	.close = clod_file_stream_close
};

clod_stream *clod_stderr = &(clod_stream){
	.impl = 2,
	.write = clod_file_stream_write,
	.close = clod_file_stream_close,
};
