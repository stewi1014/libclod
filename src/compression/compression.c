#include "compression_config.h"
#include <clod/compression.h>

bool clod_compression_support(enum clod_compression_algo algo) {
#if HAVE_LIBDEFLATE
	if (algo == CLOD_DEFLATE || algo == CLOD_GZIP || algo == CLOD_ZLIB) return true;
#endif

#if HAVE_LZ4
	if (algo == CLOD_LZ4 || algo == CLOD_LZ4HC) return true;
#endif

#if HAVE_LZMA
	if (algo == CLOD_LZMA) return true;
#endif

#if HAVE_ZSTD
	if (algo == CLOD_ZSTD) return true;
#endif

	return false;
}
