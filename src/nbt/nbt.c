#include <alloca.h>
#include <assert.h>
#include <string.h>
#include <clod/nbt.h>

constexpr size_t payload_zero_size_[] = {
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

#define type_valid(type) (0 <= (type) && (type) < (clod_nbt_type)(sizeof(payload_zero_size_) / sizeof(payload_zero_size_[0])) && payload_zero_size_[type] > 0)
#define payload_zero_size(type) (assert(type_valid(type)), payload_zero_size_[type])
#define available(ptr, end) (assert((char*)ptr <= (char*)(end)), (size_t)((char*)(end) - (char*)ptr))
#define ptr_add(ptr, end, offset) (assert(available(ptr, end) >= offset), (void*)((char*)(ptr) + (offset)))

// caller has responsibility to ensure valid inputs.
// the return value is not checked for validity.
CLOD_PURE CLOD_INLINE
static size_t
payload_size(const union clod_nbt_payload *restrict payload, const clod_nbt_type payload_type, const char *const end) {
	switch (payload_type) {
		default: return 0;
		case CLOD_NBT_INT8: return 1;
		case CLOD_NBT_INT16: return 2;
		case CLOD_NBT_INT32: return 4;
		case CLOD_NBT_INT64: return 8;
		case CLOD_NBT_FLOAT32: return 4;
		case CLOD_NBT_FLOAT64: return 8;
		case CLOD_NBT_INT8_ARRAY: {
			if (sizeof(payload->byte_array) > available(payload, end)) return 0;
			return sizeof(payload->byte_array) + (size_t)be(payload->byte_array.length);
		}
		case CLOD_NBT_INT32_ARRAY: {
			if (sizeof(payload->int32_array) > available(payload, end)) return 0;
			return sizeof(payload->int32_array) + (size_t)be(payload->int32_array.length) * 4;
		}
		case CLOD_NBT_INT64_ARRAY: {
			if (sizeof(payload->int64_array) > available(payload, end)) return 0;
			return sizeof(payload->int64_array) + (size_t)be(payload->int64_array.length) * 8;
		}
		case CLOD_NBT_STRING: {
			if (sizeof(payload->string) > available(payload, end)) return 0;
			return sizeof(payload->string) + be(payload->string.length);
		}
		case CLOD_NBT_LIST: {
			if (sizeof(payload->list) > available(payload, end)) return 0;
			size_t size = sizeof(payload->list);
			for (int32_t i = 0; i < be(payload->list.length) && size < available(payload, end); i++) {
				const size_t elem_size = payload_size(ptr_add(payload, end, size), payload->list.payload_type, end);
				if (elem_size == 0) return 0;
				size += elem_size;
			}
			return size;
		}
		case CLOD_NBT_COMPOUND: {
			size_t size = 0;
			while (
				size + sizeof(union clod_nbt_tag) < available(payload, end) &&
				*(clod_nbt_type*)ptr_add(payload, end, size) != CLOD_NBT_ZERO
			) {
				auto const elem = (union clod_nbt_tag*)ptr_add(payload, end, size);
				size += sizeof(union clod_nbt_tag) + be(elem->name_size);
				if (size > available(payload, end)) return 0;
				size += payload_size(ptr_add(payload, end, size), elem->type, end);
			}
			return size + 1;
		}
	}
}
size_t clod_nbt_payload_size(const union clod_nbt_payload *restrict payload, const clod_nbt_type payload_type, const char *const end) {
	const size_t size = payload_size(payload, payload_type, end);
	if (size > available(payload, end)) return 0;
	return size;
}
size_t clod_nbt_tag_size(const union clod_nbt_tag *restrict tag, const char *const end) {
	if (
		sizeof(*tag) > available(tag, end) ||
		!type_valid(tag->type)
	) return 0;

	const size_t tag_size = sizeof(*tag) + be(tag->name_size);
	if (tag_size > available(tag, end)) return 0;
	const size_t payload_size = clod_nbt_payload_size(ptr_add(tag, end, tag_size), tag->type, end);
	if (payload_size == 0) return 0;
	return tag_size + payload_size;
}
union clod_nbt_payload *clod_nbt_tag_payload(const union clod_nbt_tag *tag, const char *end, const clod_nbt_type payload_type) {
	if (
		payload_type == CLOD_NBT_ZERO ||
		sizeof(*tag) > available(tag, end) ||
		!type_valid(tag->type) ||
		tag->type != payload_type
	) return nullptr;

	const size_t tag_size = sizeof(*tag) + be(tag->name_size);
	if (tag_size > available(tag, end)) return nullptr;
	return ptr_add(tag, end, tag_size);
}

union clod_nbt_tag *
clod_nbt_compound(
	union clod_nbt_payload *compound,
	const char **end,
	ptrdiff_t *free,
	const clod_nbt_type type,
	sstr name
) {
	size_t off = 0;
	while (
		off + sizeof(union clod_nbt_tag) < available(compound, *end) &&
		*(clod_nbt_type*)ptr_add(compound, *end, off) != CLOD_NBT_ZERO
	) {
		union clod_nbt_tag *elem = ptr_add(compound, *end, off);
		const size_t elem_size = clod_nbt_tag_size(elem, *end);
		if (elem_size == 0) return nullptr;
		if (sstr_cmp(sstr(elem->name, be(elem->name_size)), name)) {
			if (type == CLOD_NBT_ZERO) {
				// delete
				__builtin
				memmove(elem, ptr_add(elem, *end, elem_size), available(elem, *end) - elem_size);
				memset(available())
			}
		}

		off += sizeof(union clod_nbt_tag) + be(elem->name_size);
		if (off > available(compound, *end)) return nullptr;
		const size_t elem_size = clod_nbt_tag_size(elem, *end);

		if (sstr_cmp(sstr(elem->name, be(elem->name_size)), name)) {
			if (type == CLOD_NBT_ZERO) {
				// Delete
				memmove(elem, (char*)elem + elem_size, available(elem, *end) - elem_size);
				memset(ptr_add(elem, *end, available(elem, *end) - elem_size), 0, elem_size);
				*end -= elem_size;
				*free += (ptrdiff_t)elem_size;
				return nullptr;
			}

			// Get
			return elem;
		}

		off += elem_size;
	}

	if (
		off + 1 <= available(compound, *end) &&
		*(clod_nbt_type*)ptr_add(compound, *end, off) == CLOD_NBT_ZERO
	) {
		// Create
		const size_t elem_size = sizeof(union clod_nbt_tag) + name.size + payload_zero_size(type);
		if ((size_t)*free < elem_size) return nullptr;
		union clod_nbt_tag *elem = ptr_add(compound, *end, off);
		memmove(ptr_add(elem, *end, elem_size), ptr_add(compound, *end, off), available(compound, *end) - off);

		off += (ptrdiff_t)( sizeof(union clod_nbt_tag) + name.size)
	}

	union clod_nbt_tag *elem = ptr_add(compound, *end, off);
	elem->type = type;
	elem->name_size = sstr_size(name);
	memcpy(elem->name, name.ptr, elem->name_size);
	*end += sizeof(union clod_nbt_tag) + elem->name_size;
	*end_offset += (ptrdiff_t)sizeof(union clod_nbt_tag) + elem->name_size;
}

union clod_nbt_tag *clod_nbt_compound_get(const union clod_nbt_payload *compound, const char *end, sstr name) {
	size_t off = 0;
	while (
		off + sizeof(union clod_nbt_tag) < available(compound, end) &&
		*(clod_nbt_type*)ptr_add(compound, end, off) != CLOD_NBT_ZERO
	) {
		union clod_nbt_tag *elem = (union clod_nbt_tag*)ptr_add(compound, end, off);
		off += sizeof(union clod_nbt_tag) + be(elem->name_size);
		if (off > available(compound, end)) return nullptr;
		if (sstr_cmp(sstr(elem->name, be(elem->name_size)), name)) return elem;
		off += payload_size(ptr_add(compound, end, off), elem->type, end);
	}
	return nullptr;
}
union clod_nbt_payload *clod_nbt_list_get(const union clod_nbt_payload *list, const char *end, const size_t index) {
	if (index >= (size_t)be(list->list.length)) return nullptr;
	size_t off = sizeof(list->list);
	for (size_t i = 0; i < index && off < available(list, end); i++) {
		const size_t elem_size = payload_size(ptr_add(list, end, off), list->list.payload_type, end);
		if (elem_size == 0) return nullptr;
		off += elem_size;
	}
	if (off > available(list, end)) return nullptr;
	return ptr_add(list, end, off);
}
union clod_nbt_tag *clod_nbt_compound_get(union clod_nbt_payload *compound, const char **end, ptrdiff_t *end_offset,
	clod_nbt_type type, sstr name) {




	size_t off = 0;
	while (
		off + sizeof(union clod_nbt_tag) < available(compound, *end) &&
		*(clod_nbt_type*)ptr_add(compound, *end, off) != CLOD_NBT_ZERO
	) {
		auto const elem = (union clod_nbt_tag*)ptr_add(compound, *end, off);
		off += sizeof(union clod_nbt_tag) + be(elem->name_size);
		if (off > available(compound, *end)) return nullptr;
		if (sstr_cmp(sstr(elem->name, be(elem->name_size)), name)) return elem;
		off += payload_size(ptr_add(compound, *end, off), elem->type, *end);
	}
}

/*
void clod_nbt_get(const union clod_nbt_tag *tag, const char *end, const union clod_nbt_tag **children, const size_t children_count) {
	if (children_count == 0) return;
	const union clod_nbt_tag **children_in = alloca(sizeof(*children) * children_count);
	memcpy(children_in, children, sizeof(*children) * children_count);
	memset(children, 0, sizeof(*children) * children_count);

	if (
		tag->ptr > end ||
		end - tag->ptr < sizeof(*tag) ||
		tag->type != CLOD_NBT_COMPOUND ||
		end - tag->name < be(tag->name_size)
	) return;



	auto elem = (union clod_nbt_tag*)(tag->ptr + sizeof(*tag) + be(tag->name_size));
	size_t found = 0;
	while (found < children_count && end - elem->ptr >= sizeof(*elem) && elem->type != CLOD_NBT_ZERO) {
		const size_t elem_name_size = be(elem->name_size);
		if (end - elem->name < elem_name_size) return;

		for (size_t i = 0; i < children_count; i++) {
			if (
				!children[i] &&
				children_in[i]->type == elem->type &&
				string_eq(string(children_in[i]->name, be(children_in[i]->name_size)), string(elem->name, be(elem->name_size)))
			) {
				children[i] = elem;
				found++;
			}
		}

		const size_t elem_size = clod_nbt_tag_size(elem, end);
		if (end - elem->ptr < elem_size || elem_size == 0) return;
		elem = (union clod_nbt_tag*)(elem->ptr + elem_size);
	}
}
*/