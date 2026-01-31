#include <stdint.h>

#include "../test.h"
#include <stdlib.h>
#include <string.h>
#include <clod/compression.h>

const uint8_t data[] = {
#embed "fractal_Mandelbrot_3840x2160_644.png"
};

void compression_test_level(struct clod_compressor *compressor, struct clod_decompressor *decompressor, enum clod_compression_method method, enum clod_compression_level level) {
	constexpr size_t data_size = sizeof(data);
	char *cmp_data = malloc(data_size * 2);
	size_t sizeof_cmp_data = data_size * 2;
	char *dec_data = malloc(data_size + 1024);
	size_t sizeof_dec_data = data_size + 1024;

	enum clod_compression_result res;
	size_t cmp_size;
	size_t dec_size;

	switch (method) {
		case CLOD_UNCOMPRESSED: printf("Testing compression method CLOD_UNCOMPRESSED level %d\n", level); break;
		case CLOD_GZIP: printf("Testing compression method CLOD_GZIP level %d\n", level); break;
		case CLOD_ZLIB: printf("Testing compression method CLOD_ZLIB level %d\n", level); break;
		case CLOD_DEFLATE: printf("Testing compression method CLOD_DEFLATE level %d\n", level); break;
		case CLOD_LZ4F: printf("Testing compression method CLOD_LZ4F level %d\n", level); break;
		case CLOD_XZ: printf("Testing compression method CLOD_XZ level %d\n", level); break;
		case CLOD_ZSTD: printf("Testing compression method CLOD_ZSTD level %d\n", level); break;
		case CLOD_BZIP2: printf("Testing compression method CLOD_BZIP2 level %d\n", level); break;
		case CLOD_MINECRAFT_LZ4: printf("Testing compression method CLOD_MINECRAFT_LZ4 level %d\n", level); break;
		default: printf("Compression method %d name hasn't been added to the test's logging\n", method); break;
	}

	// Zero size payload

	cmp_size = SIZE_MAX;
	res = clod_compress(compressor, cmp_data, sizeof_cmp_data, data, 0, &cmp_size, method, level);
	check("Compressing zero-size data returns success", res == CLOD_COMPRESSION_SUCCESS);
	check("Compressing zero-size data returns a compressed size", cmp_size != SIZE_MAX);

	strcpy(dec_data, "garbage");
	res = clod_decompress(decompressor, dec_data, 0, cmp_data, cmp_size, nullptr, method);
	check("Decompressing zero-size data with provided compressed and uncompressed size returns success", res == CLOD_COMPRESSION_SUCCESS);
	check("Decompressing zero-size data with provided compressed and uncompressed size does not modify dst", memcmp(dec_data, "garbage", strlen("garbage")) == 0);

	strcpy(dec_data, "garbage");
	res = clod_decompress(decompressor, dec_data, 0, cmp_data, sizeof_cmp_data, nullptr, method);
	check("Decompressing zero-size data with provided uncompressed size returns success", res == CLOD_COMPRESSION_SUCCESS);
	check("Decompressing zero-size data with provided uncompressed size does not modify dst", memcmp(dec_data, "garbage", strlen("garbage")) == 0);

	dec_size = SIZE_MAX;
	res = clod_decompress(decompressor, dec_data, sizeof_dec_data, cmp_data, cmp_size, &dec_size, method);
	check("Decompressing zero-size data with provided compressed size returns success", res == CLOD_COMPRESSION_SUCCESS);
	check("Decompressing zero-size data with provided compressed size returns actual uncompressed size", dec_size == 0);

	dec_size = SIZE_MAX;
	res = clod_decompress(decompressor, dec_data, sizeof_dec_data, cmp_data, sizeof_cmp_data, &dec_size, method);
	check("Decompressing zero-size data returns success", res == CLOD_COMPRESSION_SUCCESS);
	check("Decompressing zero-size data returns actual uncompressed size", dec_size == 0);

	// Correct zero-size payload errors

	strcpy(dec_data, "garbage");
	res = clod_decompress(decompressor, dec_data, 1, cmp_data, cmp_size, nullptr, method);
	check("Decompressing zero-size data with provided compressed size and one-too-big uncompressed size returns short output", res == CLOD_COMPRESSION_SHORT_OUTPUT);
	check("Decompressing zero-size data with provided compressed size and one-too-big uncompressed size does not modify dst past end", memcmp(dec_data + 1, "arbage", strlen("arbage")) == 0);

	strcpy(dec_data, "garbage");
	res = clod_decompress(decompressor, dec_data, 1, cmp_data, sizeof_cmp_data, nullptr, method);
	check("Decompressing zero-size data with one-too-big uncompressed size returns short output", res == CLOD_COMPRESSION_SHORT_OUTPUT);
	check("Decompressing zero-size data with one-too-big uncompressed size does not modify dst past end", memcmp(dec_data + 1, "arbage", strlen("arbage")) == 0);

	// Full size payload

	cmp_size = SIZE_MAX;
	strcpy(cmp_data, "garbage");
	res = clod_compress(compressor, cmp_data, sizeof_cmp_data, data, data_size, &cmp_size, method, level);
	check("Compressing data returns success", res == CLOD_COMPRESSION_SUCCESS);
	check("Compressing data returns a compressed size", cmp_size != SIZE_MAX);
	check("Compressing data wrote something to dst", memcmp(cmp_data, "garbage", strlen("garbage")) != 0);

	strcpy(dec_data, "garbage");
	strcpy(dec_data + data_size, "garbage");
	res = clod_decompress(decompressor, dec_data, data_size, cmp_data, cmp_size, nullptr, method);
	check("Decompressing data with provided compressed and uncompressed size returns success", res == CLOD_COMPRESSION_SUCCESS);
	check("Decompressing data with provided compressed and uncompressed size matches original data", memcmp(dec_data, data, data_size) == 0);
	check("Decompressing data with provided compressed and uncompressed size does not modify dst past end", memcmp(dec_data + data_size, "garbage", strlen("garbage")) == 0);

	strcpy(dec_data, "garbage");
	strcpy(dec_data + data_size, "garbage");
	res = clod_decompress(decompressor, dec_data, data_size, cmp_data, sizeof_cmp_data, nullptr, method);
	check("Decompressing data with provided uncompressed size returns success", res == CLOD_COMPRESSION_SUCCESS);
	check("Decompressing data with provided uncompressed size matches original data", memcmp(dec_data, data, data_size) == 0);
	check("Decompressing data with provided uncompressed size does not modify dst past end", memcmp(dec_data + data_size, "garbage", strlen("garbage")) == 0);

	strcpy(dec_data, "garbage");
	res = clod_decompress(decompressor, dec_data, sizeof_dec_data, cmp_data, cmp_size, &dec_size, method);
	check("Decompressing data with provided compressed size returns success", res == CLOD_COMPRESSION_SUCCESS);
	check("Decompressing data with provided compressed size returns actual uncompressed size", dec_size == data_size);
	check("Decompressing data with provided compressed size matches original data", memcmp(dec_data, data, data_size) == 0);

	strcpy(dec_data, "garbage");
	res = clod_decompress(decompressor, dec_data, sizeof_dec_data, cmp_data, sizeof_cmp_data, &dec_size, method);
	check("Decompressing data returns success", res == CLOD_COMPRESSION_SUCCESS);
	check("Decompressing data returns actual size", dec_size == data_size);
	check("Decompressing data matches original", memcmp(dec_data, data, data_size) == 0);

	// Correct errors

	strcpy(dec_data + data_size - 1, "garbage");
	res = clod_decompress(decompressor, dec_data, data_size - 1, cmp_data, cmp_size, nullptr, method);
	check("Decompressing with provided compressed size and one-too-small decompressed size returns short buffer", res == CLOD_COMPRESSION_SHORT_BUFFER);
	check("Decompressing with provided compressed size and one-too-small decompressed size does not modify dst past end", memcmp(dec_data + data_size - 1, "garbage", strlen("garbage")) == 0);

	strcpy(dec_data + data_size - 1, "garbage");
	res = clod_decompress(decompressor, dec_data, data_size - 1, cmp_data, sizeof_cmp_data, nullptr, method);
	check("Decompressing with one-too-small decompressed size returns short buffer", res == CLOD_COMPRESSION_SHORT_BUFFER);
	check("Decompressing with one-too-small decompressed size does not modify dst past end", memcmp(dec_data + data_size - 1, "garbage", strlen("garbage")) == 0);

	dec_size = SIZE_MAX;
	strcpy(dec_data + data_size - 1, "garbage");
	res = clod_decompress(decompressor, dec_data, data_size - 1, cmp_data, cmp_size, &dec_size, method);
	check("Decompressing with provided compressed size and one-too-small dst returns short buffer", res == CLOD_COMPRESSION_SHORT_BUFFER);
	check("Decompressing with provided compressed size and one-too-small dst returns actual uncompressed size or 0", dec_size == 0 || dec_size == data_size);
	check("Decompressing with provided compressed size and one-too-small dst does not modify dst past end", memcmp(dec_data + data_size - 1, "garbage", strlen("garbage")) == 0);

	dec_size = SIZE_MAX;
	strcpy(dec_data + data_size - 1, "garbage");
	res = clod_decompress(decompressor, dec_data, data_size - 1, cmp_data, sizeof_cmp_data, &dec_size, method);
	check("Decompressing with one-too-small dst returns short buffer", res == CLOD_COMPRESSION_SHORT_BUFFER);
	check("Decompressing with one-too-small dst returns actual uncompressed size or 0", dec_size == 0 || dec_size == data_size);
	check("Decompressing with one-too-small dst does not modify dst past end", memcmp(dec_data + data_size - 1, "garbage", strlen("garbage")) == 0);

	strcpy(dec_data + data_size + 1, "garbage");
	res = clod_decompress(decompressor, dec_data, data_size + 1, cmp_data, cmp_size, nullptr, method);
	check("Decompressing with provided compressed size and one-too-big decompressed size returns short output", res == CLOD_COMPRESSION_SHORT_OUTPUT);
	check("Decompressing with provided compressed size and one-too-big decompressed size does not modify dst past end", memcmp(dec_data + data_size + 1, "garbage", strlen("garbage")) == 0);

	strcpy(dec_data + data_size + 1, "garbage");
	res = clod_decompress(decompressor, dec_data, data_size + 1, cmp_data, sizeof_cmp_data, nullptr, method);
	check("Decompressing with one-too-big decompressed size returns short output", res == CLOD_COMPRESSION_SHORT_OUTPUT);
	check("Decompressing with one-too-big decompressed size does not modify dst past end", memcmp(dec_data + data_size + 1, "garbage", strlen("garbage")) == 0);

	size_t dummy;
	strcpy(cmp_data, "garbage");
	res = clod_compress(compressor, cmp_data, 0, data, data_size, &dummy, method, level);
	check("Compressing data with zero-size dst returns short buffer", res == CLOD_COMPRESSION_SHORT_BUFFER);
	check("Compressing data with zero-size dst does not modify dst", memcmp(cmp_data, "garbage", strlen("garbage")) == 0);

	strcpy(cmp_data + cmp_size - 1, "garbage");
	res = clod_compress(compressor, cmp_data, cmp_size - 1, data, data_size, &dummy, method, level);
	check("Compressing data with one-too-small dst returns short buffer", res == CLOD_COMPRESSION_SHORT_BUFFER);
	check("Compressing data with one-too-small dst does not modify dst past end", memcmp(cmp_data + cmp_size - 1, "garbage", strlen("garbage")) == 0);

	free(cmp_data);
	free(dec_data);
}

void compression_test(struct clod_compressor *compressor, struct clod_decompressor *decompressor, enum clod_compression_method method) {
	for (enum clod_compression_level level = 0; level < CLOD_COMPRESSION_LEVELS; level++)
		compression_test_level(compressor, decompressor, method, level);
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

	if (clod_compression_support(CLOD_LZ4F)) {
		compression_test(compressor, decompressor, CLOD_LZ4F);
	}

	if (clod_compression_support(CLOD_XZ)) {
		compression_test(compressor, decompressor, CLOD_XZ);
	}

	if (clod_compression_support(CLOD_ZSTD)) {
		compression_test(compressor, decompressor, CLOD_ZSTD);
	}

	if (clod_compression_support(CLOD_BZIP2)) {
		compression_test(compressor, decompressor, CLOD_BZIP2);
	}

	if (clod_compression_support(CLOD_MINECRAFT_LZ4)) {
		//compression_test(compressor, decompressor, CLOD_MINECRAFT_LZ4);
	}

	clod_compressor_free(compressor);
	clod_decompressor_free(decompressor);
}
