@page region_format Region File Format
@ingroup region
# Region file format
Libclod supports 3 region file header variants.
The vanilla header, the libclod header, and a backwards compatible combination of both.
The vanilla header is only supported for reading with reduced functionality,
due to limitations with the format itself, e.g. no data validation or crash recovery.
The libclod header is not backwards compatible.
The compound header is backwards compatible and includes both headers.

## Vanilla
The vanilla header does not have any dynamic storage capability.
It is a fixed size header and chunk data begins directly after.

### Header
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
The libclod region file format uses NBTs to store header data;
as such, the static header is smaller, although intentionally oversized,
and only stores some metadata and other things that need static storage.

The static header is padded to 256 bytes, leaving some space for any future extensions
that might need static storage such as shared mutexes.

All CRC-32 values use the polynomial `0x04C11DB7`, reflect input and output,
use `0xFFFFFFFF` as the initial value, and xor with `0xFFFFFFFF` to finalise.
AFAICT this is the most common CRC-32 variant, so an implementation should always be close at hand.

### Header
| Offset | Size | Type                                                              |
|--------|------|-------------------------------------------------------------------|
| 0      | 128  | Human readable magic used to identify the file                    |
| 128    | 4    | CRC-32 checksum of NBT data                                       |
| 132    | 4    | Size of NBT data in bytes                                         |
| 136    | 4    | Generation number incremented at the start and end of every write |
| 256    | ...  | NBT data                                                          |
| ...    | ...  | Chunk data                                                        |

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
When opening a compound header, libclod will ignore existing values and set them to the following.
If the implementation uses different values for these, then backwards compatibility is broken,
and the libclod header should be used instead.

| Key                | Value |
|--------------------|-------|
| ChunkFilePrefix    | "c"   |
| ChunkFileExtension | "mcc" |
| Dimensions         | 2     |
| SectorSize         | 4096  |
