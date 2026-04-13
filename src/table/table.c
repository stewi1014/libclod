#include <clod/structures/table.h>
#include <clod/hash.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LF_MAX 85u
#define LF_MIN 50u
#define LF_DENOMINATOR 100u

#define LF(table) (((table)->elem_count + (table)->deleted_count) * LF_DENOMINATOR / (table)->table_size)
// the size of the table needed to store capacity elements at the given load factor
#define LF_CAPACITY_TO_SIZE(lf, capacity) (((uint64_t)(capacity) * LF_DENOMINATOR + (lf) - 1) / (lf))

#define CTL_HASH_BITS 7
#define CTL_EMPTY             (0b00000000)
#define CTL_OCCUPIED(hash)    (0b10000000 | (uint8_t)(hash))
#define CTL_REMOVED	          (0b00000001)
#define CTL_IS_OCCUPIED(ctl) ((0b10000000 & (ctl)) > 0)

#define INDEX_NIL ((size_t)-1)

struct element {
	const void *element;
	size_t key_size;
};
struct clod_table {
	struct clod_table_opts opts;

	size_t elem_count;
	size_t deleted_count;
	size_t table_size;
	size_t cursor;

	uint8_t *restrict control;
	struct element *restrict elements;
};
static struct table_position {
	size_t index;
	uint8_t ctl;
}
get_position(const struct clod_table *t, const void *key, const size_t key_size) {
	assert(t->table_size > 0);
	auto const hash = t->opts.hash_func((size_t)t->control, key, key_size, t->opts.user);

	return (struct table_position){
		.ctl = CTL_OCCUPIED(hash),
		.index = (hash >> CTL_HASH_BITS) % t->table_size
	};
}
static bool create(struct clod_table *t, const struct clod_table_opts *opts, const size_t new_table_size) {
	memcpy(&t->opts, opts, sizeof(t->opts));
	t->elem_count = 0;
	t->deleted_count = 0;
	t->table_size = new_table_size < LF_CAPACITY_TO_SIZE(LF_MAX, t->opts.min_capacity)
		? LF_CAPACITY_TO_SIZE(LF_MAX, t->opts.min_capacity)
		: new_table_size;
	t->cursor = 0;

	if (t->table_size > 0) {
		t->elements = t->opts.malloc_func(t->table_size * (sizeof(t->control[0]) + sizeof(t->elements[0])), t->opts.user);
		if (!t->elements) {
			return false;
		}
		t->control = (uint8_t*)((char*)t->elements + t->table_size * sizeof(t->elements[0]));
		memset(t->control, 0, t->table_size);
	} else {
		t->control = nullptr;
		t->elements = nullptr;
	}

	return true;
}
static void destroy(const struct clod_table *t) {
	t->opts.free_func(t->elements, t->opts.user);
}
static struct probe {
	size_t existing;
	size_t available;
}
probe(const struct clod_table *t, const struct table_position pos, const void *key, const size_t key_size) {
	assert(pos.index < t->table_size);

	size_t available = INDEX_NIL;
	for (size_t i = 0; i < t->table_size; i++) {
		const size_t index = (pos.index + i) % t->table_size;

		if (t->control[index] == CTL_EMPTY) {
			return (struct probe){
				.existing = INDEX_NIL,
				.available = available != INDEX_NIL ? available : index
			};
		}

		if (
			t->control[index] == pos.ctl &&
			t->opts.cmp_func(
				t->elements[index].element,
				t->elements[index].key_size,
				key,
				key_size,
				t->opts.user
			) == 0
		) {
			return (struct probe){
				.existing = index,
				.available = INDEX_NIL
			};
		}

		if (t->control[index] == CTL_REMOVED && available == INDEX_NIL) {
			available = index;
		}
	}

	return (struct probe){
		.existing = INDEX_NIL,
		.available = available
	};
}
static const void *insert(struct clod_table *t, const bool replace, const void *element, const size_t key_size) {
	assert(t->elem_count < t->table_size);

	auto const pos = get_position(t, element, key_size);
	auto const res = probe(t, pos, element, key_size);

	if (res.existing != INDEX_NIL) {
		const void *previous = t->elements[res.existing].element;
		if (replace) {
			t->elements[res.existing].element = element;
			t->elements[res.existing].key_size = key_size;
		}
		return previous;
	}

	assert(res.available != INDEX_NIL);
	t->elem_count++;
	if (t->control[res.available] == CTL_REMOVED) t->deleted_count--;
	t->control[res.available] = pos.ctl;
	t->elements[res.available].element = element;
	t->elements[res.available].key_size = key_size;
	return nullptr;
}
static bool rebuild(struct clod_table *t, const size_t new_table_size) {
	assert(new_table_size >= t->elem_count);

	struct clod_table new;
	if (!create(&new, &t->opts, new_table_size)) {
		return false;
	}

	auto iter = CLOD_TABLE_ITER_INIT;
	while (clod_table_iter(t, &iter)) {
		if (insert(&new, false, iter.element, iter.key_size) != nullptr) {
			destroy(&new);
			return false;
		}
	}

	destroy(t);
	memcpy(t, &new, sizeof(new));
	return true;
}
static uint64_t default_hash(const uint64_t seed, const void *data, const size_t data_size, void*) { return clod_sip64(seed, data, data_size); }
static int default_cmp(const void *key1, size_t key1_size, const void *key2, const size_t key2_size, void *) {
	if (key1_size > key2_size) return 1;
	if (key1_size < key2_size) return -1;
	return memcmp(key1, key2, key1_size);
}
uint64_t clod_table_hash_ptr(const uint64_t seed, const void *key, size_t, void *) {
	uint8_t data[sizeof(void*)];
	memcpy(data, &key, sizeof(void*));
	return clod_sip64(seed, data, sizeof(void*));
}
int clod_table_cmp_ptr(const void *key1, size_t, const void *key2, size_t, void *) {
	if (key1 > key2) return 1;
	if (key1 < key2) return -1;
	return 0;
}
static void *default_malloc(const size_t size, void*) { return malloc(size); }
static void default_free(void *ptr, void*) { free(ptr); }
static void apply_default_opts(struct clod_table_opts *opts) {
	if (!opts->hash_func) opts->hash_func = default_hash;
	if (!opts->cmp_func) opts->cmp_func = default_cmp;
	if (!opts->malloc_func) opts->malloc_func = default_malloc;
	if (!opts->free_func) opts->free_func = default_free;
}
struct clod_table *clod_table_create(const struct clod_table_opts *opts) {
	struct clod_table *t = opts && opts->malloc_func ? opts->malloc_func(sizeof(*t), opts->user) : malloc(sizeof(*t));
	if (!t) return nullptr;

