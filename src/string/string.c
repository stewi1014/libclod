#include "clod_config.h"
#include "clod/debug.h"
#include <clod/string.h>

struct clod_string clod_string_from_cstr(const char *cstr) {
	if (!cstr) return CLOD_STRING_NULL;

	struct clod_string str = {
		.ptr = (char*)cstr,
		.len = 0,
		.cap = 0
	};

	while (cstr[str.len] != '\0') str.len++;
	return str;
}
size_t clod_string_cat(struct clod_string *dst, const struct clod_string src) {
	if (dst->ptr == nullptr || dst->cap == 0 || src.ptr == nullptr) {
		return 0;
	}

	ptrdiff_t size = dst->cap - dst->len;
	if (size > src.len) {
		size = src.len;
	}

	if (size < 0) {
		return 0;
	}

	if (src.ptr > dst->ptr && src.ptr < dst->ptr + size) {
		for (ptrdiff_t i = size - 1; i >= 0; i--) {
			dst->ptr[i + dst->len] = src.ptr[i];
		}
	} else {
		for (ptrdiff_t i = 0; i < size; i++) {
			dst->ptr[i + dst->len] = src.ptr[i];
		}
	}

	dst->len += size;
	return (size_t)size;
}
size_t clod_string_insert(struct clod_string *dst, struct clod_string src) {
	if (dst->ptr == nullptr || dst->cap == 0 || src.ptr == nullptr) {
		return 0;
	}

	ptrdiff_t size = src.len < dst->cap ? src.len : dst->cap;

	if (size < 0) {
		return 0;
	}

	clod_string_cat(
		&(struct clod_string){
			.ptr = dst->ptr,
			.len = size,
			.cap = dst->cap
		},
		*dst
	);

	dst->len += (ptrdiff_t)clod_string_cat(
		&(struct clod_string){
			.ptr = dst->ptr,
			.len = 0,
			.cap = size
		},
		src
	);

	return (size_t)size;
}
int clod_string_cmp(const struct clod_string str1, const struct clod_string str2) {
	if (str1.len > str2.len) return 1;
	if (str2.len > str1.len) return -1;
	if (str1.ptr == str2.ptr) return 0;

	for (ptrdiff_t i = 0; i < str1.len; i++) {
		if (str1.ptr[i] > str2.ptr[i]) return 1;
		if (str2.ptr[i] > str1.ptr[i]) return -1;
	}

	return 0;
}
void clod_string_put_char(struct clod_string *str, const char c) {
	if (str->ptr && str->cap > str->len) {
		str->ptr[str->len] = c;
		str->len++;
	}
}
char clod_string_get_char(struct clod_string *str) {
	if (str->ptr && str->len > 0) {
		char c = str->ptr[0];
		str->ptr++;
		str->len--;
		if (str->cap > 0) str->cap--;
		return c;
	}
	return 0;
}
char clod_string_peek_char(const struct clod_string str) {
	if (str.ptr && str.len > 0) {
		return str.ptr[0];
	}
	return 0;
}
bool clod_string_remove_prefix(struct clod_string *str, const struct clod_string prefix) {
	if (!str->ptr) return false;
	if (!prefix.ptr) return true;

	if (str->len < prefix.len) return false;
	for (ptrdiff_t i = 0; i < prefix.len; i++)
		if (str->ptr[i] != prefix.ptr[i])
			return false;

	str->ptr += prefix.len;
	str->len -= prefix.len;
	str->cap -= prefix.len;
	return true;
}
struct clod_string clod_string_contains(struct clod_string str, const struct clod_string elem) {
	if (str.ptr == nullptr) return CLOD_STRING_NULL;
	if (elem.len == 0) {
		return (struct clod_string){
			.ptr = str.ptr,
			.len = 0,
			.cap = str.cap
		};
	}

	while (str.len >= elem.len) {
		ptrdiff_t i = 0;
		while (i < elem.len && str.ptr[i] == elem.ptr[i]) i++;
		if (i == elem.len) {
			return str;
		}

		str.ptr++;
		str.len--;
		str.cap--;
	}

	return CLOD_STRING_NULL;
}
struct clod_string clod_string_find(const struct clod_string str, const char elem, int occurrence) {
	if (occurrence > 0) {
		for (ptrdiff_t i = 0; i < str.len; i++) {
			if (str.ptr[i] == elem) occurrence--;
			if (occurrence == 0) return (struct clod_string){str.ptr + i, str.len - i, str.cap - i};
		}
		return CLOD_STRING_NULL;
	}

	if (occurrence < 0) {
		for (ptrdiff_t i = str.len; i > 0; i--) {
			if (str.ptr[i - 1] == elem) occurrence++;
			if (occurrence == 0) return (struct clod_string){str.ptr + i - 1, str.len - i + 1, str.cap - i + 1};
		}
		return CLOD_STRING_NULL;
	}

	// occurrence == 0
	return str;
}
