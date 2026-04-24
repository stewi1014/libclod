#ifndef LIBCLOD_FILE_H
#define LIBCLOD_FILE_H

#include <clod/lib.h>
#include <clod/stream/stream.h>

#define CLOD_FILE_READ 1
#define CLOD_FILE_WRITE 2
#define CLOD_FILE_TRUNCATE 4
#define CLOD_FILE_CREATE 8
#define CLOD_FILE_APPEND 16
#define CLOD_FILE_DIRECTORY 32

#define CLOD_DIRENT_FILE 128
#define CLOD_DIRENT_DIRECTORY 129

/**
 * When reading a directory stream, the read buffer is filled with these structs.
 */
struct clod_dirent {
	/// Pointer to the next directory entry.
	struct clod_dirent *next;

	/// Unique identifier for the entry.
	uintptr_t id;

	/// Size of \p name.
	unsigned short name_size;

	/// Type of the entry.
	unsigned char type;

	/// Name of the entry.
	char name[];
};

typedef struct {
	clod_stream stream;

	union {
		int unix_fd;
		void *_reserved;
	};
} clod_file;

/**
 * Open a file for reading and/or writing with the stream API.
 * @param[out] file_out New file stream.
 * @param[in] directory (nullable) Directory from which path will be resolved.
 * @param[in] path (nullable) Path to the file. Reopens directory if null.
 * @param[in] flags Open flags.
 * @return 0 on success, non-zero on error.
 */
CLOD_API CLOD_NONNULL(1)
int clod_stream_file(
	clod_file *file_out,
	const clod_file *directory,
	const char *path,
	int flags
);

#endif
