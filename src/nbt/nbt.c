#include <alloca.h>
#include <assert.h>
#include <string.h>
#include <clod/nbt.h>

constexpr size_t payload_zero_sizes[] = {
	[CLOD_NBT_INT8] = 1,
	[CLOD_NBT_INT16] = 2,
	[CLOD_NBT_INT32] = 4,
	[CLOD_NBT_INT64] = 8,
	[CLOD_NBT_FLOAT32] = 4,
	[CLOD_NBT_FLOAT64] = 8,
	[CLOD_NBT_INT8_ARRAY] = 4,
	[CLOD_NBT_INT32_ARRAY] = 4,
	[CLOD_NBT_INT64_ARRAY] = 4,
	[CLOD_NBT_STRING] = 2,
	[CLOD_NBT_LIST] = 5,
	[CLOD_NBT_COMPOUND] = 1
};
constexpr char payload_zero_sizes_len = sizeof(payload_zero_sizes) / sizeof(payload_zero_sizes[0]);

#define type_zero_size(type) (0 <= (type) && (type) < payload_zero_sizes_len ? payload_zero_sizes[(unsigned)type] : 0)
#define type_valid(type) (type_zero_size(type))
#define available(ptr, end) ((ptr) != nullptr && (ptr) <= (char*)(end) ? (size_t)((char*)(end) - (ptr)) : 0)

size_t clod_nbt_payload_size(
	const char *restrict const payload,
	const void *const end,
	const char payload_type
) {
	switch (payload_type) {
		default: return 0;
		case CLOD_NBT_INT8: return 1;
		case CLOD_NBT_INT16: return 2;
		case CLOD_NBT_INT32: return 4;
		case CLOD_NBT_INT64: return 8;
		case CLOD_NBT_FLOAT32: return 4;
		case CLOD_NBT_FLOAT64: return 8;
		case CLOD_NBT_INT8_ARRAY: {
			if (available(payload, end) < 4) return 0;
			const size_t size = (size_t)bei32_dec(payload);
			if (available(payload, end) < 4 + size) return 0;
			return 4 + size;
		}
		case CLOD_NBT_INT32_ARRAY: {
			if (available(payload, end) < 4) return 0;
			const size_t size = (size_t)bei32_dec(payload) * 4;
			if (available(payload, end) < 4 + size) return 0;
			return 4 + size;
		}
		case CLOD_NBT_INT64_ARRAY: {
			if (available(payload, end) < 4) return 0;
			const size_t size = (size_t)bei32_dec(payload) * 8;
			if (available(payload, end) < 4 + size) return 0;
			return 4 + size;
		}
		case CLOD_NBT_STRING: {
			if (available(payload, end) < 2) return 0;
			const size_t size = (size_t)beu16_dec(payload);
			if (available(payload, end) < 2 + size) return 0;
			return 2 + size;
		}
		case CLOD_NBT_LIST: {
			if (available(payload, end) < 5) return 0;
			if (payload[0] == CLOD_NBT_ZERO) return 5;
			if (!type_valid(payload[0])) return 0;
			const size_t length = (size_t)bei32_dec(payload + 1);
			size_t size = 5;
			for (size_t i = 0; i < length; i++) {
				const size_t elem_size = clod_nbt_payload_size(payload + size, end, payload[0]);
				if (elem_size == 0) return 0;
				size += elem_size;
			}
			return size;
		}
		case CLOD_NBT_COMPOUND: {
			size_t size = 0;
			for (;;) {
				while (available(payload, end) >= size + 3) {
					if (!type_valid(payload[size])) break;
					const size_t name_size = beu16_dec(payload + size + 1);
					if (available(payload, end) < size + 3 + name_size) return 0;
					size += 3 + name_size + clod_nbt_payload_size(payload + size + 3 + name_size, end, payload[size]);
				}

				if (available(payload, end) < size + 1) return 0;
				if (payload[size] == CLOD_NBT_ZERO) return size + 1;
				return 0;
			}
		}
	}
}

size_t clod_nbt_tag_size(const char *restrict tag, const void *end) {
	if (available(tag, end) < 3) return 0;
	if (!type_valid(tag[0])) return 0;
	const size_t name_size = beu16_dec(tag + 1);
	if (available(tag, end) < 3 + name_size) return 0;
	return 3 + name_size + clod_nbt_payload_size(tag + 3 + name_size, end, tag[0]);
}

char *clod_nbt_tag_payload(const char *restrict tag, const void *end) {
	if (available(tag, end) < 3) return nullptr;
	if (!type_valid(tag[0])) return nullptr;
	const size_t name_size = beu16_dec(tag + 1);
	if (available(tag, end) < 3 + name_size) return nullptr;
	return (char*)tag + 3 + name_size;
}

