@page region_format Region File Format

# Region File Format
Libclod implements a novel region file format designed to provide data integrity and concurrency.
It remains backwards compatible with vanilla implementations. Methods are provided for interacting
with the format directly, in addition to a complete storage system, however, the format is documented
here for my own rubber ducking, reference and for any other potential implementations.

## Prior Reading
 - https://docs.kernel.org/locking/seqlock.html Same approach to locking that the libclod format uses.
 - https://www.akkadia.org/drepper/futex.pdf Futex overview, OS API used for inter-process synchronization.
 - https://man7.org/linux/man-pages/man2/futex.2.html Linux's futex.
 - https://man.freebsd.org/cgi/man.cgi?query=_umtx_op FreeBSD's futex.
 - https://man.openbsd.org/futex.2 OpenBSD's futex.
 - https://developer.apple.com/documentation/os/os_sync_wait_on_address macOS's futex.
 - https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress Half-arsed Windows version
of a futex that doesn't support IPC, but could still prove useful.

## Structure
| Offset | Size | Name                     | Description                                                |
|--------|------|--------------------------|------------------------------------------------------------|
| 0      | 4096 | Shadow Table [1024]      | Shadow table of chunk locations in the region file.        |
| 4096   | 4096 | MTime Table [1024]       | Modification times in unix epoch seconds.                  |
| 8192   | 128  | Magic                    | Bytes that uniquely identify the file format.              |
| 8320   | 31   | Chunk Filename Prefix    | Filename prefix that chunk files have. Null terminated.    |
| 8351   | 11   | Chunk Filename Extension | Filename extension that chunk files have. Null terminated. |
| 8364   | 4    | Sector Size              | Size of a sector. Ideally a multiple of system page size.  |
| 8368   | 4    | File Lock                | Lock protecting file operations such as resizing.          |
| 8372   | 4    | File Size                | File size in sectors.                                      |
| 8376   | 4    | Checkpoint Table CRC-32  | CRC-32 of the checkpoint table.                            |
| 8380   | 4    | Shadowed Sectors         | Approximation of how much duplicate chunk data exists.     |
| ...    | ...  |                          | Reserved for future use.                                   |
| 9216   | 3072 | Chunk Checksums [1024]   | CRC-24 checksum of chunks in the shadow table.             |
| 12288  | 4096 | Chunk Locks [1024]       | Locks used to protect each chunk.                          |
| 16384  | 4096 | Checkpoint Table [1024]  | Checkpoint table of chunk locations in the region file.    |
| 20480  | ...  | Chunk Data               | Sparse chunk data                                          |

### Chunk Location
Chunk locations are stored in the shadow table and checkpoint table.

| Offset | Size | Type              |
|--------|------|-------------------|
| 0      | 3    | Offset in sectors |
| 3      | 1    | Size in sectors   |

### Chunk Data
| Offset | Size | Type                           |
|--------|------|--------------------------------|
| 0      | 4    | Compressed chunk size in bytes |
| 4      | 1    | Chunk compression type         |
| 5      | ...  | Compressed chunk data          |

# Reading
The format supports true read-only capability, and as such, readers must be able to operate without modifying
anything, including locks. This means that readers may observe locks instead of holding them.
Observing a lock is the action of waiting for it to be free, performing the operation, and then confirming the
read data is consistent. If file corruption never happened, then simply confirming that the write lock was not
acquired by another thread during the read is enough. For readers that do not care about data integrity or can
confirm that the file has been made consistent since the last possible crash, this is enough. For robust readers
that may be reading a file that is not consistent, the checksum should be used.

In addition, readers that have write access and wish to mitigate the possibility of needing to retry the read due
to being interrupted by a writer can acquire a read lock, excluding writers until the read is complete.

# Writing
Writes must never overwrite chunk data referenced by the checkpoint table or any other chunks in the shadow table.
Put another way, the writer may only overwrite chunk data if it is the chunk it is writing to and the chunk is shadowed.
The easiest approach to this is to never overwrite anything and simply append to the end of the file using the file
operation lock. The writer must also ensure that the chunk's CRC is correct.

Secondly, writes must correctly respond to a dead lock holder.
If the writer detects that the previous lock holder is dead, it must start the recovery process or abort the
write operation. That being said, the recovery process for a failed write to a chunk is to restore the chunk
from the last checkpoint, and the write method is about to immediately overwrite said chunk anyway. As such,
a write method may simply confirm that the chunk it's writing to is the only thing that needs restoring,
and if so, ignore recovery entirely in favour of its own new data.

While appending to the end of the file is the most straightforward approach, it eventually leads
to blown-out file sizes far larger than what is needed to actually store the data the file contains.
One approach is compaction, which is described below. Another approach is for writers to opportunistically
search for free space and use it. However, this must be done carefully as another write operation,
potentially from a different implementation, might be trying to use the same space.

