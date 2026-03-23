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
#include <stdarg.h>

/**
 * Describes an array of bytes in memory. Often a human-readable string,
 * but may encapsulate other objects.
 *
 * A null string must always be represented by a null \p ptr, and
 * a read-only memory region must always be represented with a zero \p cap.
 */
struct clod_string {
	/// Pointer.
	/// The underlying memory may be read-only.
	char *ptr;

	/// Size of the active section in \p ptr.
	/// If \p cap is nonzero, \p len may never be larger than \p cap.
	ptrdiff_t len;

	/// Total capacity of the memory in \p ptr.
	/// A value of zero indicates the underlying memory is read-only and must not be modified.
	ptrdiff_t cap;
};

/// Null string.
#define CLOD_STRING_NULL ((struct clod_string){0})

/// String literal constant.
#define CLOD_STRING_C(cstr) ((struct clod_string){ .ptr = (char*)(cstr), .len = sizeof((char[]){cstr}) - 1, .cap = 0 })

/// Create a new empty string with the given capacity on the stack.
#define CLOD_STRING_NEW(size) ((struct clod_string){ .ptr = (char[size]){0}, .len = 0, .cap = (size) })

/// Get the string representation of an object.
#define CLOD_STRING_OBJECT(object) ((struct clod_string){ .ptr = (char*)(object), .len = sizeof(*(object)), .cap = sizeof(*(object)) })

/// Make a string from a C string.
CLOD_API
struct clod_string clod_string_from_cstr(const char *cstr);

/**
 * Append a string to another string.
 * The active region in \p dst is grown to include the appended data.
 *
 * @param[in,out] dst String to append to.
 * @param[in] src String to be appended.
 * @return Number of bytes written.
 */
CLOD_API CLOD_NONNULL(1)
size_t clod_string_cat(struct clod_string *dst, struct clod_string src);

/**
 * Insert a string at the beginning of another string.
 *
 * @param[in,out] dst String to prepend to.
 * @param[in] src String to prepend.
 * @return Number of bytes written.
 */
CLOD_API CLOD_NONNULL(1)
size_t clod_string_insert(struct clod_string *dst, struct clod_string src);

/**
 * Compare two strings.
 * If str1 > str2 it returns 1, if str1 == str2 it returns 0, and if str1 < str2 it returns -1;
 *
 * Strings are first compared by their length, longer is larger.
 * Then they are compared bytewise, returning on the first non-equal comparison.
 * Otherwise, they are considered equal and 0 is returned.
 *
 * @param[in] str1 1st string.
 * @param[in] str2 2nd string.
 * @return The result of str1 - str2.
 */
CLOD_API
int clod_string_cmp(struct clod_string str1, struct clod_string str2);

/// Append a single char to the end of the string.
CLOD_API CLOD_NONNULL(1)
void clod_string_put_char(struct clod_string *str, char c);

/// Remove a single char from the start of the string.
CLOD_API CLOD_NONNULL(1)
char clod_string_get_char(struct clod_string *str);

/// Get the first char in the string.
CLOD_API
char clod_string_peek_char(struct clod_string str);

/**
 * Format a string.
 *
 * %[flags][width][.precision]<type>
 *
 * The format specifier differs from printf in a few ways.
 *
 * - Flags; Options for changing formatting.
 * - Width; the minimum size of the number as formatted.
 * - Precision; the maximum size of the number as formatted.
 * - Type; the type of the value passed as argument.
 *
 * Notably there is only one specific type for a given C type,
 * with the role of duplicate type specifiers (i.e. u, x or o in printf) switching formatting
 * instead being played by flags.
 *
 * I.e. printing an unsigned int in hexadecimal is %xu instead of %x,
 * and printing an unsigned int in octal is %ou instead of %o.
 *
 * In addition, sized integer types are supported as type specifiers.
 * For example, an int64_t can be printed with %i64, %xi64, %oi64 for decimal,
 * hexadecimal and octal notation respectively.
 *
 * | Type                 | Specifier |
 * | :------------------- | --------: |
 * | int                  | i         |
 * | unsigned int         | u         |
 * | long                 | l         |
 * | unsigned long        | ul        |
 * | long long            | ll        |
 * | unsigned long long   | ull       |
 * | int32_t              | i32       |
 * | uint32_t             | u32       |
 * | int64_t              | i64       |
 * | uint64_t             | u64       |
 * | size_t               | size      |
 * | ptrdiff_t            | ptrdiff   |
 * | void *               | ptr       |
 * | double               | d         |
 * | struct clod_string * | str       |
 * | C string             | s         |
 *
 * @param[in] dst Where the formatted string is written to.
 * @param[in] fmt Format.
 * @param[in] ... Values to format.
 * @return The number of characters that have been, or would have been,
 * written to \p dst.
 */
