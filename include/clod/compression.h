/**
* @file clod/compression.h
* @defgroup compression Compression
* @{
 */
#ifndef CLOD_COMPRESSION_H
#define CLOD_COMPRESSION_H

#include <clod/lib.h>
#include <stddef.h>
#include <stdint.h>

#define CLOD_COMPRESSION_DEFAULT_LEVEL INT8_MAX

/**
 * Compression algorithms.
 * Support for compression libraries and hence algorithms can be configured at compile time
 * and can be checked with clod_compression_support.
 *
 * Currently, the library compiles with support for all compression algorithms by default.
 */
enum clod_compression_algo {
	/** No compression. Provided by memcpy lol.
	 * Uncompressed size is always known. */
	CLOD_UNCOMPRESSED = 1,
	/** GZip compression, provided by libdeflate.
	 * Uncompressed size is not known on failure. */
	CLOD_GZIP = 2,
	/** ZLib compression, provided by libdeflate.
	 * Uncompressed size is not known on failure. */
	CLOD_ZLIB = 3,
	/** Deflate compression, provided by libdeflate.
	 * Uncompressed size is not known on failure. */
	CLOD_DEFLATE = 4,
	/** LZ4 compression, provided by liblz4.
	 * Uncompressed size might be known on failure if not malformed. */
	CLOD_LZ4 = 5,
	/** LZMA compression, provided by liblzma. */
	CLOD_LZMA = 6,
	/** ZSTD compression, provided by libzstd.
	 * Uncompressed size is known on failure if not malformed. */
	CLOD_ZSTD = 7,
	/** Custom minecraft LZ4 variant.
	 * Some very unfortunate design decisions must have happened to land us with this.
	 * Uncompressed size is known on failure if not malformed. */
	CLOD_MINECRAFT_LZ4 = 8
};

/** The compression level to use.
 * These are mapped to each compression algorithm's useful range.
 */
enum clod_compression_level {
	/** Lowest compression level. Fastest with the largest compressed sizes. */
	CLOD_COMPRESSION_LOWEST = -2,
	/** Low compression level. Faster with larger compressed sizes. */
	CLOD_COMPRESSION_LOW = -1,
	/** Normal compression level. Uses uses algorithm default values. */
	CLOD_COMPRESSION_NORMAL = 0,
	/** High compression level. Slower with smaller compressed sizes. */
	CLOD_COMPRESSION_HIGH = 1,
	/** Highest compression level. Slowest with the smallest compressed sizes. */
	CLOD_COMPRESSION_HIGHEST = 2
};

/**
 * Check if support for a given compression algorithm exists.
 * @param[in] algo Compression algorithm to check.
 * @return True if the compression algorithm is supported.
 */
CLOD_API CLOD_CONST
bool clod_compression_support(enum clod_compression_algo algo);

enum clod_compression_result {
	/** The operation was successful. */
	CLOD_COMPRESSION_SUCCESS = 0,
	/** The specified algorithm isn't supported. */
	CLOD_COMPRESSION_UNSUPPORTED = 1,
	/** The compressed data is malformed. */
	CLOD_COMPRESSION_MALFORMED = 2,
	/** The provided buffer is too short to hold the output. */
	CLOD_COMPRESSION_SHORT_BUFFER = 3,
	/** The output buffer is larger than the output, and actual_size was null. */
	CLOD_COMPRESSION_SHORT_OUTPUT = 4,
	/** Allocation failed. */
	CLOD_COMPRESSION_ALLOC_FAILED = 5
};

struct clod_compressor;
struct clod_decompressor;

/**
 * Create a new compressor.
 * The compressor holds memory and tables that are reused between invocations of clod_compress.
 * It is not a streaming interface, but an optimisation.
 *
 * @return Newly allocated compressor, or nullptr on allocation failure.
 */
CLOD_API CLOD_USE_RETURN
struct clod_compressor *clod_compressor_init();

/**
 * Releases resources associated with the compressor.
 *
 * @param ctx Compressor to free.
 */
CLOD_API CLOD_NONNULL(1)
void clod_compressor_free(struct clod_compressor *ctx);

