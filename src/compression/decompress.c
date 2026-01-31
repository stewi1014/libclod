#include "compression_config.h"
#include <clod/compression.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_LIBDEFLATE
#include <libdeflate.h>
#endif

#if HAVE_LIBLZ4
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

struct clod_decompressor {
	void *(*malloc_func)(size_t);
	void (*free_func)(void *);

#if HAVE_LIBDEFLATE
	struct libdeflate_decompressor *libdeflate_decompressor;
#endif

#if HAVE_LIBLZ4
	LZ4F_dctx *lz4_ctx;
#endif

#if HAVE_LIBZSTD
	ZSTD_DCtx *zstd_dctx;
#endif
};

struct clod_decompressor *clod_decompressor_init() {
	struct clod_decompressor *ctx = malloc(sizeof(struct clod_decompressor));
	memset(ctx, 0,sizeof(*ctx));
	// LZ4 and ZSTD are missing custom memory allocation methods.
	ctx->malloc_func = malloc;
	ctx->free_func = free;
	return ctx;
}

void clod_decompressor_free(struct clod_decompressor *ctx) {
#if HAVE_LIBDEFLATE
	if (ctx->libdeflate_decompressor)
		libdeflate_free_decompressor(ctx->libdeflate_decompressor);
#endif

#if HAVE_LIBLZ4
	if (ctx->lz4_ctx)
		LZ4F_freeDecompressionContext(ctx->lz4_ctx);
#endif

#if HAVE_LIBZSTD
	if (ctx->zstd_dctx)
		ZSTD_freeDCtx(ctx->zstd_dctx);
#endif

	free(ctx);
}

#if HAVE_LIBLZMA
void *decompressor_lzma_malloc(void *user, size_t n, size_t size) {
	auto const ctx = (struct clod_decompressor *)user;
	return ctx->malloc_func(n * size);
}
void decompressor_lzma_free(void *user, void *address) {
	auto const ctx = (struct clod_decompressor *)user;
	ctx->free_func(address);
}
#endif

#if HAVE_LIBBZ2
void *decompressor_bz2_malloc(void *user, int n, int size) {
	auto const ctx = (struct clod_decompressor *)user;
	return ctx->malloc_func((size_t)n * (size_t)size);
}
void decompressor_bz2_free(void *user, void *address) {
	auto const ctx = (struct clod_decompressor *)user;
	ctx->free_func(address);
}
#endif


