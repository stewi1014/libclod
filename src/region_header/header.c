#include <assert.h>
#include <stdatomic.h>
#include <clod/region_header.h>
#include <clod/nbt.h>
#include <string.h>

#define LIBCLOD_MAGIC (char[]){"libclod 1"}
// The size of the magic that should be compared for equality.
constexpr size_t LIBCLOD_MAGIC_LEN = sizeof LIBCLOD_MAGIC;

enum clod_rhdr_type clod_rhdr_detect_type(const char *data, size_t data_size) {
	if (data_size >= sizeof LIBCLOD_MAGIC && memcmp(data, LIBCLOD_MAGIC, sizeof LIBCLOD_MAGIC) == 0)
		return CLOD_RHDR_LIBCLOD;
}

/*

#define HEADER_LIBCLOD_MIN 256
#define HEADER_VANILLA_SIZE 8192
#define HEADER_COMPOUND_MIN (8192 + 256)

#define LIBCLOD_MAGIC \
	"\n\n"\
	"libclod extended region file format version 1.\n"\
	"See github.com/stewi1014/clod for format details.\n"\
	"\n\n"

#define min(a, b) ((a) < (b) ? (a) : (b))

uint32_t clod_rfmt_generation(const struct clod_rhdr *rfmt) {
	uint32_t gen = 0;
	switch (rfmt->type) {
		case CLOD_RHDR_COMPOUND:
			gen = atomic_load((uint32_t*)(rfmt->data + HEADER_VANILLA_SIZE + 128));
			gen = beu32_dec((char*)&gen);
			break;
		case CLOD_RHDR_LIBCLOD:
			gen = atomic_load((uint32_t*)(rfmt->data + HEADER_VANILLA_SIZE + 128));
			gen = beu32_dec((char*)&gen);
			break;
		default:
			break;
	}
	return gen;
}

enum clod_rhdr_type clod_rhdr_detect_type(const char *data, size_t data_size) {
	if (data_size >= HEADER_LIBCLOD_MIN && memcmp(data, LIBCLOD_MAGIC, strlen(LIBCLOD_MAGIC)) == 0)
		return CLOD_RHDR_LIBCLOD;

	if (data_size >= HEADER_COMPOUND_MIN && memcmp(data + HEADER_VANILLA_SIZE, LIBCLOD_MAGIC, strlen(LIBCLOD_MAGIC)) == 0)
		return CLOD_RHDR_COMPOUND;

	if (data_size >= HEADER_VANILLA_SIZE)
		return CLOD_RHDR_VANILLA;

	return 0;
}
enum clod_rhdr_result clod_rhdr_init_new(struct clod_rhdr *rhdr, char *data, size_t data_size, enum clod_rhdr_type type) {
	switch (type) {
		case CLOD_RHDR_VANILLA:
			if (data_size < HEADER_VANILLA_SIZE)
				return CLOD_RHDR_MALFORMED;

			if (
				data_size >= HEADER_COMPOUND_MIN &&
				memcmp(data + HEADER_VANILLA_SIZE, LIBCLOD_MAGIC, strlen(LIBCLOD_MAGIC)) == 0
			) {
				// The header looks like a compound header.
				// Clear it.
				memset(data, 0, HEADER_COMPOUND_MIN);
			} else {
				memset(data, 0, HEADER_VANILLA_SIZE);
			}
			break;

		case CLOD_RHDR_COMPOUND:
			if (data_size < HEADER_VANILLA_SIZE + HEADER_LIBCLOD_MIN + sizeof(CLOD_NBT_ROOT_COMPOUND_INIT))
				return CLOD_RHDR_MALFORMED;

			memset(data, 0, HEADER_VANILLA_SIZE + HEADER_LIBCLOD_MIN);
			memcpy(data + HEADER_VANILLA_SIZE, LIBCLOD_MAGIC, strlen(LIBCLOD_MAGIC));
			memcpy(data + HEADER_VANILLA_SIZE + HEADER_LIBCLOD_MIN, CLOD_NBT_ROOT_COMPOUND_INIT, sizeof(CLOD_NBT_ROOT_COMPOUND_INIT));
			beu32_enc(data + HEADER_VANILLA_SIZE + 136, sizeof(CLOD_NBT_ROOT_COMPOUND_INIT));
			break;

		case CLOD_RHDR_LIBCLOD:
			if (data_size < HEADER_LIBCLOD_MIN + sizeof(CLOD_NBT_ROOT_COMPOUND_INIT))
				return CLOD_RHDR_MALFORMED;

			memset(data, 0, HEADER_LIBCLOD_MIN);
			memcpy(data, LIBCLOD_MAGIC, strlen(LIBCLOD_MAGIC));
			memcpy(data + HEADER_LIBCLOD_MIN, CLOD_NBT_ROOT_COMPOUND_INIT, sizeof(CLOD_NBT_ROOT_COMPOUND_INIT));
			break;

		default:
			return CLOD_RHDR_INVALID;
	}

	auto const res = clod_rhdr_init_write(rhdr, data, data_size);
	assert(res == CLOD_RHDR_OK); // This method should never produce an invalid header.
	return res;
}
enum clod_rhdr_result clod_rhdr_init_read(struct clod_rhdr *rhdr, char *data, size_t data_size) {
	rhdr->data = data;
	rhdr->data_size = data_size;
	rhdr->type = clod_rhdr_detect_type(data, data_size);
	rhdr->generation = clod_rfmt_generation(rhdr);

	switch (type) {
		case CLOD_RHDR_VANILLA:
			break;
		case CLOD_RHDR_COMPOUND:


		case CLOD_RHDR_LIBCLOD:
			return clod_rhdr_init_new(rhdr, data, data_size, type);
		default:
			return CLOD_RHDR_INVALID;
	}

	return CLOD_RHDR_OK;
}
enum clod_rhdr_result clod_rhdr_init_write(struct clod_rhdr *rhdr, char *data, size_t data_size) {

}
enum clod_rhdr_result clod_rhdr_done(struct clod_rhdr *rhdr) {

}
*/
