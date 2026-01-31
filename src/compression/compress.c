#include <limits.h>

#include "compression_config.h"
#include <clod/compression.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_LIBDEFLATE
#include <libdeflate.h>
#endif

#if HAVE_LIBLZ4
#include <lz4hc.h>
#include <lz4frame.h>
#endif

#if HAVE_LIBLZMA
#include <lzma.h>
#endif

#if HAVE_LIBZSTD
#include <zstd.h>
#endif

#if HAVE_LIBBZ2
#include <bzlib.h>
#endif

struct clod_compressor {
	void *(*malloc_func)(size_t);
	void (*free_func)(void *);

#if HAVE_LIBDEFLATE
	struct libdeflate_compressor *libdeflate_compressor[CLOD_COMPRESSION_LEVELS];
#endif

#if HAVE_LIBZSTD
	ZSTD_CCtx *zstd_cctx;
#endif
};

#if HAVE_LIBLZMA
void *compressor_lzma_malloc(void *user, size_t n, size_t size) {
	auto const ctx = (struct clod_compressor *)user;
	return ctx->malloc_func(n * size);
}
void compressor_lzma_free(void *user, void *address) {
	auto const ctx = (struct clod_compressor *)user;
	ctx->free_func(address);
}
#endif

#if HAVE_LIBBZ2
void *compressor_bz2_malloc(void *user, int n, int size) {
	auto const ctx = (struct clod_compressor *)user;
	return ctx->malloc_func((size_t)n * (size_t)size);
}
void compressor_bz2_free(void *user, void *address) {
	auto const ctx = (struct clod_compressor *)user;
	ctx->free_func(address);
}
#endif

struct clod_compressor *clod_compressor_init() {
	struct clod_compressor *ctx = malloc(sizeof(struct clod_compressor));
	memset(ctx, 0, sizeof(*ctx));
	// LZ4 and ZSTD are missing custom memory allocation methods.
	ctx->malloc_func = malloc;
	ctx->free_func = free;
	return ctx;
}

void clod_compressor_free(struct clod_compressor *ctx) {
#if HAVE_LIBDEFLATE
	for (int i = 0; i < CLOD_COMPRESSION_LEVELS; i++)
		if (ctx->libdeflate_compressor[i])
			libdeflate_free_compressor(ctx->libdeflate_compressor[i]);
#endif

#if HAVE_LIBZSTD
	if (ctx->zstd_cctx)
		ZSTD_freeCCtx(ctx->zstd_cctx);
#endif

	ctx->free_func(ctx);
}

