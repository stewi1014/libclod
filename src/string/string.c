#include <clod/string.h>

void clod_string_put_char(clod_string *str, const char c) {
	if (str->capacity > str->length) {
		str->array[str->length] = c;
		str->length++;
	}
}
char clod_string_get_char(clod_string *str) {
	if (str->length && str->array) {
		char c = str->array[0];
		str->array++;
		str->length--;
		if (str->capacity) str->capacity--;
		return c;
	}
	return 0;
}
char clod_string_peek_char(const clod_string str) {
	if (str.length && str.array) {
		return str.array[0];
	}
	return 0;
}
clod_string clod_string_from_cstr(const char *cstr) {
	if (!cstr) return CLOD_STRING_NULL;

	clod_string str = {
		.array = (char*)cstr,
		.length = 0,
		.capacity = 0
	};

	while (str.array[str.length] != '\0') str.length++;
	return str;
}
int clod_string_cmp(const clod_string str1, const clod_string str2) {
	if (str1.length > str2.length) return 1;
	if (str2.length > str1.length) return -1;
	if (str1.array == str2.array) return 0;

	for (size_t i = 0; i < str1.length; i++) {
		if (str1.array[i] > str2.array[i]) return 1;
		if (str2.array[i] > str1.array[i]) return -1;
	}

	return 0;
}
bool clod_string_has_prefix(clod_string *str, const clod_string prefix) {
	if (str->length < prefix.length) return false;
	for (size_t i = 0; i < prefix.length; i++)
		if (str->array[i] != prefix.array[i])
			return false;

	str->array += prefix.length;
	str->length -= prefix.length;
	str->capacity = str->capacity > prefix.length ? str->capacity - prefix.length : 0;
	return true;
}
size_t clod_string_cat(clod_string *str, clod_string append) {
	while (str->length < str->capacity && append.length > 0) {
		str->array[str->length] = *append.array++;
		append.length--;
		str->length++;
	}
	return append.length;
}
clod_string clod_string_contains(clod_string str, const clod_string elem) {
	if (str.array == nullptr) return CLOD_STRING_NULL;
	if (elem.length == 0) {
		return (clod_string){
			.array = str.array,
			.length = 0,
			.capacity = str.capacity
		};
	}

	while (str.length >= elem.length) {
		size_t i = 0;
		while (i < elem.length && str.array[i] == elem.array[i]) i++;
		if (i == elem.length) {
			return str;
		}

		str.array++;
		str.length--;
		str.capacity--;
	}

	return CLOD_STRING_NULL;
}
clod_string clod_string_find(const clod_string str, const char elem, int occurrence) {
	if (occurrence > 0) {
		for (size_t i = 0; i < str.length; i++) {
			if (str.array[i] == elem) occurrence--;
			if (occurrence == 0) return (clod_string){str.array + i, str.length - i, str.capacity - i};
		}
		return CLOD_STRING_NULL;
	}

	if (occurrence < 0) {
		for (size_t i = str.length; i > 0; i--) {
			if (str.array[i - 1] == elem) occurrence++;
			if (occurrence == 0) return (clod_string){str.array + i - 1, str.length - i + 1, str.capacity - i + 1};
		}
		return CLOD_STRING_NULL;
	}

	// occurrence == 0
	return str;
}
