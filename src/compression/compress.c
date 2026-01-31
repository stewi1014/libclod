#include "compression_config.h"
#include <clod/compression.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_LIBDEFLATE
#include <libdeflate.h>
#endif

#if HAVE_LZ4
#include <lz4.h>
#include <lz4frame.h>
#include <lz4hc.h>
#endif

#if HAVE_LZMA
#include <lzma.h>
#endif

#if HAVE_ZSTD
#include <zstd.h>
#endif

struct clod_compressor {
#if HAVE_LIBDEFLATE
	struct libdeflate_compressor *libdeflate_compressor;
	int libdeflate_compressor_level;
#endif

#if HAVE_LZ4
	LZ4_stream_t *lz4_stream;
	LZ4_streamHC_t *lz4hc_stream;
#endif

#if HAVE_LZMA
	lzma_stream *lzma_stream;
#endif

#if HAVE_ZSTD
	ZSTD_CCtx *zstd_compressor;
#endif
};

struct clod_compressor *clod_compressor_init() {
	struct clod_compressor *ctx = malloc(sizeof(struct clod_compressor));
	memset(ctx, 0, sizeof(*ctx));
	return ctx;
}

void clod_compressor_free(struct clod_compressor *ctx) {
#if HAVE_LIBDEFLATE
	if (ctx->libdeflate_compressor)
		libdeflate_free_compressor(ctx->libdeflate_compressor);
#endif

#if HAVE_LZ4
	if (ctx->lz4_stream)
		LZ4_freeStream(ctx->lz4_stream);
	if (ctx->lz4hc_stream)
		LZ4_freeStreamHC(ctx->lz4hc_stream);
#endif

#if HAVE_LZMA
	if (ctx->lzma_stream) {
		lzma_end(ctx->lzma_stream);
		free(ctx->lzma_stream);
	}
#endif

#if HAVE_ZSTD
	if (ctx->zstd_compressor)
		ZSTD_freeCCtx(ctx->zstd_compressor);
#endif

	free(ctx);
}

enum clod_compression_result
clod_compress(struct clod_compressor *ctx,
	void *dst, const size_t dst_max_size,
	const void *src, const size_t src_size,
	size_t *actual_size,
	const enum clod_compression_algo algo,
	const enum clod_compression_level level
) {
	switch (algo) {
		case CLOD_UNCOMPRESSED: {
			if (dst_max_size < src_size) {
				*actual_size = src_size;
				return CLOD_COMPRESSION_SHORT_BUFFER;
			}
			memcpy(dst, src, src_size);
			*actual_size = src_size;
			return CLOD_COMPRESSION_SUCCESS;
		}
		case CLOD_GZIP: {
			#if HAVE_LIBDEFLATE
				const int libdeflate_level =
					level == CLOD_COMPRESSION_HIGHEST ? 12 :
					level == CLOD_COMPRESSION_HIGH ? 9 :
					level == CLOD_COMPRESSION_NORMAL ? 6 :
					level == CLOD_COMPRESSION_LOW ? 3 :
					level == CLOD_COMPRESSION_LOWEST ? 1 :
					6;

				if (ctx->libdeflate_compressor && ctx->libdeflate_compressor_level != libdeflate_level) {
					libdeflate_free_compressor(ctx->libdeflate_compressor);
					ctx->libdeflate_compressor = nullptr;
				}
				if (!ctx->libdeflate_compressor) {
					ctx->libdeflate_compressor = libdeflate_alloc_compressor(libdeflate_level);
					if (!ctx->libdeflate_compressor) {
						return CLOD_COMPRESSION_ALLOC_FAILED;
					}
					ctx->libdeflate_compressor_level = libdeflate_level;
				}

				const size_t size = libdeflate_gzip_compress(ctx->libdeflate_compressor,
					src, src_size,
					dst, dst_max_size);

				*actual_size = size;
				if (size == 0) return CLOD_COMPRESSION_SHORT_BUFFER;
				return CLOD_COMPRESSION_SUCCESS;
			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif
		}
		case CLOD_ZLIB: {
			#if HAVE_LIBDEFLATE
				const int libdeflate_level =
					level == CLOD_COMPRESSION_HIGHEST ? 12 :
					level == CLOD_COMPRESSION_HIGH ? 9 :
					level == CLOD_COMPRESSION_NORMAL ? 6 :
					level == CLOD_COMPRESSION_LOW ? 3 :
					level == CLOD_COMPRESSION_LOWEST ? 1 :
					6;

				if (ctx->libdeflate_compressor && ctx->libdeflate_compressor_level != libdeflate_level) {
					libdeflate_free_compressor(ctx->libdeflate_compressor);
					ctx->libdeflate_compressor = nullptr;
				}
				if (!ctx->libdeflate_compressor) {
					ctx->libdeflate_compressor = libdeflate_alloc_compressor(libdeflate_level);
					if (!ctx->libdeflate_compressor) {
						return CLOD_COMPRESSION_ALLOC_FAILED;
					}
					ctx->libdeflate_compressor_level = libdeflate_level;
				}

				const size_t size = libdeflate_zlib_compress(ctx->libdeflate_compressor,
					src, src_size,
					dst, dst_max_size);

				*actual_size = size;
				if (size == 0) return CLOD_COMPRESSION_SHORT_BUFFER;
				return CLOD_COMPRESSION_SUCCESS;
			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif
		}
		case CLOD_DEFLATE: {
			#if HAVE_LIBDEFLATE
				const int libdeflate_level =
					level == CLOD_COMPRESSION_HIGHEST ? 12 :
					level == CLOD_COMPRESSION_HIGH ? 9 :
					level == CLOD_COMPRESSION_NORMAL ? 6 :
					level == CLOD_COMPRESSION_LOW ? 3 :
					level == CLOD_COMPRESSION_LOWEST ? 1 :
					6;

				if (ctx->libdeflate_compressor && ctx->libdeflate_compressor_level != libdeflate_level) {
					libdeflate_free_compressor(ctx->libdeflate_compressor);
					ctx->libdeflate_compressor = nullptr;
				}

				if (!ctx->libdeflate_compressor) {
					ctx->libdeflate_compressor = libdeflate_alloc_compressor(libdeflate_level);
					if (!ctx->libdeflate_compressor) {
						return CLOD_COMPRESSION_ALLOC_FAILED;
					}
					ctx->libdeflate_compressor_level = libdeflate_level;
				}

				const size_t size = libdeflate_deflate_compress(ctx->libdeflate_compressor,
					src, src_size,
					dst, dst_max_size);

				*actual_size = size;
				if (size == 0) return CLOD_COMPRESSION_SHORT_BUFFER;
				return CLOD_COMPRESSION_SUCCESS;
			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif
		}
		case CLOD_LZ4: {
			#if HAVE_LZ4

				LZ4F_preferences_t prefs = LZ4F_INIT_PREFERENCES;
				prefs.frameInfo.contentChecksumFlag = 1;
				prefs.frameInfo.contentSize = src_size;
				prefs.compressionLevel =
					level == CLOD_COMPRESSION_HIGHEST ? LZ4_ :
					level == CLOD_COMPRESSION_HIGH ? 10 :
					level == CLOD_COMPRESSION_NORMAL ? 6 :
					;



				LZ4F_compressFrame()


			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif
		}
		case CLOD_LZMA: {
			#if HAVE_LZMA

				return CLOD_COMPRESSION_UNSUPPORTED;

			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif
		}
		case CLOD_ZSTD: {
			return CLOD_COMPRESSION_UNSUPPORTED;
		}
		default: {
			return CLOD_COMPRESSION_UNSUPPORTED;
		}
	}
}
