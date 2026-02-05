/**
 * @file clod/region_header.h
 * @ingroup region
 * @defgroup region_header
 * @{
 *
 * Methods for reading and writing region headers.
 * Data integrity during concurrent usage is ensured (except with vanilla headers)
 * with methods that fail fast if any foreign write is underway.
 * A read is started with a call to clod_rhdr_init_read, and ended with a call to clod_rhdr_done.
 * A write is started with a call to clod_rhdr_init_write, and ended with a call to clod_rhdr_done.
 *
 * A simple approach to concurrent access would be to retry indefinitely
 * until all intended operations have completed without any interruption from a foreign write.
 *
 * If this retry approach is relied upon in a heavily concurrent environment,
 * readers will starve and busy wait, reducing available CPU for writers and further reducing performance.
 * That being said, changes to the header should be tiny operations compared to the other associated
 * operations (i.e. reading/writing chunk data). A program which uses this retry method to
 * exclusively protect the header while performing other heavier operations should be fine.
 *
 * @note Data integrity is only guaranteed for libclod and compound headers,
 * and only for the header data provided to them.
 * An example of incorrect usage would be reading a region file's header into a buffer,
 * modifying that buffer with this library, and then writing that buffer back to the file.
 * This will cause other programs that read the region file to read incoherent data,
 * and permanent data loss on interaction with any other writing programs.
 * In that case you'd need to perform your own logic in line with the libclod region header format.
 * The generation number would need to be incremented in the file _before_ reading the file,
 * and then incremented again _after_ writing everything to the file.
 */
#ifndef CLOD_REGION_HEADER_H
#define CLOD_REGION_HEADER_H
#include <stddef.h>
#include <stdint.h>

/**
 * Result of an operation.
 * Most operations can be interrupted by a write operation.
 */
enum clod_rhdr_result {
	/** No worries. */
	CLOD_RHDR_OK = 0,
	/** Data appears malformed. */
	CLOD_RHDR_MALFORMED = 1,
	/** A foreign write interrupted the operation.
	 *
	 * For read methods this means that a write has started since the last call to clod_rhdr_init_read.
	 *
	 * For write methods this is never returned, except for clod_rhdr_init_write,
	 * where it indates a write is already underway.
	 * It is unacceptable to interrupt a write, so other write methods return
	 * CLOD_RHDR_MISUSE if they were interrupted.
	 */
	CLOD_RHDR_WRITE_INTERRUPT = 2,
	/** A write operation was used where writing is not supported. */
	CLOD_RHDR_NOT_WRITABLE = 3,
	/** Invalid parameters. */
	CLOD_RHDR_INVALID = 4,
	/** Someone else is interacting with the region file header in a way which
	 * causes header corruption and permanent data loss.
	 * This library should always protect its users from ever accidentally behaving in this way.
	 * The return of this error likely means header corruption and data loss have already occurred.
	 * The implementation that caused this error needs to be fixed,
	 * and the whole region file likely needs to be restored from backup. */
	CLOD_RHDR_MISUSE = 5
};

/**
 * Region header format.
 */
enum clod_rhdr_type {
	/** The vanilla region header. */
	CLOD_RHDR_VANILLA = 1,
	/** The libclod region header. */
	CLOD_RHDR_LIBCLOD = 2,
	/** Compound header including vanilla and libclod. */
	CLOD_RHDR_COMPOUND = 3
};

/**
 * Detect which header type the data uses.
 *
 * @param[in] data Region data.
 * @param[in] data_size Size of the region data.
 * @return The type of the header,
 * or 0 if it doesn't seem to match any header type.
 */
enum clod_rhdr_type clod_rhdr_detect_type(const char *data, size_t data_size);

/**
 * Region header handle and metadata used by methods.
 */
struct clod_rhdr {
	char *data;
	size_t data_size;
	enum clod_rhdr_type type;
	uint32_t generation;
};

/**
 * Create a new header, and initialise a clod_rhdr struct for writing.
 *
 * If type is vanilla and the existing header appears to be compound,
 * the method will clear the compound header's magic.
 * This is only done if \p data_size is large enough to possibly contain a compound header.
 *
 * @param[in] rhdr Struct to initialise.
 * @param[in] data Region data.
 * @param[in] data_size Size of \p data.
 * @param[in] type Header type to create.
 * @return Result of the operation. CLOD_RHDR_OK or CLOD_RHDR_MALFORMED if data_size is too small.
 */
enum clod_rhdr_result clod_rhdr_init_new(
	struct clod_rhdr *rhdr,
	char *data,
	size_t data_size,
	enum clod_rhdr_type type
);

/**
 * Initialise a clod_rhdr struct with existing data for usage with read methods.
 * If a write is currently being made to the header, it immediately returns CLOD_RHDR_WRITE_INTERRUPT.
 *
 * @param[in] rhdr Struct to initialise.
 * @param[in] data Region data.
 * @param[in] data_size Size of \p data.
 * @return Result of the operation.
 * @throws
 */
enum clod_rhdr_result clod_rhdr_init_read(struct clod_rhdr *rhdr, char *data, size_t data_size);

/**
 * Initialise a clod_rhdr struct with existing data for usage with write methods.
 *
 * @param[in] rhdr Struct to initialise.
 * @param[in] data Region data.
 * @param[in] data_size Size of \p data.
 * @return Result of the operation. CLOD_RHDR_OK, CLOD_RHDR_MALFORMED or CLOD_RHDR_WRITE_INTERRUPT.
 */
enum clod_rhdr_result clod_rhdr_init_write(struct clod_rhdr *rhdr, char *data, size_t data_size);

/**
 * Finalise reads or writes to the header.
 *
 * @param[in] rhdr Struct where reads or writes were made.
 * @return Result of the operation. CLOD_RHDR_OK, CLOD_RHDR_MALFORMED or CLOD_RHDR_WRITE_INTERRUPT.
 * CLOD_RHDR_WRITE_INTERRUPT is returned if a write made without this \p rhdr instance
 * occurred at any point since the last call to clod_rhdr_init_read or clod_rhdr_init_write.
 */
enum clod_rhdr_result clod_rhdr_done(struct clod_rhdr *rhdr);

#endif
