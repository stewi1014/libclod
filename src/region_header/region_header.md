@page region_header Region File Header Format
@ingroup region_header

# Region Header Format
Libclod supports 3 region file header variants.
The vanilla header, the libclod header, and a backwards compatible combination of both.
The vanilla header is only supported for reading with reduced functionality,
due to limitations with the format itself, e.g. no data validation or crash recovery.
The libclod header is not backwards compatible.
The compound header is backwards compatible and includes both headers.

## Vanilla
The vanilla header does not have any dynamic storage capability.
It is a fixed size header and chunk data begins directly after.

| Offset | Size | Description                                    |
|--------|------|------------------------------------------------|
| 0      | 4096 | Chunk locations [1024]                         |
| 4096   | 4096 | Modification time in unix epoch seconds [1024] |
| 8192   | ...  | Chunk data                                     |

#### Chunk location
| Offset | Size | Type              |
|--------|------|-------------------|
| 0      | 3    | Offset in sectors |
| 3      | 1    | Size in sectors   |

#### Chunk data
| Offset | Size | Type                           |
|--------|------|--------------------------------|
| 0      | 4    | Compressed chunk size in bytes |
| 4      | 1    | Chunk compression type         |
| 5      | ...  | Compressed chunk data          |

## Libclod


| Offset | Size | Name        | Description                                                      |
|--------|------|-------------|------------------------------------------------------------------|
| 0      | 16   | Magic       | Bytes that uniquely identify the libclod format                  |
| 16     | ...  | Unused      |                                                                  |
| 32     | 4    | Generation  | Generation number used to ensure data coherency (little endian)  |
| 40     | 4    |             | Intentionaly unused so generation is also a valid 64-bit integer |
| 44     | 4    | Unused      | Reserved for future expansion                                    |


| Offset | Size | Name                           | Description                                     |
|--------|------|--------------------------------|-------------------------------------------------|
| 0      | 4096 | Chunk Locations Table 1 [1024] | Location of each chunk in the region file       |
| 4096   | 4096 | Chunk MTime Table 1 [1024]     | Modification times in unix epoch seconds        |
| 8192   | 128  | Libclod Magic                  | Bytes that uniquely identify the libclod format |
| 8320   | 4    | Sector size                    | Size of sectors in this region file             |
| 8324   | 4    | Dimensions                     | Number of dimensions in this region file        |
| 8328   | 8    | Generation                     | Generation number used to ensure data coherency |
| ...    | ...  |                                | Reserved for future use                         |
| 8448   | 30   | Chunk Filename Prefix          | Filename prefix that chunk files have           |
| 8478   | 10   | Chunk Filename Extension       | Filename extension that chunk files have        |
| ...    | ...  |                                | Reserved for future use                         |
| 12288  | 4096 | Chunk CRC-32 Table 1 [1024]    | CRC-32 of each chunk's data                     |
| 16384  | 4096 | Chunk Locations Table 2 [1024] | Location of each chunk in the region file       |
| 20480  | 4096 | Chunk CRC-32 Table 2 [1024]    | CRC-32 of each chunk's data                     |


| Offset | Size | Name                 | Description                                     |
|--------|------|----------------------|-------------------------------------------------|
| 0      | 16   | Magic                | Unique identifier for the file format           |
| 16     | 8    | Generation           | Generation number used to ensure data coherency |
| 24     | 104  | Unused               | Reserved for future expansion                   |
| 128    | 1    | Chunk Table Flags 1  | Flags                                           |
| 129    | 3    | Chunk Table CRC-24 1 | CRC-24 checksum of chunk table 1                |
| 132    | 4    | Chunk Table Offset 1 | Offset in bytes of the 1st chunk table          |
| 136    | 1    | Chunk Table Flags 2  | Flags                                           |
| 137    | 3    | Chunk Table CRC-24 2 | CRC-24 checksum of chunk table 2                |
| 140    | 4    | Chunk Table Offset 2 | Offset in bytes of the 2nd chunk table          |
| 144    | 880  | Unused               | Reserved for future expansion                   |
| 1024   | 3072 | Chunk CRC-24 [1024]  | CRC-24 checksum of each chunk's data            |



| Offset | Size | Name   | Description                                            |
|--------|------|--------|--------------------------------------------------------|
| 0      | 4    | Size   | Size in bytes of the chunk                             |
| 0      | 4    | CRC-32 | CRC-32 checksum of the section's data                  |
| 8      | 5    | Offset | Offset in bytes of the section in the file             |
| 16     | 6    | Size   | Size in bytes of the section                           |
| 24     | 8    | MTime  | Last modification time in nanoseconds since unix epoch |



The libclod region file format uses NBTs to store header data;
as such, the static header is smaller, although intentionally oversized,
and only stores some metadata and other things that need static storage.

