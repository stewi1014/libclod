#include <clod/string.h>

clod_string clod_string_from_cstr(const char *cstr) {
	if (!cstr) return CLOD_STRING_NULL;
	int size = 0;
	while (size < INT_MAX && cstr[size] != '\0') size++;
	return clod_string((char*)cstr, size, 0);
}
clod_string clod_string_from_buff(void *buff, const size_t buff_size) {
	int size = buff_size > INT_MAX ? INT_MAX : (int)buff_size;
	return clod_string(buff, 0, size);
}
int clod_string_cmp(const clod_string str1, const clod_string str2) {
	if (str1.length != str2.length)
		return str1.length - str2.length;

	if (str1.length == 0 && str2.length == 0)
		return true;

	if (str1.pointer == str2.pointer)
		return 0;

	for (int i = 0; i < str1.length; i++)
		if (str1.pointer[i] != str2.pointer[i])
			return str1.pointer[i] - str2.pointer[i];

	return 0;
}
clod_string clod_string_cat(const clod_string str, const clod_string append) {
	int i = 0;
	while (str.length + i < str.capacity && i < append.length) {
		str.pointer[str.length + i] = append.pointer[i];
		i++;
	}
	return clod_string(str.pointer, str.length + i, str.capacity);
}
clod_string clod_string_contains(clod_string str, clod_string elem) {
	if (str.length < elem.length)
		return CLOD_STRING_NULL;

	if (elem.length == 0)
		return clod_string(str.pointer, 0, str.capacity);

	for (int i = 0; i < str.length - elem.length; i++) {
		if (clod_string_cmp(
			clod_string(str.pointer + i, str.length - i, str.capacity - i),
			clod_string(elem.pointer, elem.length, elem.capacity)
		)) {
			return clod_string(str.pointer + i, elem.length, str.capacity - i);
		}
	}

	return CLOD_STRING_NULL;
}
clod_string clod_string_find(const clod_string str, const char elem, int occurrence) {
	if (occurrence > 0) {
		for (int i = 0; i < str.length; i++) {
			if (str.pointer[i] == elem) occurrence--;
			if (occurrence == 0) return clod_string(str.pointer + i, str.length - i, str.capacity - i);
		}
		return CLOD_STRING_NULL;
	}

	if (occurrence < 0) {
		for (int i = str.length; i > 0; i--) {
			if (str.pointer[i - 1] == elem) occurrence++;
			if (occurrence == 0) return clod_string(str.pointer + i - 1, str.length - i + 1, str.capacity - i + 1);
		}
		return CLOD_STRING_NULL;
	}

	// occurrence == 0
	return str;
}
