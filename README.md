@mainpage
# libclod

Libclod is still a work in progress.

Libclod is licenced under the [GNU Affero General Public License v3.0](./LICENCE.txt) or later.
For alternate licencing contact me.

Libclod is a library aimed at data storage and manipulation. It aims to provide a high-performance
and ergonomic API for the features it supports with minimal dependencies.

#### [Documentation](https://stewi1014.github.io/libclod/)
#### [Source](https://github.com/stewi1014/libclod)

 - [Data Sturctures](#data-structures) A number of data structures useful for storing data in memory or storage.
 - [Compression API](#compression-api) Methods for compressing data with a uniform API across different compression methods.
 - [File IO](#file-io) Streaming methods for file, network and other IO.
 - [Hash Functions](#hash-functions) Streamable hash methods.
 - [Memory Allocator](#memory-allocator) Libclod includes its own custom memory allocator.
 - [Region Storage](#region-storage) Concurrent data storage system with exceptional performance.
 - [Threading API](#threading) Libclod's own custom threading API built from scratch.

## Building

Libclod is built using cmake.
`cmake -B build && cmake --build build -j 32 && ctest build && sudo cmake --install build`

## Features

### Data Structures
#### [NBT](https://stewi1014.github.io/libclod/group__nbt.html)
The NBT format is a depth-first serialised tree structure supporting various data types.
The NBT parser is fast and doesn't use any memory.
It doesn't provide an intermediate data structure; I don't believe the
want for an intermediate data structure is borne out of sound reasoning.
It recursively steps through NBT data at approx 6GB/s on my machine.

#### [Hash Table](https://stewi1014.github.io/libclod/group__table.html)
The hash table has decent performance and uses SipHash by default.
It supports keys and values of any size and custom hash and comparison functions.

#### [Tree](https://stewi1014.github.io/libclod/group__tree.html)
A tree structure based on B-Trees that supports variable node, key and value sizes.

### [Compression API](https://stewi1014.github.io/libclod/group__compression.html)
Libclod wraps some compression libraries to provide a single compress and decompress method
with uniform behaviour across all compression methods it supports.
Most compression methods attempt to be compatible with some existing format.
It is used internally and might be helpful for FFI users who have slow native compression libraries.
Shoutout to libdeflate for being a work of art.

### [File IO](https://stewi1014.github.io/libclod/group__file.html)
Libclod provides a streaming interface, much like libc, but enables users to implement streams as well.
Some things streams support are file IO, network IO and audio playback and recording.

### [Hash Functions](https://stewi1014.github.io/libclod/group__hash.html)
Libclod provides a few high-performance hash functions for use with data.
For CRC checksums, it provides a method to update checksums with arbitrary sections of previous data replaced
by leveraging the math CRCs are built on. I.e. update a checksum of 1GB of data by only rehashing the modified section.

### [Memory Allocator](https://stewi1014.github.io/libclod/group__memory.html)
Libclod's memory allocator is designed to have good performance for general use, and operate
with minimal overhead. It intentionally avoids support for concurrent usage, with the low overhead
instead supporting each thread having its own dedicated memory allocator.

### [Region Storage](https://stewi1014.github.io/libclod/group__region.html)
Libclod's region storage is a high-performance N-dimension position->data storage system that supports
data integrity through program and system crashes and massive concurrent access from both processes and threads.
Multiple processes and threads can access the same storage simultaneously without any additional coordination with
extremely high performance facilitated by custom-built synchronisation primitives that allow access, including
concurrent writes, with almost no blocking at all. In addition, it uses almost no memory except
for data compression buffers, and does not require a dedicated process like other database software.

It borrows its name from Minecraft's region files, as part of the original inspiration for this project was
to re-implement Minecraft's region file format in C. This project and its data structures are far removed
from Minecraft's implementation and are more in line with modern database techniques despite backwards
compatibility with minecraft being supported under specific configurations.

### [Threading API](https://stewi1014.github.io/libclod/group__threading.html)
Libclod provides a threading API.

## Dependencies

All library dependencies are optional!
They can be enabled or disabled at build time,
thereby omitting the features said dependency provided.
The only exception is the C standard library.

That being said, omitting the majority of compression libraries
will make most file formats that this library interacts with unreadable.
Vendoring compression libraries is a future goal.

I reserve the right to use the entire feature set of these dependencies;
using libclod with dependencies that have features intentionally disabled is,
in general, not supported, although many specific cases would be fine.

- libc
- libdeflate `-DUSE_LIBDEFLATE=ON/OFF`
- liblz4 `-DUSE_LIBLZ4=ON/OFF`
- liblzma `-DUSE_LIBLZMA=ON/OFF`
- libzstd `-DUSE_LIBZSTD=ON/OFF`
- libbz2 `-DUSE_LIBBZ2=ON/OFF`
- sqlite3 `-DUSE_SQLITE3=ON/OFF`
- libpq `-DUSE_LIBPQ=ON/OFF`

### Linux
Linux is tested.
Some linux-specific optimisations are used if they are available.

### BSD
BSD variants are untested but expected to work with minimal changes, if any.
They support everything libclod needs, so the only barrier to support
is getting things plugged in properly if they aren't already.

### macOS
macOS is untested but expected to work with minimal changes, if any.
The lowest common denominator is 14.4 where they added a public API for a futex-style feature,
which libclod needs. Before then, projects would use a private and undocumented API for this (yuck!).
Other than that, macOS seems to have a reasonably strong feature set and robust public API.
The only barrier to support is getting things plugged in properly if they aren't already.

### Windows
Unfortunately, this project highlights Windows's weakness as an operating system and platform. Not only does
Windows lack support for the standardised APIs that other operating systems share, requiring maintaining duplicate
code for Windows vs everything else, but functionality ubiquitous among other systems is often missing in Windows.
Due to this lack of functionality, it might not be possible to implement libclod for Windows without significant
feature culling. To top it off, Windows also lacks a modern C compiler or toolchain without 3rd party software.
Libclod is written in modern C.

Libclod is attempting to implement some database-like features, and there's a reason why
robust support for Windows by database software is almost unheard of. While it won't be easy,
and I'm far from finished complaining about it, I'm not willing to write Windows off yet.
Hopefully there's a way to work around most of the problems and get libclod working on Windows.
