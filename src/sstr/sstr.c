#include <clod/sstr.h>
#include <string.h>

bool clod_sstr_eq(const clod_sstr str1, const clod_sstr str2) {
	if (str1.size != str2.size) return false;
	if (str1.ptr == str2.ptr) return true;
	if (str1.ptr == nullptr || str2.ptr == nullptr) return false;
	return memcmp(str1.ptr, str2.ptr, str1.size) == 0;
}
void clod_sstr_cat(clod_sstr *str1, const clod_sstr str2) {
	if (str2.size == 0) return;
	memcpy(str1->ptr + str1->size, str2.ptr, str2.size);
	str1->size += str2.size;
}
clod_sstr clod_sstr_contains(const clod_sstr str, const clod_sstr elem) {
	if (str.size < elem.size) return CLOD_SSTR_NULL;
	if (elem.size == 0) return clod_sstr(str.ptr, 0);

	for (size_t i = 0; i < str.size - elem.size; i++)
		if (memcmp(str.ptr + i, elem.ptr, elem.size) == 0)
			return clod_sstr(str.ptr + i, elem.size);

	return CLOD_SSTR_NULL;
}
clod_sstr clod_sstr_find(const clod_sstr str, const char elem, ptrdiff_t occurrence) {
	if (occurrence > 0) {
		for (size_t i = 0; i < str.size; i++) {
			if (str.ptr[i] == elem) occurrence--;
			if (occurrence == 0) return clod_sstr(str.ptr + i, str.size - i);
		}
		return CLOD_SSTR_NULL;
	}

	if (occurrence < 0) {
		for (size_t i = str.size; i > 0; i--) {
			if (str.ptr[i - 1] == elem) occurrence++;
			if (occurrence == 0) return clod_sstr(str.ptr + i - 1, str.size - i + 1);
		}
		return CLOD_SSTR_NULL;
	}

	// occurrence == 0
	return str;
}
