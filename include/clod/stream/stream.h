/**
 * @file stream.h
 * @defgroup stream Data streaming methods.
 */
#ifndef LIBCLOD_STREAM_H
#define LIBCLOD_STREAM_H

#include <clod/lib.h>
#include <clod/string.h>
#include <clod/errors.h>

typedef struct clod_stream clod_stream;

/// Standard input.
CLOD_API
extern clod_stream *clod_stream_stdin;

/// Standard output.
CLOD_API
extern clod_stream *clod_stream_stdout;

/// Standard error.
CLOD_API
extern clod_stream *clod_stream_stderr;

/**
 * Stream of data.
 * It roughly replaces libc's FILE, but aims to support user implementations,
 * and has additional library implementations not supported by FILE such as networking.
 *
 * All members are optional and may be omitted for streams to whom they have no relevance.
 * A stream that supports no methods is valid, for example a directory for whom the user has no read permission
 * can be opened, and such a directory is only useful as a relative path reference for opening files.
 *
 * Negative error codes are standardised across all stream implementations and platforms, and are defined at the
 * top of this file, while positive error codes are implementation and platform-dependent.
 * The absolute minimum number of translated error codes shall be used, and only when there is a clear
 * case for runtime code path switching should they be used. They are not to be used as debugging tools
 * or user messages. They are exclusively for programs to change their behaviour based on the error code.
 *
 * For example, distinguishing between EBADF, EFAULT, EINVAL... from linux file write syscall should not be implemented,
 * as none of them provide useful information to the program at runtime. The only thing the program needs
 * to know is "the file does not work" e.g. to avoid an infinite loop. On the other hand, distinguishing
 * the normal end of a TCP connection from other errors can be important, as, instead of treating the connection
 * as failed, the program may which to attempt to reconnect. To that end EPIPE is translated to CLOD_STREAM_EOF.
 */
struct clod_stream {
	/**
	 * A stream's read method shall read a portion of data from the stream
	 * or return a non-zero error code. If \p read is null the stream does not
	 * support reading.
	 *
	 * The new data is appended to the destination buffer using its free capacity.
	 *
	 * @param[in] self Pointer to this stream.
	 * @param[in] dst Destination buffer.
	 * @return 0 on success, non-zero error code on failure.
	 */
	int (*read)(clod_stream *self, struct clod_string *dst);

	/**
	 * A stream's write method shall write the entire portion of data to the stream or
	 * return a non-zero error code. In such a case some data may have still been written.
	 * The amount of written data can be discerned through \p src, as the successfully written
	 * data is removed from the string.
	 * If \p write is null the stream does not support writing.
	 *
	 * The written data is removed from the source buffer by incrementing its pointer
	 * and adjusting other parameters as such.
	 *
	 * @param[in] self Pointer to this stream.
	 * @param[in] src Source buffer.
	 * @return 0 on success, non-zero error code on failure.
	 */
	int (*write)(clod_stream *self, struct clod_string *src);

	/**
	 * A stream's close method shall close the stream and release any resources
	 * associated with it. If \p close is null the stream does not need to be closed.
	 * Notably, some streams which don't have an underlying close method will still
	 * need a close function to free the clod_stream object itself from memory.
	 *
	 * @note The stream cannot be interacted with following a call to \p close.
	 * Conversely, close should free the clod_stream from memory if relevant.
	 *
	 * @param[in] self Pointer to this stream.
	 * @return 0 on success, non-zero error code on failure.
	 */
	int (*close)(clod_stream *self);
};

/**
 * Copies data from src to dst until EOF or an error occurs.
 * @param[in] dst Where data is written to.
 * @param[in] src Where data is read from.
 * @param[in] buffer Staging buffer used for the copy.
 * @param[in] buffer_size Size of the staging buffer,
 * @param[out] total_transferred (nullable) Total number of bytes transferred.
 * @return 0 on success, non-zero on failure.
 * Notably, \p src returning EOF is the only way this method returns a success (0) value.
 * If \p dst returns EOF, EOF is returned like any other error.
 */
CLOD_API CLOD_NONNULL(1, 2, 3)
int clod_stream_copy(clod_stream *dst, clod_stream *src, void *buffer, size_t buffer_size, size_t *total_transferred);

/**
 * Create an unbuffered bidirectional pipe.
 * There is no overhead to bidirectionality when used in a unidirectional context.
 * Data written to one stream is readable from the other and vice versa.
 * Each call to write blocks until the written data has been completely read from the other stream.
 * Both streams must be individually closed.
 * @param[out] pipe1_out One half of the pipe.
 * @param[out] pipe2_out Second half of the pipe.
 */
CLOD_API CLOD_NONNULL(1, 2)
void clod_stream_pipe(clod_stream *pipe1_out, clod_stream *pipe2_out);

/**
 * Write a formatted string to the given stream.
 * It only supports up to 256 characters.
 * @param[in] dst Stream to write to.
 * @param[in] fmt Format string.
 * @param[in] ... Values to format.
 * @return Result of the write call to \p dst.
 */
CLOD_API CLOD_NONNULL(1)
int clod_stream_format(clod_stream *dst, struct clod_string fmt, ...);

/// Same as clod_stream_format, but takes a va_list argument instead of '...'.
CLOD_API CLOD_NONNULL(1)
int clod_stream_vformat(clod_stream *dst, struct clod_string fmt, va_list args);

#endif
