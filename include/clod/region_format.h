/**
 * @file clod/region_format.h
 * @ingroup region
 * @defgroup region_format Region Format
 * @{
 *
 * Methods for reading and writing to region files.
 * None of the methods here are thread safe; for concurrent usage each
 * thread must independently create its own handle to the same region file.
 */
#ifndef LIBCLOD_REGION_FORMAT_H
#define LIBCLOD_REGION_FORMAT_H

#include <clod/lib.h>
#include <stddef.h>
#include <stdint.h>


/**
 * Method provided by the user that is used to synchronise the file to disk.
 * This method does not need external synchronisation as it is only called
 * with the protection of the file lock.
 *
 * @param[in] size Size of the section of the file from the start to synchronise.
 * @param[in] user The provided user pointer.
 * @return True if the operation was successful, false if not.
 */
typedef bool clod_rfmt_file_sync(size_t size, void *user);

struct clod_rfmt;
struct clod_rfmt_opts;

/**
 * Result of an operation.
 */
enum clod_rfmt_result {
	/** No worries. */
	CLOD_RFMT_OK = 0,
	/** The provided timeout was reached. */
	CLOD_RFMT_TIMEOUT = 2,
	/** Invalid parameters. */
	CLOD_RFMT_INVALID = 4,
	/** The library successfully stopped an invalid operation that may have caused data loss.
	 * Some checks to prevent such misuse are put behind NDEBUG, and as such won't detect the misuse,
	 * and potentially cause data loss, in release builds. The return of this result indicates a
	 * critical bug in the library user. */
	CLOD_RFMT_OWN_MISUSE = 5,
	/** Someone else is interacting with the region file in a way which causes corruption and permanent
	 * data loss, or there is a critical bug in this library. The return of this error likely means
	 * data loss has already occurred. The implementation that caused this error needs to be fixed. */
	CLOD_RFMT_OTHER_MISUSE = 6,
	/** The provided \p file_manage method indicated an error. */
	CLOD_RFMT_FILE_MANAGE_ERROR = 7,
	/** The provided \p file_sync method indicated an error. */
	CLOD_RFMT_FILE_SYNC_ERROR = 8
};

/**
 * Configuration options for the region format.
 */
struct clod_rfmt_opts {
	/** The amount of time that must pass with no progress being made before
	 * a lock's owner is considered dead. A zero value uses defaults. */
	uint32_t dead_lock_timeout_ms;

	/**
	 * Method provided by the user that is used to get, query, and resize the region file.
	 * Multiple optional arguments may be provided in the same call.
	 *
	 * @param[in] data (nullable) If non-null, the method shall return a pointer to the complete file contents.
	 * @param[in] size (nullable) If non-null, the method shall return the size of the file.
	 * @param[in] new_size (nullable) If non-null, the method shall resize the file to the provided size.
	 * @param[in] user The provided user pointer.
	 * @return True if the operation was successful, false if not.
	 */
	bool (*file_manage)(uint8_t **data, size_t *size, const size_t *new_size, void *user);

	/**
	 * Method provided by the user that is used to synchronise the file to disk.
	 * This method does not need external synchronisation as it is only called
	 * with the protection of the file lock.
	 *
	 * @param[in] size Size of the section of the file from the start to synchronise.
	 * @param[in] user The provided user pointer.
	 * @return True if the operation was successful, false if not.
	 */
	bool (*file_sync)(size_t size, void *user);

	/** Custom memory allocation function. Nullable.
	 * This is not used for any buffers, this library doesn't buffer anything. */
	void *(*malloc_func)(size_t size, void *user);

	/** Custom memory free function. Nullable. */
	void (*free_func)(void *ptr, void *user);

	/** User value passed to callbacks. Anything. */
	void *user;
};

/**
 * Create a new region file in read/write mode.
 *
 * @param[in] opts Configuration options for the opened file.
 * @param[in] chunk_filename_prefix Filename prefix for chunk files.
 * @param[in] chunk_filename_extension Filename extension for chunk files.
 * @param[in] sector_size Size fo sectors in the file. Ideally a multiple of system page size.
 * @return Handle to the file.
 */
CLOD_API CLOD_NONNULL(1, 2, 3)
struct clod_rfmt *clod_rfmt_init_new(
	struct clod_rfmt_opts *opts,
	char *chunk_filename_prefix,
	char *chunk_filename_extension,
	uint32_t sector_size
);

/**
 * Initialise a region file for read/write interaction.
 *
 * @param[in] opts Configuration options for the opened file.
 * @return Handle to the region file.
 */
CLOD_API CLOD_NONNULL(1)
struct clod_rfmt *clod_rfmt_init_rw(struct clod_rfmt_opts *opts);

/**
 * Initialise a region file for read-only interaction.
 *
 * @param[in] opts Configuration options for the opened file.
 * @return Handle to the region file.
 */
CLOD_API CLOD_NONNULL(1)
struct clod_rfmt *clod_rfmt_init_ro(struct clod_rfmt_opts *opts);

#endif
