@page region_format Region File Format
@ingroup region_format

# Region File Format
Libclod implements a novel region file format designed to provide data integrity and concurrency.
It remains backwards compatible with vanilla implementations. Methods are provided for interacting
with the format directly, in addition to a complete storage system, however, the format is documented
here for my own rubber ducking, reference and for any other potential implementations.

## Prior Reading
 - https://docs.kernel.org/locking/seqlock.html Same approach to locking that the libclod format uses.
 - https://www.akkadia.org/drepper/futex.pdf Futex overview. What libclod uses to block threads on locks.
 - https://man7.org/linux/man-pages/man2/futex.2.html Linux futex.
 - https://developer.apple.com/documentation/os/os_sync_wait_on_address macOS futex.
 - https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress Half-arsed Windows version
of a futex that doesn't work properly, but could still prove useful.

## Structure
| Offset | Size | Name                     | Description                                               |
|--------|------|--------------------------|-----------------------------------------------------------|
| 0      | 4096 | Shadow Table [1024]      | Shadow table of chunk locations in the region file.       |
| 4096   | 4096 | MTime Table [1024]       | Modification times in unix epoch seconds.                 |
| 8192   | 128  | Libclod Magic            | Bytes that uniquely identify the file format.             |
| 8320   | 30   | Chunk Filename Prefix    | Filename prefix that chunk files have.                    |
| 8350   | 10   | Chunk Filename Extension | Filename extension that chunk files have.                 |
| 8360   | 4    | Sector Size              | Size of a sector. Ideally a multiple of system page size. |
| 8364   | 4    | File Resize Lock         | Size of the file and a lock protecting file resizing.     |
| 8368   | 4    |                          | Reserved                                                  |
| 8372   | 4    | Checkpoint Table CRC-32  | CRC-32 of the checkpoint table.                           |
| 8376   | 4    | Shadowed Sectors         | Approximation of how much duplicate chunk data exists.    |
| ...    | ...  |                          | Reserved for future use.                                  |
| 9216   | 3072 |                          | Unused, free for custom implementation data.              |
| 12288  | 4096 | Chunk Locks [1024]       | Locks used to protect each chunk.                         |
| 16384  | 4096 | Checkpoint Table [1024]  | Checkpoint table of chunk locations in the region file.   |

### Chunk Location
| Offset | Size | Type              |
|--------|------|-------------------|
| 0      | 3    | Offset in sectors |
| 3      | 1    | Size in sectors   |

### Chunk Lock
| Bit | Size | Name         | Description                                       |
|-----|------|--------------|---------------------------------------------------|
| 0   | 1    | Blocked Flag | Blocks aquiring of the lock, even if it is free.  |
| 1   | 1    | Wait Acquire | If there is a thread waiting to acquire the lock. |
| 2   | 1    | Locked Flag  | If the lock is locked or not.                     |
| 3   | 29   | Counter      | Counter number.                                   |

### File Resize Lock
| Bit | Size | Name                 | Description                                       |
|-----|------|----------------------|---------------------------------------------------|
| 0   | 1    | Blocked Flag         | Blocks aquiring of the lock, even if it is free.  |
| 1   | 1    | Wait Acquire         | If there is a thread waiting to acquire the lock. |
| 2   | 1    | Locked Flag          | If the lock is locked or not.                     |
| 3   | 29   | File size in sectors | Size of the file in sectors.                      |

### Chunk Data
| Offset | Size | Type                           |
|--------|------|--------------------------------|
| 0      | 4    | Compressed chunk size in bytes |
| 4      | 1    | Chunk compression type         |
| 5      | ...  | Compressed chunk data          |

# Chunk Locking
The format uses a counter-style locking scheme that forms the basis of synchronisation for users of the format
and is a requirement for any program that wishes to write to the format or ensure the consistency of read data.
Acquiring a lock involves exclusively exchanging the locked flag from unset to set and incrementing the counter.
Then, on lock release, the counter is incremented again, the locked flag is cleared, and waiters are woken up.

The detection of a dead lock holder is done through a simple timeout, and the unsavory nature of this approach
is mitigated by the ability for lock holders to refresh their locks by incrementing the counter. The period of
this lock refreshing may vary depending on the implementation, but a duration of ~100 milliseconds is probably
the longest one would want to hold a lock for without refreshing it. A timeout of 5 seconds before declaring
the lock holder dead gives a reasonable buffer for slow IO operations without causing too much disruption.

Each part of the file has strict rules on what locks must be held or observed before access to ensure the
consistency of read data and the integrity of the file. The only exception is for a reader that does not
care about data integrity. In this documentation it is assumed that readers do care about data integrity.

This table is not a comprehensive description of when operations are or are not allowed.
For example, the detection of a dead lock holder comes with a new set of constraints as it's possible
that the semantics of the shadow and checkpoint tables are reversed.