The static header is padded to 256 bytes, leaving some space for any future extensions
that might need static storage such as shared mutexes.

Data coherency through concurrent usage is implemented using a generation number,
which is incremented at the start and end of every write made to the header.
As such, the state of the lowest bit indicates if a write is currently in progress.
Reads can ensure data coherency by checking that no write is in progress before reading
and checking that the generation did not change during the read.
Writes **must** ensure that no other write is in progress before starting,
and always increment the generation before and after the write.

Libclod's `region.h` library attempts to ensure data coherency through system crashes by
backing up the header before the first write and restoring it on recovery.
The backup header is kept for as long as changes are backwards compatible with the backup header.
When changes incompatible with the backup header are made, the changes are written to a new file
before atomically replacing the old file and deleting the backup header.

All CRC-32 values use the polynomial `0x04C11DB7`, reflect input and output,
use `0xFFFFFFFF` as the initial value, and xor with `0xFFFFFFFF` to finalise.
AFAICT this is the most common CRC-32 variant, so an implementation should always be close at hand.

| Offset | Size | Type                                            |
|--------|------|-------------------------------------------------|
| 0      | 128  | Human readable magic used to identify the file  |
| 128    | 4    | Generation number used to ensure data coherency |
| 132    | 4    | CRC-32 checksum of NBT data                     |
| 136    | 4    | Size of NBT data in bytes                       |
| 140    | 4    | Flags                                           |
| 256    | ...  | NBT data                                        |
| ...    | ...  | Chunk data                                      |

#### Flags
| Bit | Description                           |
|-----|---------------------------------------|
| 0   | The file has been atomically replaced |

### NBT data
The idea behind the NBT structure is that implementations can store whatever they need to.
Data not relevant to a given implementation is simply ignored by it,
and there's backwards compatible extensibility for new features.

There are a couple limitations to NBT data, most of which stem from the fact that
writes can cause other tags to move memory locations.
Since dynamic storage is the goal, the alternative to copying is some kind of
memory allocation scheme that will have to deal with issues like fragmentation,
backwards compatability and require a complex structure to organise it all.
By the end of all that you probably won't even be any faster than NBT's dumb copying anyways.
Modern CPUs copy at tens of GB/s on a bad day. You'll just have an overengineered
file format for the sake of being able to cache pointers a little easier.

Besides, when it comes to dynamic storage formats, we already have a reliable and well-understood
in-domain format. It doesn't make sense to use something else without a concrete reason.

#### Root Tag
| Key                | Type     | Description                                 |
|--------------------|----------|---------------------------------------------|
| ChunkFilePrefix    | String   | Prefix chunk filenames have (e.g. "c")      |
| ChunkFileExtension | String   | Extension chunk filenames have (e.g. "mcc") |
| Dimensions         | Byte     | Number of dimensions in the region file     |
| SectorSize         | Int      | Sector size                                 |
| Chunks             | Compound | Chunk metadata                              |

#### Chunks Tag
| Key              | Type              | Description                                        |
|------------------|-------------------|----------------------------------------------------|
| ModificationTime | Int Array [1024]  | Chunk last modification time in unix epoch seconds |
| FileOffset       | Int Array [1024]  | Location of the chunk data in the file             |
| FileSectors      | Byte Array [1024] | Chunk size in sectors                              |
| Checksum         | Int Array [1024]  | Checksum of chunk data                             |
| UncompressedSize | Int Array [1024]  | Chunk size in bytes                                |

## Compound
The compound format aims to provide backwards compatibility with the vanilla format.
It is simply both the vanilla and libclod header concatenated together,
with the _Chunk/ModificationTime_, _Chunk/FileOffset_ and _Chunk/FileSectors_ arrays missing
from the libclod header and chunks being stored at a larger offset to make space.

| Offset | Size | Type           | Description        |
|--------|------|----------------|--------------------|
| 0      | 8192 | Vanilla header | The vanilla header |
| 8192   | ...  | Libclod header | The libclod header |

Implementations that don't support the libclod format will see a perfectly valid and correct
vanilla header and function normally; however, it's likely they will overwrite the libclod header
with chunk data when writing. Libclod will see a failing magic or checksum and fall back to treating the file
as a pure vanilla region file, and information in the libclod header is lost.

Notably, to facilitate backwards compatibility, some attributes must be fixed.
When opening a compound header, libclod will ignore existing values and use the following.
If the implementation uses different values for these, then backwards compatibility is broken,
and the libclod header should be used instead.

| Key                | Value |
|--------------------|-------|
| ChunkFilePrefix    | "c"   |
| ChunkFileExtension | "mcc" |
| Dimensions         | 2     |
| SectorSize         | 4096  |
