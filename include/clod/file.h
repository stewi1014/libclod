#ifndef LIBCLOD_FILE_H
#define LIBCLOD_FILE_H

#include <clod/lib.h>
#include <clod/stream.h>
#include <clod/memory.h>

#define CLOD_FILE_READ 1
#define CLOD_FILE_WRITE 2
#define CLOD_FILE_TRUNCATE 4
#define CLOD_FILE_CREATE 8
#define CLOD_FILE_APPEND 16
#define CLOD_FILE_DIRECTORY 32

/**
 * Open a file for reading and/or writing with the stream API.
 * @param[out] stream_out New file stream.
 * @param[in] directory (nullable) Directory from which path will be resolved.
 * @param[in] path (nullable) Path to the file. Reopens directory if null.
 * @param[in] flags Open flags.
 * @return 0 on success, non-zero on error.
 */
CLOD_API CLOD_NONNULL(1)
int clod_file(
	clod_stream *stream_out,
	const clod_stream *directory,
	const char *path,
	int flags
);

#endif
