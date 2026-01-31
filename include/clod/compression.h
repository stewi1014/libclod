/**
 * @file clod/compression.h
 * @defgroup compression Compression
 * @{
 *
 * These compression methods provide a generic interface for compressing data using
 * a variety of compression algorithms. Support for compression algorithms
 * can be configured at compile time through enabling or disabling the libraries that provide them.
 *
 * Each compressed output is independent of the last, and no external state (i.e. dictionaries) exists.
 * Some algorithms are only well optimised for small data sizes when used
 * in a way which breaks one of those two properties.
 * As such, compressing many small blobs of data can result in bad compression ratios
 * and static overheads making relative performance worse.
 */
#ifndef CLOD_COMPRESSION_H
#define CLOD_COMPRESSION_H

#include <clod/lib.h>
#include <stddef.h>

/**
 * Compression methods.
 *
 * Support for compression libraries and hence methods can be configured at compile time
 * and can be checked with clod_compression_support. Currently, the library
 * compiles with support for all compression methods by default.
 *
 * Naming follows CLOD_<CONTAINER>_<ALGORITHM>, with algorithm being omitted
 * if it is abundantly obvious (i.e. container only supports one compression algorithm).
 */
enum clod_compression_method {
	/** No compression, provided by memcpy lol. */
	CLOD_UNCOMPRESSED = 1,

	/** GZip container with deflate compression, provided by libdeflate.
	 * Uncompressed size is not known on failure. */
	CLOD_GZIP = 2,

	/** ZLib container with deflate compression, provided by libdeflate.
	 * Uncompressed size is not known on failure. */
	CLOD_ZLIB = 3,

	/** Deflate compression, provided by libdeflate.
	 * Uncompressed size is not known on failure. */
	CLOD_DEFLATE = 4,

	/** LZ4F container with LZ4 compression, provided by liblz4.
	 * Uncompressed size might be known on failure. */
	CLOD_LZ4F = 5,

	/** XZ container with LZMA2 compression, provided by liblzma.
	 * Uncompressed size is not known on failure. */
	CLOD_XZ = 6,

	/** ZSTD compression, provided by libzstd.
	 * Uncompressed size might be known on failure.
	 * This is probably the best choice for most use cases. */
	CLOD_ZSTD = 7,

	/** BZIP2 compression, provided by libbz2.
	 * Uncompressed size is not known on failure. */
	CLOD_BZIP2 = 8,

	/** Custom minecraft container with LZ4 compression.
	 * Some very unfortunate design decisions must have been made to land us with this.
	 * Uncompressed size might be known on failure. */
	CLOD_MINECRAFT_LZ4 = 10
};

/** The compression level to use.
 * These are mapped to each compression algorithm's useful range.
 */
enum clod_compression_level {
	/** Lowest compression level. Fastest with the largest compressed sizes. */
	CLOD_COMPRESSION_LOWEST = 0,
	/** Low compression level. Faster with larger compressed sizes. */
	CLOD_COMPRESSION_LOW = 1,
	/** Normal compression level. Uses uses algorithm default values. */
	CLOD_COMPRESSION_NORMAL = 2,
	/** High compression level. Slower with smaller compressed sizes. */
	CLOD_COMPRESSION_HIGH = 3,
	/** Highest compression level. Slowest with the smallest compressed sizes. */
	CLOD_COMPRESSION_HIGHEST = 4,
	/** Number of compression levels. */
	CLOD_COMPRESSION_LEVELS
};

/**
 * Check if support for a given compression algorithm exists.
 * @param[in] method Compression algorithm to check.
 * @return True if the compression algorithm is supported.
 */
CLOD_API CLOD_CONST
bool clod_compression_support(enum clod_compression_method method);

/**
 * Result of a compression operation.
 */
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

/**
 * @struct clod_compressor
 * Memory and tables reusable across invocations of clod_compress.
 */
struct clod_compressor;

/**
 * @struct clod_decompressor
 * Memory and tables reusable across invoations of clod_decompress.
 */
struct clod_decompressor;

/**
 * Create a new compressor.
 * The compressor holds memory and tables that are reused between invocations of clod_compress.
 * It is not a streaming interface, but an optimisation.

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
 * Compress data with a given compression method.
 *
 * @param[in] ctx Compressor.
 * @param[out] dst Where compressed data is written.
 * @param[in] dst_max_size Size of \p dst.
 * @param[in] src Where data to be compressed is read from.
 * @param[in] src_size Size of data to compress in \p src.
 * @param[out] actual_size The actual size of the compressed output.
 * @param[in] method The compression method to use.
 * @param[in] level Compression level to use.
 *
 * @return Result of compression.
 * @throws CLOD_COMPRESSION_SUCCESS On successful compression.
 * @throws CLOD_COMPRESSION_UNSUPPORTED If the compression method is unsupported.
 * @throws CLOD_COMPRESSION_SHORT_BUFFER If \p dst is too small to hold the compressed output.
 * @throws CLOD_COMPRESSION_ALLOC_FAILED If memory allocation failed.
 */
CLOD_API CLOD_NONNULL(1, 2, 4, 6)
enum clod_compression_result clod_compress(
	struct clod_compressor *ctx,
	void *dst, size_t dst_max_size,
	const void *src, size_t src_size,
	size_t *actual_size,
	enum clod_compression_method method,
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
 * Decompress some data with the given method.
 *
 * The nullability of \p actual_size informs clod_decompress if the size of the uncompressed output is known.
 * If the size is known, \p actual_size should be null, and the size passed to \p dst_size.
 * If the size is unknown, \p actual_size must be non-null and returns the size of the uncompressed output.
 * It is strongly recommended to know the uncompressed size ahead of time.
 *
 * When CLOD_COMPRESSION_SHORT_BUFFER is returned, it is sometimes possible to know the
 * uncompressed size even without successfully decompressing the whole buffer.
 * In that case, \p actual_size might be set to the true uncompressed size.
 *
 * @param[in] ctx Decompressor.
 * @param[out] dst Where decompressed data is written.
 * @param[in] dst_size Size of \p dst.
 * @param[in] src Where compressed data is read from.
 * @param[in] src_size Size of compressed data in \p src.
 * Decompression will _probably_ work as expected if you supply a value larger than the compressed size.
 * It's method-dependent, and I'm not giving you any more than that in the public API.
 * @param[out] actual_size The actual size of the decompressed output.
 * If CLOD_COMPRESSION_SHORT_BUFFER is returned and the uncompressed size can be discerned,
 * \p actual_size might be set to the size of uncompressed data that would have been returned.
 * @param[in] method The compression method to use.
 *
 * @return The result of the decompression.
 * @throws CLOD_COMPRESSION_SUCCESS On successful decompression.
 * @throws CLOD_COMPRESSION_UNSUPPORTED If the compression method is unsupported.
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
	enum clod_compression_method method
);

/**
 * @}
 */

#endif
