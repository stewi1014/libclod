#include "../test.h"
#include <stdlib.h>
#include <string.h>
#include <clod/compression.h>

const uint8_t data[] = {
#embed "fractal_Mandelbrot_3840x2160_644.png"
};

void compression_test(struct clod_compressor *compressor, struct clod_decompressor *decompressor, uint64_t algo) {
	constexpr size_t data_size = sizeof(data);

	size_t compressed_size;
	void *compressed = malloc(data_size);
	size_t decompressed_size;
	void *decompressed = malloc(data_size + 1);

	compressed_size = 0;
	auto res = clod_compress(compressor,
		compressed, data_size,
		data, 0,
		&compressed_size, algo, CLOD_COMPRESSION_NORMAL
	);
	check(res == CLOD_COMPRESSION_SUCCESS);
	check(compressed_size > 0);

	res = clod_decompress(decompressor,
		decompressed, 1,
		compressed, compressed_size,
		nullptr, algo);
	check(res == CLOD_COMPRESSION_SHORT_OUTPUT);

	res = clod_decompress(decompressor,
		decompressed, 0,
		compressed, compressed_size,
		nullptr, algo);
	check(res == CLOD_COMPRESSION_SUCCESS);

	decompressed_size = SIZE_MAX;
	res = clod_decompress(decompressor,
		decompressed, 0,
		compressed, compressed_size,
		&decompressed_size, algo);
	check(res == CLOD_COMPRESSION_SUCCESS);
	check(decompressed_size == 0);

	decompressed_size = SIZE_MAX;
	res = clod_decompress(decompressor,
		decompressed, data_size,
		compressed, compressed_size,
		&decompressed_size, algo);
	check(res == CLOD_COMPRESSION_SUCCESS);
	check(decompressed_size == 0);

	compressed_size = SIZE_MAX;
	res = clod_compress(compressor,
		compressed, data_size,
		data, data_size,
		&compressed_size, algo, CLOD_COMPRESSION_NORMAL);
	check(res == CLOD_COMPRESSION_SUCCESS);
	check(compressed_size != SIZE_MAX);

	res = clod_decompress(decompressor,
		decompressed, data_size + 1,
		compressed, compressed_size,
		nullptr, algo);
	check(res == CLOD_COMPRESSION_SHORT_OUTPUT);

	res = clod_decompress(decompressor,
		decompressed, data_size - 1,
		compressed, compressed_size,
		nullptr, algo);
	check(res == CLOD_COMPRESSION_SHORT_BUFFER);

	strcpy(decompressed, "garbage");
	res = clod_decompress(decompressor,
		decompressed, data_size,
		compressed, compressed_size,
		nullptr, algo);
	check(res == CLOD_COMPRESSION_SUCCESS);
	check(memcmp(decompressed, data, data_size) == 0);

	decompressed_size = SIZE_MAX;
	res = clod_decompress(decompressor,
		decompressed, data_size - 1,
		compressed, compressed_size,
		&decompressed_size, algo);
	check(res == CLOD_COMPRESSION_SHORT_BUFFER);
	check(decompressed_size == 0);

	strcpy(decompressed, "garbage");
	decompressed_size = SIZE_MAX;
	res = clod_decompress(decompressor,
		decompressed, data_size + 1,
		compressed, compressed_size,
		&decompressed_size, algo);
	check(res == CLOD_COMPRESSION_SUCCESS);
	check(decompressed_size == data_size);
	check(memcmp(decompressed, data, data_size) == 0);

	strcpy(decompressed, "garbage");
	decompressed_size = SIZE_MAX;
	res = clod_decompress(decompressor,
		decompressed, data_size,
		compressed, compressed_size,
		&decompressed_size, algo);
	check(res == CLOD_COMPRESSION_SUCCESS);
	check(decompressed_size == data_size);
	check(memcmp(decompressed, data, data_size) == 0);

	free(compressed);
	free(decompressed);
}

int main() {
	struct clod_compressor *compressor = clod_compressor_init();
	struct clod_decompressor *decompressor = clod_decompressor_init();

	if (clod_compression_support(CLOD_GZIP)) {
		compression_test(compressor, decompressor, CLOD_GZIP);
	}

	if (clod_compression_support(CLOD_ZLIB)) {
		compression_test(compressor, decompressor, CLOD_ZLIB);
	}

	if (clod_compression_support(CLOD_DEFLATE)) {
		compression_test(compressor, decompressor, CLOD_DEFLATE);
	}

	if (clod_compression_support(CLOD_LZ4)) {
		compression_test(compressor, decompressor, CLOD_LZ4);
	}

	if (clod_compression_support(CLOD_LZ4HC)) {
		compression_test(compressor, decompressor, CLOD_LZ4HC);
	}

	if (clod_compression_support(CLOD_LZMA)) {
		compression_test(compressor, decompressor, CLOD_LZMA);
	}

	if (clod_compression_support(CLOD_ZSTD)) {
		compression_test(compressor, decompressor, CLOD_ZSTD);
	}

	clod_compressor_free(compressor);
	clod_decompressor_free(decompressor);
}
