/**
 * @file nbt.h
 * @brief Sized string library.
 *
 * A few simple methods for dealing with strings when an explicit size is needed instead of being zero terminated.
 */
#ifndef CLOD_SSTR_H
#define CLOD_SSTR_H

#include <stddef.h>
#include <string.h>

/**
 * Sized string.
 */
typedef struct {
	char *ptr;
	size_t size;
} sstr;

// Null string value
#define SSTR_NULL           ((sstr){ .ptr = nullptr,           .size = 0                })
// C string to sized string. The string retains c_string.
#define SSTR_C(c_string)    ((sstr){ .ptr = (char*)(c_string), .size = strlen(c_string) })
// Create a sized string from char array and size. The string retains ptr_v.
#define sstr(ptr_v, size_v) ((sstr){ .ptr = (char*)(ptr_v),    .size = (size_v)         })

/**
 * Compare two strings.
 * @return True if the strings are equal, false otherwise.
 */
static inline bool sstr_cmp(const sstr str1, const sstr str2) {
	if (str1.size != str2.size) return false;
	if (str1.ptr == str2.ptr) return true;
	if (str1.ptr == nullptr || str2.ptr == nullptr) return false;
	return memcmp(str1.ptr, str2.ptr, str1.size) == 0;
}

#endif