	if (opts) memcpy(&t->opts, opts, sizeof(t->opts));
	else memset(&t->opts, 0, sizeof(t->opts));
	apply_default_opts(&t->opts);

	if (!create(t, &t->opts, LF_CAPACITY_TO_SIZE(LF_MAX, t->opts.min_capacity))) {
		t->opts.free_func(t, t->opts.user);
		return nullptr;
	}

	return t;
}
void clod_table_destroy(struct clod_table *t) {
	destroy(t);
	t->opts.free_func(t, t->opts.user);
}
size_t clod_table_len(const struct clod_table *t) {
	return t->elem_count;
}
bool clod_table_add(struct clod_table *t, const void *element, const size_t key_size, void **existing_out) {
	if (t->table_size == 0 || LF(t) >= LF_MAX) {
		if (!rebuild(t, LF_CAPACITY_TO_SIZE(LF_MIN, t->elem_count + 1))) {
			if (existing_out) *existing_out = nullptr;
			return false;
		}
	}

	const void *existing = insert(t, false, element, key_size);
	if (existing_out) *existing_out = (void*)existing;
	return !existing;
}
bool clod_table_set(struct clod_table *t, const void *element, const size_t key_size, void **existing_out) {
	if (t->table_size == 0 || LF(t) >= LF_MAX) {
		if (!rebuild(t, LF_CAPACITY_TO_SIZE(LF_MIN, t->elem_count + 1))) {
			*existing_out = nullptr;
			return false;
		}
	}

	const void *existing = insert(t, true, element, key_size);
	*existing_out = (void*)existing;
	return true;
}
void *clod_table_get(const struct clod_table *t, const void *key, const size_t key_size) {
	if (t->table_size == 0) return nullptr;
	auto const res = probe(t, get_position(t, key, key_size), key, key_size);
	if (res.existing != INDEX_NIL) {
		return (void*)t->elements[res.existing].element;
	}

	return nullptr;
}
void *clod_table_del(struct clod_table *t, const void *key, const size_t key_size) {
	if (t->table_size == 0) return nullptr;
	auto const res = probe(t, get_position(t, key, key_size), key, key_size);
	if (res.existing != INDEX_NIL) {
		t->control[res.existing] = CTL_REMOVED;
		t->elem_count--;
		t->deleted_count++;
		t->cursor++;
		return (void*)t->elements[res.existing].element;
	}

	return nullptr;
}
bool clod_table_iter(const struct clod_table *t, struct clod_table_iter *iter) {
	while (iter->_internal < t->table_size) {
		const size_t index = (iter->_internal + t->cursor) % t->table_size;
		if (CTL_IS_OCCUPIED(t->control[index])) {
			auto const res = t->elements[index];
			iter->element = (void*)res.element;
			iter->key_size = res.key_size;
			iter->_internal++;
			return true;
		}
		iter->_internal++;
	}
	iter->_internal = 0;
	iter->element = nullptr;
	iter->key_size = 0;
	return false;
}