libclod's approach is to find free space and forwardly declare it as the chunk's location in the shadow table
before confirming that no other chunk is using the same space. If the chunk's location is clashing,
it gives up and appends instead.
1. Make a note of all locks to ensure sequential memory ordering for subsequent operations.
2. Acquire the lock for the chunk to be written to.
3. Search in a pseudo-random order through all chunk locations in the shadow and
   checkpoint table to find the best-fitting free space.
4. Forward place the found space in the shadow table.
5. Increment our chunk's lock counter.
6. Check all chunk locks for acquisitions during the search and confirm that no chunks are using the same space.
7. Use the space.

# File Operations
Some file operations would have race conditions if not otherwise managed. A global file operation lock exists
to manage this.

## File Creation
To address file initialisation race conditions, the OS is relied on to ensure exclusive file creation.
On unix-based systems this is the O_EXCL flag. The method which exclusively created the file is then
responsible for initialising the file with magic and other constants before setting the file lock to
a non-zero value. Conversely, methods which open an existing file must observe the file lock for a
non-zero value before reading any constants.

## File resizing
To facilitate the atomic allocation of new space at the end of the file, a file growth lock is used.
This lock is used to protect resizing of the file during concurrent usage. To prevent the potential
for deadlock, no chunk locks can be acquired while the file resize lock is held. The lock has the same
form as other locks.

# Maintenance Operations
Maintenance operations, defined as operations which hold all chunk locks, must operate in a manner that always
leaves either the checkpoint table or shadow table in a valid state. In addition, they must ensure the distinction
of which one is valid can be made through the use of the checkpoint table's checksum. If the checkpoint
table is valid, then the validity of chunks in the shadow table must follow the usual rules of a dead
lock holder indicating potential invalidity of that chunk. If the checkpoint table is not valid,
this must be indicated through a failing checksum, and it's imperative that the shadow table was left
in a valid state.

## Checkpointing
The checkpoint table is a copy of the shadow table at some point in the past with known valid contents.
It is used to restore a chunk if a write to a chunk fails. However, the checkpoint table needs to be written
during checkpointing, which can potentially leave it in an invalid state. As such, there is a brief period of
time when the semantics of the shadow and checkpoint tables must be inverted. All operations that modify
the shadow table must respect the possibility for this inversion. Conversely, the checkpointing operation
must facilitate the detection of this inversion and recovery by ensuring that a crash is detectable by any
future operation, and that a valid table exists and can be determined. Ensuring detection of a crash
is achieved by locking all chunk locks, such that all future writers will detect a dead lock holder.
The validity of the checkpoint table can then be determined using its checksum, with the implication that
a failing checksum indicates that the shadow table is valid, which the checkpoint operation must ensure.

A source of corruption that needs considering is torn flushes to the disk. A torn flush may cause a
shadowed chunk that is committed to the disk to be invalid.
TODO: address this better in this documentation.

So, a typical checkpoint operation would be:
1. Acquire all chunk locks, making a note of any which have dead lock holders.
2. Make the shadow table valid. **If the checkpoint table checksum is consistent**, revert any chunks
with dead lock holders to the last checkpoint.
3. Sync the entire file to disk. After this, all shadowed chunk data is valid.
4. Copy the shadow table to the checkpoint table and update the checkpoint table checksum.
5. Release all chunk locks.

## Compaction
Of course, the requirement to not overwrite chunk data referenced by the checkpoint table means that
the file inevitably grows beyond the actual size required to store the data it contains.
In addition, writer implementations may append to the file even if space exists, which will cause the
file to grow indefinitely until it exhausts the addressable sector range (2^24 sectors). To mitigate this,
some additional method must be employed. One such approach is a dedicated compaction operation.

There are various approaches to compaction with different tradeoffs, but it's worth noting libclod's approach.
Libclod's compaction process is perfect; it leaves no unused space at the expense of performance,
and as such is used sparingly such as on file close.
1. Acquire all chunk locks.
2. Get the size of the largest chunk in the file N.
3. Copy all chunks that exist in the (N * 64)-sized space at the beginning of the file and append them to the end of the file.
4. Perform a checkpoint.
5. Copy as many chunks into the created empty space, taking the first chunks as they appear following the free space.
6. Perform a second checkpoint to free the space from the previously copied chunks.
7. Repeat from step 5 until all chunks have been copied.
8. Perform a final checkpoint.
9. Truncate the file to the actual used size.
10. Sync the file to disk.
11. Release all chunk locks.

# Recovery
Recovery takes two forms, depending on the nature of the failure. If any number of chunk writes have failed,
then the recovery process is to simply restore the chunks from the last checkpoint. However, if a maintenance
operation has failed, then the checkpoint table may not be valid. The contract maintenance operations have
ensures that all locks will show a dead lock holder, and at least one table is valid. In either case, the
need for recovery is initiated by the detection of a dead lock holder.

If the checkpoint table checksum is inconsistent, then the shadow table must be valid, and recovery involves
copying the shadow table to the checkpoint table and updating the checkpoint table crc. This is the same as a
checkpoint operation and is idempotent. As such, recovery from a failed checkpoint operation is to simply retry
the same checkpoint operation again.

