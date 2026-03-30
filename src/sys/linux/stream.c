#include <clod/stream.h>
#include "syscall.h"
#include "clod/file.h"
#include <dirent.h>

int clod_file_stream_close(clod_stream *self);
int clod_file_stream_read(clod_stream *self, struct clod_string *dst);
int clod_file_stream_readdir(clod_stream *self, struct clod_string *dst);
int clod_file_stream_write(clod_stream *self, struct clod_string *src);

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0100
#define O_TRUNC 01000
#define O_APPEND 02000
#define O_DIRECTORY 0200000

struct linux_dirent {
	long int ino;
	long int off;
	unsigned short size;
	unsigned char type;
	char name[];
};
static_assert(offsetof(struct linux_dirent, name) == offsetof(struct clod_dirent, name));

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

	if (flags & CLOD_FILE_CREATE) {
		o_flags |= O_CREAT;
	}

	if (flags & CLOD_FILE_APPEND) {
		o_flags |= O_APPEND;
	}

	long res;
	if (directory)
		res = syscall(__NR_openat, (int)(long)directory->impl, (long)path, o_flags, 0664);
	else
		res = syscall(__NR_open, (long)path, o_flags, 0664);

	if (res == -EINVAL || res == -EFAULT || res == -EBADF) return CLOD_STREAM_INVALID;
	if (res == -ENOENT) return CLOD_STREAM_EOF;
	if (res < 0) return (int)-res;

	stream_out->impl = (uintptr_t)res;

	if (flags & CLOD_FILE_READ) {
		if (flags & CLOD_FILE_DIRECTORY) {
			stream_out->read = clod_file_stream_readdir;
		} else {
			stream_out->read = clod_file_stream_read;
		}
	} else {
		stream_out->read = nullptr;
	}

	if (flags & CLOD_FILE_WRITE) {
		stream_out->write = clod_file_stream_write;
	} else {
		stream_out->write = nullptr;
	}

	stream_out->close = clod_file_stream_close;

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
int clod_file_stream_readdir(clod_stream *self, struct clod_string *dst) {
	if (dst->cap <= dst->len)
		return CLOD_STREAM_OK;

	long res = syscall(__NR_getdents64, (long)self->impl, (long)(dst->ptr + dst->len), dst->cap - dst->len);
	if (res == 0) return CLOD_STREAM_EOF;
	if (res == -EBADF) return CLOD_STREAM_INVALID;
	if (res < 0) return (int)-res;

	size_t off = 0;
	while (off < (size_t)res) {
		void *ptr = dst->ptr + dst->len + off;
		const struct linux_dirent ent = *(struct linux_dirent*)ptr;
		struct clod_dirent *clod_ent = ptr;

		off += ent.size;

		clod_ent->next = off < (size_t)res ? (struct clod_dirent*)((char*)ptr + ent.size) : nullptr;
		clod_ent->id = (uintptr_t)ent.ino;
		clod_ent->name_size = ent.size - (unsigned short)offsetof(struct linux_dirent, name) - 1;
		switch (ent.type) {
			case DT_DIR: clod_ent->type = CLOD_DIRENT_DIRECTORY; break;
			case DT_REG: clod_ent->type = CLOD_DIRENT_FILE; break;
			default: clod_ent->type = ent.type;
		}
	}
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
