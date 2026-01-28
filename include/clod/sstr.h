/**
 * @file sstr.h
 * @brief Sized string helpers.
 *
 * A few simple methods for dealing with strings when an explicit size is needed instead of being zero terminated.
 * It's C-centric (obviously) and mostly serves as a helper for some C-centric public libclod methods.
 */
#ifndef CLOD_SSTR_H
#define CLOD_SSTR_H

#include <clod/lib.h>
#include <stddef.h>
#include <string.h>

/**
 * Sized string.
 */
typedef struct {
	char *ptr;
	size_t size;
} clod_sstr;

// Create a sized string from char array and size. The string retains ptr_v.
#define clod_sstr_wrap(ptr_v, size_v) ((sstr){ .ptr = (char*)(ptr_v), .size = (size_v) })
// Null string value
#define CLOD_SSTR_NULL ((sstr){ .ptr = nullptr, .size = 0 })
// C string to sized string. The string retains c_string.
#define CLOD_SSTR_C(c_string) ((sstr){ .ptr = (char*)(c_string), .size = strlen(c_string) })
// Compare two clod_sstr strings.
#define clod_sstr_cmp(str1, str2) clod_str_cmp((str1).ptr, (str1).size, (str2).ptr, (str2).size)

/**
 * Compare two strings.
 * @param[in] str1 1st string. Not dereferenced if \p str1_size is 0.
 * @param[in] str1_size Size of \p str1.
 * @param[in] str2 2nd string. Not dereferenced if \p str2_size is 0.
 * @param[in] str2_size Size of \p str2.
 * @return If the strings are equal.
 */
CLOD_INLINE
static inline bool clod_str_cmp(const char *str1, size_t str1_size, const char *str2, size_t str2_size) {
	if (str1_size != str2_size) return false;
	if (str1 == str2) return true;
	if (str1 == nullptr || str2 == nullptr) return false;
	return memcmp(str1, str2, str1_size) == 0;
}

#endif