| Name                     | No Lock           | Observe One Lock    | One Lock                  | All Locks         |
|--------------------------|-------------------|---------------------|---------------------------|-------------------|
| Shadow Table [1024]      | None              | Read specific chunk | Read/Write specific chunk | Read/Write        |
| MTime Table [1024]       | None              | Read specific chunk | Read/Write specific chunk | Read/Write        |
| Libclod Magic            | Read Only         | Read Only           | Read Only                 | Read Only         |
| Chunk Filename Prefix    | Read Only         | Read Only           | Read Only                 | Read Only         |
| Chunk Filename Extension | Read Only         | Read Only           | Read Only                 | Read Only         |
| Sector Size              | Read Only         | Read Only           | Read Only                 | Read Only         |
| File Resize Lock         | None              | None                | Lockable                  | Lockable          |
| Checkpoint Table CRC-32  | None              | Read Only           | Read Only                 | Read/Write        |
| Shadowed Count           | Atomic            | Atomic              | Atomic                    | Atomic            |
| Chunk Locks [1024]       | Atomic Read/Write | Atomic Read/Write   | Atomic Read/Write         | Atomic Read/Write |
| Checkpoint Table [1024]  | None              | Read Only           | Read Only                 | Read/Write        |

# Reading
The format supports true read-only capability, and as such, readers must operate without modifying anything,
including locks. This, in addition to the want for simplicity, means that readers can't exclude writers.
Instead, readers perform their operation while observing the lock to ensure that read data is consistent.
When a reader observes a change to the lock, i.e. an acquisition during the read, it has a couple of options.
If the chunk data comes with its own checksum, the data can be independently verified and no retry is needed.
If the read operation is some kind of background scaper, the operation could be cancelled altogether
and retried on the next pass. Or, if stale data is acceptable, the reader could use the checkpoint table,
which only requires that at least one lock was not held during the read.

# Writing
Writes must never overwrite chunk data referenced by the checkpoint table or other chunks in the shadow table.
Put another way, the writer may only overwrite chunk data if it is the chunk it is writing to and the chunk is shadowed.
The easiest approach to this is to simply append to the end of the file. The writer must also ensure that all
potential avenues for chunk invalidity result in the chunk's lock appearing to have a dead writer. I.e.
a writer is not allowed to abort partway through a write operation and leave the chunk in an invalid state.
In that case it must either revert the chunk to a previous valid state or leave the lock locked to indicate failure,
which will cause other conforming implementations to revert the chunk to the last checkpoint.

Secondly, writes must correctly use their chunk's lock and respond to a dead lock holder.
If the writer detects that the previous lock holder is dead, it must start the recovery process or abort the
write operation. That being said, the recovery process for a failed write to a chunk is to get the chunk from the
last checkpoint and restore it to the shadow table, which the write method is about to immediately overwrite anyway.
As such, a writer can check if the necessary recovery is simply restoring the chunk it's writing to, and if so,
continue as normal.

While appending to the end of the file is the most straightforward approach, it eventually leads
to blow out file sizes far larger than what is needed to actually store the data the file contains.
One approach is compaction, which is described below. Another approach is for writers to opportunistically
search for free space and use it. However, this must be done carefully as another write operation,
potentially from a different implementation, might be trying to use the same space.

libclod's approach is to find free space and forwardly declare it as the chunk's location in the shadow table
before confirming that no other chunk is using the same space. If the chunk's location is clashing,
it gives up and appends instead.
1. Make a note of all locks.
2. Acquire the lock for the chunk to be written to.
3. Search in a pseudo-random order through all chunk locations in the shadow and
   checkpoint table to find the best-fitting free space.
4. Forward place the found space in the shadow table.
5. Increment our chunk's lock counter.
6. For every chunk that is locked or whose counter changed, check its location in the shadow
   table doesn't point to the same space. If it does, discard the space and append instead.
7. Use the space.

# File Resize
To facilitate the atomic allocation of new space at the end of the file, a file growth lock is used.
This lock is used to protect resizing of the file during concurrent usage. To prevent the potential
for deadlock, no chunk locks can be acquired while the file resize lock is held. The lock internally
stores the size of the file in sectors and can be used with futexes in the same way as the chunk locks.

# Maintenance Operations
Maintenance operations, defined as operations which hold all locks, must operate in a manner that always leaves
either the checkpoint table or shadow table in a valid state. In addition, they must ensure the distinction
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
is achieved by locking all chunk locks, such that any future writer will detect a dead lock holder.
The validity of the checkpoint table can then be determined using its checksum, with the implication that
a failing checksum indicates that the shadow table is valid, which the checkpoint operation must ensure.

So, a typical checkpoint operation would be:
1. Acquire all chunk locks, making a note of any which have dead lock holders.
2. Make the shadow table valid. If the checkpoint table checksum is consistent, revert any chunks
with dead lock holders to the last checkpoint.
3. Sync the entire file to disk. After this, all shadowed chunk data is valid, and we can make the checkpoint.
4. Copy the shadow table to the checkpoint table and update the checkpoint table checksum.
5. Release all chunk locks.

## Compaction
Of course, the requirement to not overwrite chunk data referenced by the checkpoint table means that
the file inevitably grows beyond the actual size required to store the data it contains.
In addition, writer implementations may exclusively append to the file, which will cause the file to
grow indefinitely until it exhausts the addressable sector range (2^24 sectors). To mitigate this,
some additional method must be employed. One such approach is a dedicated compaction operation.

The exact approach to compaction is implementation defined, but it's worth noting libclod's approach.
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
