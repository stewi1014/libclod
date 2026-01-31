#include "compression_config.h"
#include <clod/compression.h>

bool clod_compression_support(enum clod_compression_method method) {
	if (method == CLOD_UNCOMPRESSED) return true;

	#if HAVE_LIBDEFLATE
		if (method == CLOD_DEFLATE || method == CLOD_GZIP || method == CLOD_ZLIB) return true;
	#endif

	#if HAVE_LIBLZ4
		if (method == CLOD_LZ4F || method == CLOD_MINECRAFT_LZ4) return true;
	#endif

	#if HAVE_LIBLZMA
		if (method == CLOD_XZ) return true;
	#endif

	#if HAVE_LIBZSTD
		if (method == CLOD_ZSTD) return true;
	#endif

	#if HAVE_LIBBZ2
		if (method == CLOD_BZIP2) return true;
	#endif

	return false;
}
