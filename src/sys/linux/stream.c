#include <clod/stream.h>
#include "syscall.h"

struct clod_file {
	clod_stream stream;
	int unix_fd;
};

int clod_file_stream_read(clod_stream *self, struct clod_string *dst) {
	struct clod_file *file_self = (struct clod_file*)self;
	if (dst->cap <= dst->len)
		return CLOD_STREAM_OK;

	long ret = syscall_read(
		file_self->unix_fd,
		dst->ptr + dst->len,
		(size_t)(dst->cap - dst->len)
	);

	if (ret < 0) return (int)-ret;
	if (ret == 0) return CLOD_STREAM_EOF;
	dst->len += ret;
	return CLOD_STREAM_OK;
}

int clod_file_stream_write(clod_stream *self, struct clod_string *src) {
	struct clod_file *file_self = (struct clod_file*)self;
	if (src->len == 0)
		return CLOD_STREAM_OK;

	while (src->len) {
		long ret = syscall_write(
			file_self->unix_fd,
			src->ptr,
			(size_t)src->len
		);

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
	struct clod_file *file_self = (struct clod_file*)self;
	long ret = syscall_close(file_self->unix_fd);
	if (ret < 0) return (int)-ret;
	return CLOD_STREAM_OK;
}

clod_stream *clod_stdin = &(struct clod_file){
	.stream = {
		.read = clod_file_stream_read,
		.close = clod_file_stream_close,
	},
	.unix_fd = 0
}.stream;

clod_stream *clod_stdout = &(struct clod_file){
	.stream = {
		.write = clod_file_stream_write,
		.close = clod_file_stream_close,
	},
	.unix_fd = 1
}.stream;

clod_stream *clod_stderr = &(struct clod_file){
	.stream = {
		.write = clod_file_stream_write,
		.close = clod_file_stream_close,
	},
	.unix_fd = 2
}.stream;