CLOD_API
size_t clod_string_format(struct clod_string *dst, struct clod_string fmt, ...);

/// Same as clod_string_format, but takes a va_list argument instead of '...'.
CLOD_API
size_t clod_string_vformat(struct clod_string *dst, struct clod_string fmt, va_list args);

/**
 * Parses values in a string into the provided arguments.
 * Attempts to be as close to a reverse of clod_string_format as possible.
 *
 * @param[in] src Source string to parse.
 * @param[in] fmt Format.
 * @param[in] ... Pointers to the types specified in \p fmt.
 * @return Number of parameters parsed.
 */
CLOD_API
int clod_string_parse(struct clod_string src, struct clod_string fmt, ...);

/// Same as clod_string_parse but takes a va-list argument instead of '...'.
CLOD_API
int clod_string_vparse(struct clod_string src, struct clod_string fmt, va_list args);

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
struct clod_string clod_string_contains(struct clod_string str, struct clod_string elem);

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
struct clod_string clod_string_find(struct clod_string str, char elem, int occurrence);

/**
 * Check if a string has a given prefix, moving the string up to the remaining string
 * following the prefix, if found.
 * @param[in,out] str String to check for and remove prefix from.
 * @param[in] prefix Prefix to check for.
 * @return True if \p str had the given prefix.
 */
CLOD_API
bool clod_string_remove_prefix(struct clod_string *str, struct clod_string prefix);

#define CLOD_STRING_DIGIT_ALPHABET CLOD_STRING_C("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_")
#define CLOD_STRING_DIGIT_ALPHABET_CAPS CLOD_STRING_C("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_")

/// Writes an integer value to the string.
/// The value is truncated if the destination is not large enough.
/// @return The number of characters that have been,
/// or would have been written to \p dst.
CLOD_API
size_t clod_string_put_int(struct clod_string *dst, intmax_t val,
	struct clod_string alphabet, unsigned char base, unsigned char min_digits, unsigned char max_digits);

/// Consumes an integer value in the string with the given base.
/// If no value could be decoded, the string is not incremented.
CLOD_API
intmax_t clod_string_get_int(struct clod_string *str, unsigned char base, struct clod_string alphabet);

/// Writes an unsigned integer value to the string.
/// The value is truncated if the destination is not large enough.
/// @return The number of characters that have been,
/// or would have been written to \p dst.
CLOD_API
size_t clod_string_put_uint(struct clod_string *dst, uintmax_t val,
	struct clod_string alphabet, unsigned char base, unsigned char min_digits, unsigned char max_digits);

/// Consumes an unsigned integer value in the string with the given base.
/// If no value could be decoded, the string is not incremented.
CLOD_API
uintmax_t clod_string_get_uint(struct clod_string *str, unsigned char base, struct clod_string alphabet);

/// Writes a double value to the string.
/// The value is truncated if the destination is not large enough.
/// @return The number of characters that have been,
/// or would have been written to \p dst.
CLOD_API
size_t clod_string_put_double(struct clod_string *dst, double val,
	struct clod_string alphabet, unsigned char base, unsigned char min_digits, unsigned char max_digits);

/// Consumes a double value in the string with the given base.
/// If no value could be decoded, the string is not incremented.
CLOD_API
double clod_string_get_double(struct clod_string *str, unsigned char base, struct clod_string alphabet);

#endif