/**
 * Compress data with a given compression algorithm.
 *
 * @param[in] ctx Compressor.
 * @param[out] dst Where compressed data is written.
 * @param[in] dst_max_size Size of \p dst.
 * @param[in] src Where data to be compressed is read from.
 * @param[in] src_size Size of data to compress in \p src.
 * @param[out] actual_size The actual size of the compressed output.
 * If the compressed size is known, \p actual_size returns this size, even if compression failed.
 * If the compressed size is not known (only true on failure), it is set to 0.
 * @param[in] algo The compression algorith to use.
 * @param[in] level Compression level to use.
 *
 * @return Result of compression.
 * @throws CLOD_COMPRESSION_SUCCESS On successful compression.
 * @throws CLOD_COMPRESSION_UNSUPPORTED If the compression algorithm is unsupported.
 * @throws CLOD_COMPRESSION_SHORT_BUFFER If \p dst is too small to hold the compressed output.
 * @throws CLOD_COMPRESSION_ALLOC_FAILED If memory allocation failed.
 */
CLOD_API CLOD_NONNULL(1, 2, 4, 6)
enum clod_compression_result clod_compress(
	struct clod_compressor *ctx,
	void *dst, size_t dst_max_size,
	const void *src, size_t src_size,
	size_t *actual_size,
	enum clod_compression_algo algo,
	enum clod_compression_level level
);

/**
 * Create a new decompressor.
 * The decompressor holds memory and tables that are reused between invocations of clod_decompress.
 * It is not a streaming interface, but an optimisation.
 *
 * @return Newly allocated decompressor, or nullptr on allocation failure.
 */
CLOD_API CLOD_USE_RETURN
struct clod_decompressor *clod_decompressor_init();

/**
 * Release resources associated with the decompressor.
 *
 * @param ctx Decompressor to free.
 */
CLOD_API CLOD_NONNULL(1)
void clod_decompressor_free(struct clod_decompressor *ctx);

/**
 * Decompress some data with the given algorithm.
 *
 * It is recommended to always know the decompressed size ahead of time,
 * and as such, use a null \p actual_size value.
 * If \p actual_size is null and the decompressed output is smaller than \p dst_size,
 * the method will fail and return CLOD_COMPRESSION_SHORT_OUTPUT.
 * If \p dst_size is smaller than the decompressed output, the method fails with
 * CLOD_COMPRESSION_SHORT_BUFFER.
 * Sometimes compressed data formats store an uncompressed size in a header,
 * which allows the uncompressed size to be known without decompressing successfully.
 * In that case, \p actual_size is set to the uncompressed size even on failure.
 *
 * @param[in] ctx Decompressor.
 * @param[out] dst Where decompressed data is written.
 * @param[in] dst_size Size of \p dst.
 * @param[in] src Where compressed data is read from.
 * @param[in] src_size Size of compressed data in \p src.
 * @param[out] actual_size The actual size of the decompressed output.
 * @param algo The compression algorithm to use.
 *
 * @return The result of the decompression.
 * @throws CLOD_COMPRESSION_SUCCESS On successful decompression.
 * @throws CLOD_COMPRESSION_UNSUPPORTED If the compression algorithm is unsupported.
 * @throws CLOD_COMPRESSION_MALFORMED If the compressed data is malformed. *THIS IS NOT AN INTEGRITY CHECK.*
 * @throws CLOD_COMPRESSION_SHORT_BUFFER If \p dst is too small to hold the decompressed output.
 * @throws CLOD_COMPRESSION_SHORT_OUTPUT If \p dst is larger than the decompressed output and bytes_written is null.
 * @throws CLOD_COMPRESSION_ALLOC_FAILED If memory allocation failed.
 */
CLOD_API CLOD_NONNULL(1, 2, 4)
enum clod_compression_result clod_decompress(
	struct clod_decompressor *ctx,
	void *dst, size_t dst_size,
	const void *src, size_t src_size,
	size_t *actual_size,
	enum clod_compression_algo algo
);

/**
 * @}
 */

#endif