enum clod_compression_result
clod_decompress(struct clod_decompressor *ctx,
	void *const dst, const size_t dst_size,
	const void *const src, const size_t src_size,
	size_t *const actual_size,
	const enum clod_compression_method method
) {
	switch (method) {
		case CLOD_UNCOMPRESSED: {
			if (dst_size < src_size) {
				if (actual_size) *actual_size = src_size;
				return CLOD_COMPRESSION_SHORT_BUFFER;
			}
			if (!actual_size && dst_size != src_size) return CLOD_COMPRESSION_SHORT_OUTPUT;
			memcpy(dst, src, src_size);
			if (actual_size) *actual_size = src_size;
			return CLOD_COMPRESSION_SUCCESS;
		}
		case CLOD_GZIP: {
			#if HAVE_LIBDEFLATE
				if (!ctx->libdeflate_decompressor) {
					struct libdeflate_options opts = {0};
					opts.sizeof_options = sizeof(opts);
					opts.malloc_func = ctx->malloc_func;
					opts.free_func = ctx->free_func;

					ctx->libdeflate_decompressor = libdeflate_alloc_decompressor_ex(&opts);
					if (!ctx->libdeflate_decompressor) {
						return CLOD_COMPRESSION_ALLOC_FAILED;
					}
				}

				const enum libdeflate_result res = libdeflate_gzip_decompress(ctx->libdeflate_decompressor,
					src, src_size,
					dst, dst_size,
					actual_size
				);

				if (res == LIBDEFLATE_INSUFFICIENT_SPACE) {
					if (actual_size) *actual_size = 0;
					return CLOD_COMPRESSION_SHORT_BUFFER;
				}

				switch (res) {
					case LIBDEFLATE_SUCCESS: return CLOD_COMPRESSION_SUCCESS;
					case LIBDEFLATE_BAD_DATA: return CLOD_COMPRESSION_MALFORMED;
					case LIBDEFLATE_SHORT_OUTPUT: return CLOD_COMPRESSION_SHORT_OUTPUT;
					default: return CLOD_COMPRESSION_UNSUPPORTED;
				}
			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif
		}
		case CLOD_ZLIB: {
			#if HAVE_LIBDEFLATE
				if (!ctx->libdeflate_decompressor) {
					struct libdeflate_options opts = {0};
					opts.sizeof_options = sizeof(opts);
					opts.malloc_func = ctx->malloc_func;
					opts.free_func = ctx->free_func;

					ctx->libdeflate_decompressor = libdeflate_alloc_decompressor_ex(&opts);
					if (!ctx->libdeflate_decompressor) {
						return CLOD_COMPRESSION_ALLOC_FAILED;
					}
				}

				const enum libdeflate_result res = libdeflate_zlib_decompress(ctx->libdeflate_decompressor,
					src, src_size,
					dst, dst_size,
					actual_size
				);

				if (res == LIBDEFLATE_INSUFFICIENT_SPACE) {
					if (actual_size) *actual_size = 0;
					return CLOD_COMPRESSION_SHORT_BUFFER;
				}

				switch (res) {
					case LIBDEFLATE_SUCCESS: return CLOD_COMPRESSION_SUCCESS;
					case LIBDEFLATE_BAD_DATA: return CLOD_COMPRESSION_MALFORMED;
					case LIBDEFLATE_SHORT_OUTPUT: return CLOD_COMPRESSION_SHORT_OUTPUT;
					default: return CLOD_COMPRESSION_UNSUPPORTED;
				}
			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif
		}
		case CLOD_DEFLATE: {
			#if HAVE_LIBDEFLATE
				if (!ctx->libdeflate_decompressor) {
					struct libdeflate_options opts = {0};
					opts.sizeof_options = sizeof(opts);
					opts.malloc_func = ctx->malloc_func;
					opts.free_func = ctx->free_func;

					ctx->libdeflate_decompressor = libdeflate_alloc_decompressor_ex(&opts);
					if (!ctx->libdeflate_decompressor) {
						return CLOD_COMPRESSION_ALLOC_FAILED;
					}
				}

				const enum libdeflate_result res = libdeflate_deflate_decompress(ctx->libdeflate_decompressor,
					src, src_size,
					dst, dst_size,
					actual_size
				);

				if (res == LIBDEFLATE_INSUFFICIENT_SPACE) {
					if (actual_size) *actual_size = 0;
					return CLOD_COMPRESSION_SHORT_BUFFER;
				}

				switch (res) {
					case LIBDEFLATE_SUCCESS: return CLOD_COMPRESSION_SUCCESS;
					case LIBDEFLATE_BAD_DATA: return CLOD_COMPRESSION_MALFORMED;
					case LIBDEFLATE_SHORT_OUTPUT: return CLOD_COMPRESSION_SHORT_OUTPUT;
					default: return CLOD_COMPRESSION_UNSUPPORTED;
				}
			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif
		}
		case CLOD_LZ4F: {
			#if HAVE_LIBLZ4
				if (src_size < LZ4F_MIN_SIZE_TO_KNOW_HEADER_LENGTH) {
					return CLOD_COMPRESSION_MALFORMED;
				}

				const size_t header_size = LZ4F_headerSize(src, src_size);
				if (LZ4F_isError(header_size) || header_size > src_size) {
					return CLOD_COMPRESSION_MALFORMED;
				}

				if (!ctx->lz4_ctx) {
					const LZ4F_errorCode_t err = LZ4F_createDecompressionContext(&ctx->lz4_ctx, LZ4F_VERSION);
					if (LZ4F_isError(err) || !ctx->lz4_ctx) {
						return CLOD_COMPRESSION_ALLOC_FAILED;
					}
				}

				size_t src_offset = 0, dst_offset = 0;

				LZ4F_frameInfo_t info;
				src_offset = src_size;
				size_t res = LZ4F_getFrameInfo(ctx->lz4_ctx, &info, src, &src_offset);

				if (LZ4F_isError(res)) {
					LZ4F_resetDecompressionContext(ctx->lz4_ctx);
					return CLOD_COMPRESSION_MALFORMED;
				}

				if (info.contentSize > dst_size) {
					LZ4F_resetDecompressionContext(ctx->lz4_ctx);
					if (actual_size) *actual_size = info.contentSize;
					return CLOD_COMPRESSION_SHORT_BUFFER;
				}

				if (!actual_size && info.contentSize > 0 && info.contentSize < dst_size) {
					LZ4F_resetDecompressionContext(ctx->lz4_ctx);
					return CLOD_COMPRESSION_SHORT_OUTPUT;
				}

				while (true) {
					size_t dst_chunk_size = dst_size - dst_offset;
					size_t src_chunk_size = src_size - src_offset;

					res = LZ4F_decompress(ctx->lz4_ctx,
						(char*)dst + dst_offset, &dst_chunk_size,
						(const char*)src + src_offset, &src_chunk_size,
						nullptr);

					if (LZ4F_isError(res)) {
						LZ4F_resetDecompressionContext(ctx->lz4_ctx);
						return CLOD_COMPRESSION_MALFORMED;
					}

					dst_offset += dst_chunk_size;
					src_offset += src_chunk_size;

					if (res == 0) {
						LZ4F_resetDecompressionContext(ctx->lz4_ctx);
						if (!actual_size && dst_offset < dst_size) return CLOD_COMPRESSION_SHORT_OUTPUT;
						if (actual_size) *actual_size = dst_offset;
						return CLOD_COMPRESSION_SUCCESS;
					}

					if (src_offset >= src_size) {
						LZ4F_resetDecompressionContext(ctx->lz4_ctx);
						return CLOD_COMPRESSION_MALFORMED;
					}

					if (dst_offset >= dst_size) {
						LZ4F_resetDecompressionContext(ctx->lz4_ctx);
						if (actual_size) *actual_size = info.contentSize > 0 ? info.contentSize : 0;
						return CLOD_COMPRESSION_SHORT_BUFFER;
					}
				}

			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif

		}
		case CLOD_XZ: {
			#if HAVE_LIBLZMA
				const lzma_allocator allocator = {
					.alloc = decompressor_lzma_malloc,
					.free = decompressor_lzma_free,
					.opaque = ctx,
				};

				lzma_stream stream = LZMA_STREAM_INIT;
				stream.allocator = &allocator;

				lzma_ret ret = lzma_stream_decoder(&stream, UINT64_MAX, 0);
				if (ret != LZMA_OK) {
					return ret == LZMA_MEM_ERROR ? CLOD_COMPRESSION_ALLOC_FAILED : CLOD_COMPRESSION_UNSUPPORTED;
				}

				stream.next_in = (uint8_t *)src;
				stream.avail_in = src_size;
				stream.next_out = (uint8_t *)dst;
				stream.avail_out = dst_size;

				do {
					ret = lzma_code(&stream, LZMA_FINISH);
				} while (ret == LZMA_OK);

				lzma_end(&stream);

				switch (ret) {
					case LZMA_STREAM_END:
						if (!actual_size && stream.avail_out != 0) return CLOD_COMPRESSION_SHORT_OUTPUT;
						if (actual_size) *actual_size = stream.total_out;
						return CLOD_COMPRESSION_SUCCESS;
					case LZMA_BUF_ERROR:
						if (stream.avail_out == 0) {
							if (actual_size) *actual_size = 0;
							return CLOD_COMPRESSION_SHORT_BUFFER;
						}
						return CLOD_COMPRESSION_MALFORMED;
					case LZMA_MEM_ERROR: case LZMA_MEMLIMIT_ERROR:
						return CLOD_COMPRESSION_ALLOC_FAILED;
					case LZMA_FORMAT_ERROR: case LZMA_OPTIONS_ERROR: case LZMA_DATA_ERROR:
						return CLOD_COMPRESSION_MALFORMED;
					default:
						return CLOD_COMPRESSION_UNSUPPORTED;
				}
			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif
		}
		case CLOD_ZSTD: {
			#if HAVE_LIBZSTD
				if (!ctx->zstd_dctx) {
					ctx->zstd_dctx = ZSTD_createDCtx();
					if (!ctx->zstd_dctx) return CLOD_COMPRESSION_ALLOC_FAILED;
				}

				ZSTD_inBuffer in = {src, src_size, 0};
				ZSTD_outBuffer out = {dst, dst_size, 0};

				size_t ret = 1;
				while (in.pos < in.size && ret != 0) {
					ret = ZSTD_decompressStream(ctx->zstd_dctx, &out, &in);
					if (ZSTD_isError(ret)) {
						ZSTD_DCtx_reset(ctx->zstd_dctx, ZSTD_reset_session_only);
						if (ZSTD_getErrorCode(ret) == ZSTD_error_noForwardProgress_destFull) {
							if (actual_size) {
								auto const frame_size = ZSTD_getFrameContentSize(src, src_size);
								if (frame_size == ZSTD_CONTENTSIZE_UNKNOWN || frame_size == ZSTD_CONTENTSIZE_ERROR)
									*actual_size = 0;
								else
									*actual_size = frame_size;
							}
							return CLOD_COMPRESSION_SHORT_BUFFER;
						}
						return CLOD_COMPRESSION_MALFORMED;
					}
				}

				if (ret == 0) {
					if (!actual_size && out.pos != out.size) return CLOD_COMPRESSION_SHORT_OUTPUT;
					if (actual_size) *actual_size = out.pos;
					return CLOD_COMPRESSION_SUCCESS;
				}

				return CLOD_COMPRESSION_MALFORMED;
			#else
				return CLOD_COMPRESSION_UNSUPPORTED;
			#endif
		}
		case CLOD_BZIP2: {
			#if HAVE_LIBBZ2
				// TODO: support sizes > UINT32_MAX

				bz_stream stream = {0};
				stream.next_in = (char *)src;
				stream.avail_in = (unsigned)src_size;
				stream.next_out = (char *)dst;
				stream.avail_out = (unsigned)dst_size;
				stream.bzalloc = decompressor_bz2_malloc;
				stream.bzfree = decompressor_bz2_free;
				stream.opaque = ctx;

				char dummy;
				if (dst_size == 0) {
					stream.next_out = &dummy;
					stream.avail_out = 1;
				}

				int res = BZ2_bzDecompressInit(&stream, 0, 0);
				if (res == BZ_MEM_ERROR) return CLOD_COMPRESSION_ALLOC_FAILED;
				if (res != BZ_OK) return CLOD_COMPRESSION_UNSUPPORTED;

				do {
					if (stream.avail_out == 0) {
						BZ2_bzDecompressEnd(&stream);
						if (actual_size) *actual_size = 0;
						return CLOD_COMPRESSION_SHORT_BUFFER;
					}
					res = BZ2_bzDecompress(&stream);
				} while (res == BZ_OK);

				BZ2_bzDecompressEnd(&stream);

				switch (res) {
					case BZ_STREAM_END:
						if (dst_size == 0) {
							if (stream.avail_out != 1) return CLOD_COMPRESSION_SHORT_BUFFER;
						} else {
							if (!actual_size && stream.avail_out != 0) return CLOD_COMPRESSION_SHORT_OUTPUT;
						}
						if (actual_size) *actual_size = (size_t)stream.total_out_lo32 + ((size_t)stream.total_out_hi32 << 32);
						return CLOD_COMPRESSION_SUCCESS;
					case BZ_MEM_ERROR: return CLOD_COMPRESSION_ALLOC_FAILED;
					case BZ_DATA_ERROR_MAGIC:
					case BZ_DATA_ERROR: return CLOD_COMPRESSION_MALFORMED;
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