enum clod_compression_result
clod_compress(struct clod_compressor *ctx,
	void *dst, const size_t dst_max_size,
	const void *src, const size_t src_size,
	size_t *actual_size,
	const enum clod_compression_method method,
	const enum clod_compression_level level
) {
	switch (method) {
		case CLOD_UNCOMPRESSED: {
			if (dst_max_size < src_size) {
				return CLOD_COMPRESSION_SHORT_BUFFER;
			}
			memcpy(dst, src, src_size);
			*actual_size = src_size;
			return CLOD_COMPRESSION_SUCCESS;
		}
		case CLOD_GZIP: {
			#if HAVE_LIBDEFLATE

				struct libdeflate_compressor *compressor = ctx->libdeflate_compressor[level];
				if (!compressor) {
					const int libdeflate_level =
						level == CLOD_COMPRESSION_HIGHEST ? 12 :
						level == CLOD_COMPRESSION_HIGH ? 9 :
						level == CLOD_COMPRESSION_NORMAL ? 6 :
						level == CLOD_COMPRESSION_LOW ? 3 :
						level == CLOD_COMPRESSION_LOWEST ? 1 :
						6;

					struct libdeflate_options opts = {0};
					opts.sizeof_options = sizeof(opts);
					opts.free_func = ctx->free_func;
					opts.malloc_func = ctx->malloc_func;

					compressor = libdeflate_alloc_compressor_ex(libdeflate_level, &opts);
					if (!compressor) {
						return CLOD_COMPRESSION_ALLOC_FAILED;
					}
					ctx->libdeflate_compressor[level] = compressor;
				}

				const size_t size = libdeflate_gzip_compress(compressor,
					src, src_size,
					dst, dst_max_size);

				if (size == 0) return CLOD_COMPRESSION_SHORT_BUFFER;

				*actual_size = size;
				return CLOD_COMPRESSION_SUCCESS;
			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif
		}
		case CLOD_ZLIB: {
			#if HAVE_LIBDEFLATE

				struct libdeflate_compressor *compressor = ctx->libdeflate_compressor[level];
				if (!compressor) {
					const int libdeflate_level =
						level == CLOD_COMPRESSION_HIGHEST ? 12 :
						level == CLOD_COMPRESSION_HIGH ? 9 :
						level == CLOD_COMPRESSION_NORMAL ? 6 :
						level == CLOD_COMPRESSION_LOW ? 3 :
						level == CLOD_COMPRESSION_LOWEST ? 1 :
						6;

					struct libdeflate_options opts = {0};
					opts.sizeof_options = sizeof(opts);
					opts.free_func = ctx->free_func;
					opts.malloc_func = ctx->malloc_func;

					compressor = libdeflate_alloc_compressor_ex(libdeflate_level, &opts);
					if (!compressor) {
						return CLOD_COMPRESSION_ALLOC_FAILED;
					}
					ctx->libdeflate_compressor[level] = compressor;
				}

				const size_t size = libdeflate_zlib_compress(compressor,
					src, src_size,
					dst, dst_max_size);

				if (size == 0) return CLOD_COMPRESSION_SHORT_BUFFER;

				*actual_size = size;
				return CLOD_COMPRESSION_SUCCESS;
			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif
		}
		case CLOD_DEFLATE: {
			#if HAVE_LIBDEFLATE

				struct libdeflate_compressor *compressor = ctx->libdeflate_compressor[level];
				if (!compressor) {
					const int libdeflate_level =
						level == CLOD_COMPRESSION_HIGHEST ? 12 :
						level == CLOD_COMPRESSION_HIGH ? 9 :
						level == CLOD_COMPRESSION_NORMAL ? 6 :
						level == CLOD_COMPRESSION_LOW ? 3 :
						level == CLOD_COMPRESSION_LOWEST ? 1 :
						6;

					struct libdeflate_options opts = {0};
					opts.sizeof_options = sizeof(opts);
					opts.free_func = ctx->free_func;
					opts.malloc_func = ctx->malloc_func;

					compressor = libdeflate_alloc_compressor_ex(libdeflate_level, &opts);
					if (!compressor) {
						return CLOD_COMPRESSION_ALLOC_FAILED;
					}
					ctx->libdeflate_compressor[level] = compressor;
				}

				const size_t size = libdeflate_deflate_compress(compressor,
					src, src_size,
					dst, dst_max_size);

				if (size == 0) return CLOD_COMPRESSION_SHORT_BUFFER;

				*actual_size = size;
				return CLOD_COMPRESSION_SUCCESS;
			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif
		}
		case CLOD_LZ4F: {
			#if HAVE_LIBLZ4

				LZ4F_preferences_t prefs = LZ4F_INIT_PREFERENCES;
				prefs.frameInfo.contentSize = src_size;
				prefs.compressionLevel =
					level == CLOD_COMPRESSION_HIGHEST ? LZ4HC_CLEVEL_MAX :
					level == CLOD_COMPRESSION_HIGH ? LZ4HC_CLEVEL_OPT_MIN :
					level == CLOD_COMPRESSION_NORMAL ? LZ4HC_CLEVEL_DEFAULT :
					level == CLOD_COMPRESSION_LOW ? LZ4HC_CLEVEL_MIN :
					level == CLOD_COMPRESSION_LOWEST ? 0 :
					LZ4HC_CLEVEL_DEFAULT;

				const size_t size = LZ4F_compressFrame(
					dst, dst_max_size,
					src, src_size,
					&prefs);

				if (LZ4F_isError(size)) {
					return CLOD_COMPRESSION_SHORT_BUFFER;
				}

				*actual_size = size;
				return CLOD_COMPRESSION_SUCCESS;

			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif
		}
		case CLOD_XZ: {
			#if HAVE_LIBLZMA

				const lzma_allocator allocator = {
					.alloc = compressor_lzma_malloc,
					.free = compressor_lzma_free,
					.opaque = ctx,
				};

				lzma_stream stream = LZMA_STREAM_INIT;
				stream.allocator = &allocator;

				const uint32_t preset =
					level == CLOD_COMPRESSION_HIGHEST ? LZMA_PRESET_EXTREME | 9 :
					level == CLOD_COMPRESSION_HIGH ? 8 :
					level == CLOD_COMPRESSION_NORMAL ? 6 :
					level == CLOD_COMPRESSION_LOW ? 3 :
					level == CLOD_COMPRESSION_LOWEST ? 1 :
					6;

				lzma_ret ret = lzma_easy_encoder(&stream, preset, LZMA_CHECK_CRC64);
				if (ret != LZMA_OK) {
					return ret == LZMA_MEM_ERROR ? CLOD_COMPRESSION_ALLOC_FAILED : CLOD_COMPRESSION_UNSUPPORTED;
				}

				stream.next_in = (uint8_t *)src;
				stream.avail_in = src_size;
				stream.next_out = (uint8_t *)dst;
				stream.avail_out = dst_max_size;

				do {
					ret = lzma_code(&stream, LZMA_FINISH);
				} while (ret == LZMA_OK);

				lzma_end(&stream);

				switch (ret) {
					case LZMA_STREAM_END:
						*actual_size = stream.total_out;
						return CLOD_COMPRESSION_SUCCESS;
					case LZMA_BUF_ERROR:
						if (stream.avail_out == 0) {
							*actual_size = 0;
							return CLOD_COMPRESSION_SHORT_BUFFER;
						}
						return CLOD_COMPRESSION_UNSUPPORTED;
					case LZMA_MEM_ERROR: return CLOD_COMPRESSION_ALLOC_FAILED;
					default: return CLOD_COMPRESSION_UNSUPPORTED;
				}

			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif
		}
		case CLOD_ZSTD: {
			#if HAVE_LIBZSTD
				if (!ctx->zstd_cctx) {
					ctx->zstd_cctx = ZSTD_createCCtx();
					if (!ctx->zstd_cctx) return CLOD_COMPRESSION_ALLOC_FAILED;
				}

				const int compression_level =
					level == CLOD_COMPRESSION_HIGHEST ? ZSTD_maxCLevel() :
					level == CLOD_COMPRESSION_HIGH ? 14 :
					level == CLOD_COMPRESSION_NORMAL ? 5 :
					level == CLOD_COMPRESSION_LOW ? 3 :
					level == CLOD_COMPRESSION_LOWEST ? 1 :
					3;

				const size_t size = ZSTD_compressCCtx(ctx->zstd_cctx,
					dst, dst_max_size,
					src, src_size,
					compression_level
				);
				switch (ZSTD_getErrorCode(size)) {
					case ZSTD_error_no_error:
						*actual_size = size;
						return CLOD_COMPRESSION_SUCCESS;
					case ZSTD_error_dstSize_tooSmall:
						return CLOD_COMPRESSION_SHORT_BUFFER;
					case ZSTD_error_memory_allocation:
						return CLOD_COMPRESSION_ALLOC_FAILED;
					default:
						return CLOD_COMPRESSION_UNSUPPORTED;
				}
			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif
		}
		case CLOD_BZIP2: {
			#if HAVE_LIBBZ2
				// TODO: support sizes > UINT32_MAX
				if (src_size > UINT_MAX)
					return CLOD_COMPRESSION_UNSUPPORTED;

				bz_stream stream = {0};
				stream.next_in = (char *)src;
				stream.avail_in = (unsigned)src_size;
				stream.next_out = (char *)dst;
				stream.avail_out = (unsigned)dst_max_size;
				stream.bzalloc = compressor_bz2_malloc;
				stream.bzfree = compressor_bz2_free;
				stream.opaque = ctx;

				const int block_size =
					level == CLOD_COMPRESSION_HIGHEST ? 9 :
					level == CLOD_COMPRESSION_HIGH ? 7 :
					level == CLOD_COMPRESSION_NORMAL ? 5 :
					level == CLOD_COMPRESSION_LOW ? 3 :
					level == CLOD_COMPRESSION_LOWEST ? 1 :
					6;

				int res = BZ2_bzCompressInit(&stream, block_size, 0, 0);
				if (res == BZ_MEM_ERROR) return CLOD_COMPRESSION_ALLOC_FAILED;
				if (res != BZ_OK) return CLOD_COMPRESSION_UNSUPPORTED;

				do {
					if (stream.avail_out == 0) {
						BZ2_bzCompressEnd(&stream);
						*actual_size = 0;
						return CLOD_COMPRESSION_SHORT_BUFFER;
					}
					res = BZ2_bzCompress(&stream, BZ_FINISH);
				} while (res == BZ_FINISH_OK);

				BZ2_bzCompressEnd(&stream);

				switch (res) {
					case BZ_STREAM_END:
						*actual_size = (size_t)stream.total_out_lo32 + ((size_t)stream.total_out_hi32 << 32);
						return CLOD_COMPRESSION_SUCCESS;
					case BZ_MEM_ERROR: return CLOD_COMPRESSION_ALLOC_FAILED;
					default: return CLOD_COMPRESSION_UNSUPPORTED;
				}

			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif
		}
		default: {
			return CLOD_COMPRESSION_UNSUPPORTED;
		}
	}
}