If the checkpoint table checksum is consistent, then we know the checkpoint table is valid. The state of each
lock then indicates the validity of its chunk in the shadow table, and invalid chunks can be restored from the
last checkpoint.

# Locking
So, this seems a bit... insane. Why is such an overgrown locking system necessary?
I keep coming back to this question – because the truth is this doesn't sit right with me.
Yet every time I try to approach this problem with a simpler solution, I find myself hitting a wall
with the same old problems. How do you detect a dead lock? How do you ensure a read call made after a
write doesn't return stale data? How do you avoid writer starvation? How do you stop readers from
very-busy waiting? How do you exclude all users during a checkpoint? How do you recover from a dead lock?
How do you support a read-only mode? How do you do all of this in a way that works robustly across multiple
processes? If a file is written on one platform, the writer fails, and the file is then moved to a different
platform, how do you detect the dead lock left by the previous platform to initiate recovery?

The core reason why this lock implementation is so unwieldy is because it's trying to do a lot with very little.
Still, why not implement a single global read-write lock instead of having one for each chunk?
Well, if that were done instead, there would be two options for writers. Either hold the write lock while
compressing and writing to the file, or only hold the lock while writing to the file. In the former case,
the ability to use multiple cores for compression is thrown out the window. In the latter case, reads made
while the writer is compressing will return stale data, and conditions protected by the lock are only assessable
after compression is completed. For example, if the write method is a read-modify-write operation, then it
needs to discard all the compressed data it just spent a lot of CPU compressing, and start again.

Perhaps I'm just making excuses for feature-creep. Still, when compared to other lock implementations,
with similar functionality, this approach is still quite simple - even for locking methods that don't
support multiple readers or dead lock recovery. That being said, designing synchronisation primitives is
a seriously tough topic. Here be dragons.

The locks used here are a kind of extension on the seqlock approach and provide two types of read-lock,
a mechanism for avoiding writer starvation, efficient acquisition of multiple locks at once, and a second
sequence number for detecting and distinguishing dead read-lock holders from dead write-lock holders.
This is achieved at the expense of a small~ish limit on the number of read locks available,
no mitigations for the thundering herd problem, and a rather dumb approach to detecting dead lock holders.
All together, the entire array of locks can be thought of as a three-tiered global-writer-reader lock,
where each tier excludes the ones below it.

In addition to facilitating concurrent usage, the locks also serve as an important mechanism for detecting
failed operations as part of ensuring data integrity and initiating the recovery process. Dead lock detection
is achieved by observing sequence numbers, and when no progress on the lock is made over a timeout period,
the lock is considered dead. The dumb nature of this approach is mitigated by the ability for lock holders
to indefinitely refresh their locks, allowing locks to be held for arbitrarily long periods of time without
false-positive dead lock detection.

## Structure
| Bit Offset | Size | Name           | Description                                        |
|------------|------|----------------|----------------------------------------------------|
| 0          | 1    | Blocked Flag   | Prevents aquiring the lock, even if it's unlocked. |
| 1          | 1    | Write Lock     | Lock which only one thread may hold at a time.     |
| 2          | 5    | Read Locks     | Count of read-lock holders.                        |
| 7          | 3    | Read Sequence  | Sequence number incremented by read-lock holders.  |
| 10         | 22   | Write Sequence | Sequence number incremented by write-lock holders. |

## Lock Requirements
Each part of the file has strict rules on what locks must be held or observed before access to ensure the
consistency of read data and the integrity of the file. The only exception is for a reader that does not
care about data integrity. In this documentation it is assumed that readers do care about data integrity.

This table is a rough guide for what locks are required to interact with different format elements.
It is not comprehensive. For example, the detection of a dead lock holder comes with a new set of
constraints as it's possible that the semantics of the shadow and checkpoint tables are reversed.

| Name                     | Read data/Acquire read lock | Write data/acquire write lock |
|--------------------------|-----------------------------|-------------------------------|
| Shadow Table [1024]      | Chunk read lock             | Chunk write lock              |
| MTime Table [1024]       | Chunk read lock             | Chunk write lock              |
| Magic                    | Always                      | Exclusive file creation       |
| Chunk Filename Prefix    | Always                      | Exclusive file creation       |
| Chunk Filename Extension | Always                      | Exclusive file creation       |
| Sector Size              | Always                      | Exclusive file creation       |
| File Lock                | Always                      | Always                        |
| File Size                | File read lock              | File write lock               |
| Checkpoint Table CRC-32  | Any chunk read lock         | All chunk write locks         |
| Shadowed Sectors         | Atomic                      | Atomic                        |
| Chunk Checksums [1024]   | Chunk read lock             | Chunk write lock              |
| Chunk Locks [1024]       | Cannot hold file lock       | Cannot hold file lock         |
| Checkpoint Table [1024]  | Any chunk read lock         | All chunk write locks         |