clod_sstr clod_nbt_tag_name(const char *restrict tag, const void *end) {
	if (available(tag, end) < 3) return CLOD_SSTR_NULL;
	if (!type_valid(tag[0])) return CLOD_SSTR_NULL;
	const size_t name_size = beu16_dec(tag + 1);
	if (available(tag, end) < 3 + name_size) return CLOD_SSTR_NULL;
	return clod_sstr(tag + 3, name_size);
}

bool clod_nbt_iter_next(
	const char *const restrict payload,
	const void *const end,
	const char payload_type,
	struct clod_nbt_iter *iter
) {
	switch (payload_type) {
	case CLOD_NBT_COMPOUND: {
		if (iter->payload == nullptr) {
			memset(iter, 0, sizeof(*iter));
			iter->tag = (char*)payload;
		} else {
			iter->tag += iter->size;
			iter->index++;
		}

		if (available(iter->tag, end) < 1) goto iter_fail;
		if (iter->tag[0] == CLOD_NBT_ZERO) {
			iter->tag++;
			iter->payload = nullptr;
			iter->size = 0;
			iter->type = CLOD_NBT_ZERO;
			return false;
		}

		char *tag_payload = clod_nbt_tag_payload(iter->tag, end);
		if (!tag_payload) goto iter_fail;
		const size_t payload_size = clod_nbt_payload_size(tag_payload, end, iter->tag[0]);
		if (payload_size == 0) goto iter_fail;

		iter->payload = tag_payload;
		iter->size = (size_t)(tag_payload - iter->tag) + payload_size;
		iter->type = iter->tag[0];
		return true;
	}
	case CLOD_NBT_LIST: {
		if (iter->payload == nullptr) {
			if (available(payload, end) < 5) goto iter_fail;
			memset(iter, 0, sizeof(*iter));
			iter->payload = (char*)payload;
			iter->type = payload[0];
		} else {
			iter->payload += iter->size;
			iter->index++;
		}

		if (iter->index >= (uint32_t)bei32_dec(payload + 1)) {
			iter->tag = iter->payload;
			iter->payload = nullptr;
			iter->size = 0;
			iter->type = CLOD_NBT_ZERO;
			return false;
		}

		const size_t payload_size = clod_nbt_payload_size(iter->payload, end, payload[0]);
		if (payload_size == 0) goto iter_fail;
		iter->size = payload_size;
		return true;
	}
	case CLOD_NBT_STRING: {
		if (iter->payload == nullptr) {
			if (available(payload, end) < 2) goto iter_fail;
			memset(iter, 0, sizeof(*iter));
			iter->payload = (char*)payload;
			iter->size = 1;
			iter->type = CLOD_NBT_INT8;
		} else {
			iter->payload++;
			iter->index++;
		}

		if (iter->index >= beu16_dec(payload)) {
			iter->tag = iter->payload;
			iter->payload = nullptr;
			iter->size = 0;
			iter->type = CLOD_NBT_ZERO;
			return false;
		}
		return true;
	}
	case CLOD_NBT_INT8_ARRAY: {
		if (iter->payload == nullptr) {
			if (available(payload, end) < 4) goto iter_fail;
			memset(iter, 0, sizeof(*iter));
			iter->payload = (char*)payload;
			iter->size = 1;
			iter->type = CLOD_NBT_INT8;
		} else {
			iter->payload++;
			iter->index++;
		}

		if (iter->index >= (uint32_t)bei32_dec(payload)) {
			iter->tag = iter->payload;
			iter->payload = nullptr;
			iter->size = 0;
			iter->type = CLOD_NBT_ZERO;
			return false;
		}
		return true;
	}
	case CLOD_NBT_INT32_ARRAY: {
		if (iter->payload == nullptr) {
			if (available(payload, end) < 4) goto iter_fail;
			memset(iter, 0, sizeof(*iter));
			iter->payload = (char*)payload;
			iter->size = 4;
			iter->type = CLOD_NBT_INT32;
		} else {
			iter->payload += 4;
			iter->index++;
		}

		if (iter->index >= (uint32_t)bei32_dec(payload)) {
			iter->tag = iter->payload;
			iter->payload = nullptr;
			iter->size = 0;
			iter->type = CLOD_NBT_ZERO;
			return false;
		}
		return true;
	}
	case CLOD_NBT_INT64_ARRAY: {
		if (iter->payload == nullptr) {
			if (available(payload, end) < 4) goto iter_fail;
			memset(iter, 0, sizeof(*iter));
			iter->payload = (char*)payload;
			iter->size = 8;
			iter->type = CLOD_NBT_INT64;
		} else {
			iter->payload += 8;
			iter->index++;
		}

		if (iter->index >= (uint32_t)bei32_dec(payload)) {
			iter->tag = iter->payload;
			iter->payload = nullptr;
			iter->size = 0;
			iter->type = CLOD_NBT_ZERO;
			return false;
		}
		return true;
	}
	default: return false;
	}

iter_fail:
	memset(iter, 0, sizeof(*iter));
	return false;
}

