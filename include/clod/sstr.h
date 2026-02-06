/**
 * @file sstr.h
 * @brief Sized string helpers.
 *
 * A definition of a sized string and a few simple methods for dealing with them.
 * It's C-centric (obviously) and mostly serves as a helper for some C-centric public libclod methods.
 */
#ifndef LIBCLOD_SSTR_H
#define LIBCLOD_SSTR_H

#include <clod/lib.h>
#include <stddef.h>

/**
 * Sized string.
 * If size is 0, ptr should never be dereferenced.
 */
typedef struct {
	/** Pointer to the string data.
	 * The underlying array may be read-only. */
	char *ptr;
	/** Size of the string in bytes. */
	size_t size;
} clod_sstr;

/** Create a sized string from char array and size. The string retains ptr_v. */
#define clod_sstr(ptr_v, size_v) ((clod_sstr){ .ptr = (char*)(ptr_v), .size = (size_v) })
/** Null string value. */
#define CLOD_SSTR_NULL ((clod_sstr){ .ptr = nullptr, .size = 0 })
/** C string to sized string. The string retains c_string. */
#define CLOD_SSTR_C(c_string) ((clod_sstr){ .ptr = (char*)(c_string), .size = strlen(c_string) })

/**
 * Compare two strings.
 * Two strings are considered equal if their sizes are the same,
 * and their pointers are either equal or point to objects that are equal.
 *
 * @param[in] str1 1st string.
 * @param[in] str2 2nd string.
 * @return If the strings are equal.
 */
CLOD_API
bool clod_sstr_eq(clod_sstr str1, clod_sstr str2);

/**
 * Append bytes to a string, incrementing the appended-to string's size to reflect the new size.
 *
 * @param[in,out] str1 String to be appended to.
 * @param[in] str2 String to append.
 */
CLOD_API
void clod_sstr_cat(clod_sstr *str1, clod_sstr str2);

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
clod_sstr clod_sstr_contains(clod_sstr str, clod_sstr elem);

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
clod_sstr clod_sstr_find(clod_sstr str, char elem, ptrdiff_t occurrence);

#endif
