/**
 * @file clod/string.h
 * @brief Sized string helpers.
 *
 * A definition of a sized string and a few simple methods for dealing with them.
 * It's C-centric and mostly serves as a helper for some C-centric public libclod methods.
 *
 * The general approach here is that methods operate on a particular input clod_string, and return
 * that same string but with the length field updated to reflect the result of the operation.
 * Some methods may make other changes, but no memory allocation is ever performed.
 */
#ifndef LIBCLOD_STRING_H
#define LIBCLOD_STRING_H

#include <clod/lib.h>

/**
 * Array of bytes, often a human-readable string.
 * If length == 0 the string is empty.
 * If capacity <= 0 and pointer != nullptr the string is constant and cannot be mutated.
 * If capacity > length then the string is null terminated.
 */
typedef struct {
	/** Pointer to the string data.
	 * The underlying array may be read-only. */
	char *pointer;

	/** Length of the string in bytes. */
	int length;

	/** Total capacity of the string in bytes. */
	int capacity;
} clod_string;

#define clod_string(ptr, len, cap) ((clod_string){ .pointer = (ptr), .length = (len), .capacity = (cap)})
#define CLOD_STRING_NULL ((clod_string){0})

/// Make a clod_string using a C string.
clod_string clod_string_from_cstr(const char *cstr);

/// Use a buffer to make a new empty clod_string.
/// The string only uses up to INT_MAX bytes.
clod_string clod_string_from_buff(void *buff, size_t buff_size);

/**
 * Compare two strings.
 * Strings are first compared by their length, longer is larger, returning str2.length - str2.length.
 * Then they are compared bytewise, returning the result of str1[i] - str2[i] for the first non-equal byte.
 * Otherwise, they are considered equal and 0 is returned.
 *
 * @param[in] str1 1st string.
 * @param[in] str2 2nd string.
 * @return The result of str1 - str2.
 */
CLOD_API
int clod_string_cmp(clod_string str1, clod_string str2);

/**
 * Append a string to antoher string.
 *
 * @param[in] str String to append to.
 * @param[in] append String to append.
 * @return str with \p append concatenated.
 * If the operation was truncated the returned strings size will be equal to its capacity.
 */
CLOD_API
clod_string clod_string_cat(clod_string str, clod_string append);

/**
 * Find the first instance of a string inside another string.
 *
 * @param[in] str The string to search in.
 * @param[in] elem String to search for.
 * @return String pointing to the same object \p str points to at the offset
 * where \p elem was found and with a size equal to \p elem's size.
 * If no match was found, it returns null.
 */
CLOD_API
clod_string clod_string_contains(clod_string str, clod_string elem);

/**
 * Find the Nth instance of \p elem in \p str.
 * When \p occurrence is positive or negative, it searches from the start or end of \p str respectively.
 * Then, abs(\p occurrence) - 1 instances of \p elem are skipped,
 * and a string pointing to the final instance returned.
 *
 * @param[in] str The string to search in.
 * @param[in] elem The character to search for.
 * @param[in] occurrence Which instance of \p elem to search for.
 * @return String pointing to the found instance of \p elem,
 * and a length including the rest of \p str.
 * If \p occurrence is 0, \p str is returned.
 */
CLOD_API
clod_string clod_string_find(clod_string str, char elem, int occurrence);

#endif