char *clod_nbt_compound_get(
	const char *restrict compound,
	const void *end,
	const clod_sstr name
) {
	struct clod_nbt_iter iter = CLOD_NBT_ITER_ZERO;
	while (clod_nbt_iter_next(compound, end, CLOD_NBT_COMPOUND, &iter)) {
		if (clod_sstr_eq(clod_nbt_tag_name(iter.tag, end), name)) return iter.tag;
	}
	return nullptr;
}

char *clod_nbt_compound_add(
	char *restrict compound,
	const void **end,
	ptrdiff_t *free,
	const clod_sstr name,
	const char type
) {
	if (!type_valid(type)) return nullptr;
	const size_t elem_size = 3 + name.size + type_zero_size(type);
	if (!compound) {
		*free -= (ptrdiff_t)elem_size;
		return nullptr;
	}

	struct clod_nbt_iter iter = CLOD_NBT_ITER_ZERO;
	while (clod_nbt_iter_next(compound, *end, CLOD_NBT_COMPOUND, &iter)) {
		if (clod_sstr_eq(clod_nbt_tag_name(iter.tag, *end), name)) return iter.tag;
	}

	if (!iter.tag) return nullptr;
	*free -= (ptrdiff_t)elem_size;
	if (*free < 0) {
		// Out of space.
		return nullptr;
	}

	memmove(iter.tag + elem_size, iter.tag, available(iter.tag, *end) - elem_size);

	iter.tag[0] = type;
	beu16_enc(iter.tag + 1, (uint16_t)name.size);
	memcpy(iter.tag + 3, name.ptr, name.size);
	memset(iter.tag + 3 + name.size, 0, type_zero_size(type));

	*end = *(char**)end + elem_size;
	return iter.tag;
}

bool clod_nbt_compound_del(
	char *restrict compound,
	const void **end,
	ptrdiff_t *free,
	const clod_sstr name
) {
	struct clod_nbt_iter iter = CLOD_NBT_ITER_ZERO;
	while (clod_nbt_iter_next(compound, *end, CLOD_NBT_COMPOUND, &iter)) {
		if (clod_sstr_eq(clod_nbt_tag_name(iter.tag, *end), name)) {
			memmove(iter.tag, iter.tag + iter.size, available(iter.tag, *end) - iter.size);
			*end = *(char**)end - iter.size;
			*free += (ptrdiff_t)iter.size;
			return true;
		}
	}
	return false;
}

bool clod_nbt_list_resize(
	char *restrict list,
	const char **end,
	ptrdiff_t *free,
	char type,
	const uint32_t length
) {
	if (!list) {
		*free -= (ptrdiff_t)(length * type_zero_size(type));
		return false;
	}
	if (available(list, *end) < 5) return false;

	if (list[0] != type) {
		const size_t old_size = clod_nbt_payload_size(list, *end, CLOD_NBT_LIST);
		const size_t new_size = 5 + length * type_zero_size(type);
		if (old_size == 0) return false;

		const ptrdiff_t delta = (ptrdiff_t)new_size - (ptrdiff_t)old_size;
		*free -= delta;
		if (*free < 0) return false;

		memmove(list + new_size, list + old_size, available(list, *end) - old_size);
		memset(list, 0, new_size);
		list[0] = type;
		bei32_enc(list + 1, (int32_t)length);
		*end = *(char**)end + delta;
		return true;
	}

	const uint32_t old_length = (uint32_t)bei32_dec(list + 1);

	if (old_length < length) {
		const size_t old_size = clod_nbt_payload_size(list, *end, CLOD_NBT_LIST);
		const size_t append_size = type_zero_size(list[0]) * (length - old_length);

		*free -= (ptrdiff_t)append_size;
		if (*free < 0) return false;

		memmove(list + old_size + append_size, list + old_size, available(list, *end) - old_size - append_size);
		memset(list + old_size, 0, append_size);
		bei32_enc(list + 1, (int32_t)length);

		*end = *(char**)end + append_size;
		return true;
	}

	if (old_length > length) {
		struct clod_nbt_iter iter = CLOD_NBT_ITER_ZERO;
		char *truncate = nullptr;
		while (clod_nbt_iter_next(list, *end, CLOD_NBT_LIST, &iter))
			if (iter.index == length) truncate = iter.payload;

		if (!iter.tag || !truncate) return false;

		memmove(truncate, iter.tag, available(iter.tag, *end));
		bei32_enc(list + 1, (int32_t)length);
		*free += iter.tag - truncate;
		*end = *(char**)end - (iter.tag - truncate);
		return true;
	}

	return true;
}
