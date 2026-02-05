#include "header.h"
#include <clod/nbt.h>
#include <clod/vmath.h>
#include <clod/hash.h>
#include <stdatomic.h>
#include <time.h>

#include "clod/region.h"

#define HEADER_LIBCLOD_MIN 256
#define HEADER_VANILLA_SIZE 8192
#define HEADER_COMPOUND_MIN (8192 + 256)

#define LIBCLOD_MAGIC \
	"\n\n"\
	"libclod extended region file format version 1.\n"\
	"See github.com/stewi1014/clod for format details.\n"\
	"\n\n"

enum clod_rhdr_type clod_rhdr_type(const char *file, size_t file_size) {
	if (file_size >= HEADER_LIBCLOD_MIN && memcmp(file, LIBCLOD_MAGIC, strlen(LIBCLOD_MAGIC)) == 0)
		return CLOD_RHDR_LIBCLOD;

	if (file_size >= HEADER_COMPOUND_MIN && memcmp(file + HEADER_VANILLA_SIZE, LIBCLOD_MAGIC, strlen(LIBCLOD_MAGIC)) == 0)
		return CLOD_RHDR_COMPOUND;

	if (file_size >= HEADER_VANILLA_SIZE)
		return CLOD_RHDR_VANILLA;

	return CLOD_RHDR_UNKNOWN;
}
bool validate_nbt(struct clod_rhdr *header) {
	uint32_t checksum;
	uint32_t nbt_size;
	char *nbt_data, *nbt_end;

	switch (header->type) {
		case CLOD_RHDR_LIBCLOD:
			checksum = beu32_dec(header->file + 128);
			nbt_size = beu32_dec(header->file + 132);
			nbt_data = header->file + HEADER_LIBCLOD_MIN;
			nbt_end = header->file + HEADER_LIBCLOD_MIN + nbt_size;

			if (HEADER_LIBCLOD_MIN + nbt_size > header->file_size) return false;
			break;
		case CLOD_RHDR_COMPOUND:
			checksum = beu32_dec(header->file + HEADER_VANILLA_SIZE + 128);
			nbt_size = beu32_dec(header->file + HEADER_VANILLA_SIZE + 132);
			nbt_data = header->file + HEADER_COMPOUND_MIN;
			nbt_end = header->file + HEADER_COMPOUND_MIN + nbt_size;

			if (HEADER_COMPOUND_MIN + nbt_size > header->file_size) return false;
			break;
		default:
			return false;
	}

	if (checksum && clod_crc32(nbt_data, nbt_size) != checksum) return false;
	if (clod_nbt_tag_size(nbt_data, nbt_end) != nbt_size) return false;
	return false;
}
enum clod_rhdr_result clod_rhdr_init(
	struct clod_rhdr *header,
	char *file,
	size_t file_size
) {
	header->file = file;
	header->file_size = file_size;
	header->type = clod_rhdr_type(file, file_size);
	header->generation = clod_rhdr_generation(header);

	if (header->type != CLOD_RHDR_LIBCLOD && header->type != CLOD_RHDR_COMPOUND)
		return CLOD_RHDR_OK;

	if (header->generation & 1) return CLOD_RHDR_WRITE_INTERRUPT;
	char *libclod_header = header->type == CLOD_RHDR_LIBCLOD ? file : file + HEADER_VANILLA_SIZE;

	const uint32_t checksum = beu32_dec(libclod_header + 128);
	const uint32_t nbt_size = beu32_dec(libclod_header + 132);

	if (nbt_size + )

}
uint32_t clod_rhdr_generation(const struct clod_rhdr *header) {
	uint32_t *gen_ptr;
	if (header->type == CLOD_RHDR_LIBCLOD)
		gen_ptr = (uint32_t*)(header->file + 136);
	else if (header->type == CLOD_RHDR_COMPOUND)
		gen_ptr = (uint32_t*)(header->file + HEADER_VANILLA_SIZE + 136);
	else
		return 0;

	uint32_t gen = atomic_load(gen_ptr);
	return beu32_dec((char*)&gen);
}
enum clod_rhdr_result clod_rhdr_generation_wstart(struct clod_rhdr *header) {
	uint32_t *gen_ptr;
	if (header->type == CLOD_RHDR_LIBCLOD)
		gen_ptr = (uint32_t*)(header->file + 136);
	else if (header->type == CLOD_RHDR_COMPOUND)
		gen_ptr = (uint32_t*)(header->file + HEADER_VANILLA_SIZE + 136);
	else
		return CLOD_RHDR_OK;

	if (header->generation & 1) return CLOD_RHDR_WRITE_INTERRUPT;

	uint32_t expected;
	beu32_enc((char*)&expected, header->generation);
	uint32_t desired;
	beu32_enc((char*)&desired, header->generation + 1);

	if (!atomic_compare_exchange_strong(gen_ptr, &expected, desired)) {
		return CLOD_RHDR_WRITE_INTERRUPT;
	}

	header->generation = header->generation + 1;
	return CLOD_RHDR_OK;
}
enum clod_rhdr_result clod_rhdr_generation_wend(struct clod_rhdr *header) {
	uint32_t *gen_ptr;
	if (header->type == CLOD_RHDR_LIBCLOD)
		gen_ptr = (uint32_t*)(header->file + 136);
	else if (header->type == CLOD_RHDR_COMPOUND)
		gen_ptr = (uint32_t*)(header->file + HEADER_VANILLA_SIZE + 136);
	else
		return CLOD_RHDR_OK;

	if ((header->generation & 1) == 0) return CLOD_RHDR_NOT_WRITING;

	uint32_t expected;
	beu32_enc((char*)&expected, header->generation);
	uint32_t desired;
	beu32_enc((char*)&desired, header->generation + 1);

	if (!atomic_compare_exchange_strong(gen_ptr, &expected, desired)) {
		return CLOD_RHDR_WRITE_INTERRUPT;
	}

	header->generation = header->generation + 1;
	return CLOD_RHDR_OK;
}
