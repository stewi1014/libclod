# libclod memory allocator

## Tree
The allocator uses two trees.

### Free tree
The free tree is address indexed.

The free tree contains all free block ranges.
It is the meat of the allocator and provides primary allocation.

### Used tree
The used tree is address indexed.

The used tree contains all slabs and spans that are currently in use.
It provides the primary, albeit slow, path for allocating slab-sized memory, where the tree is searched
for a slab of the correct size that has free space.

## Slab Free list


## Block
The backing memory is split into blocks of 256 bytes.
Blocks can be either unused, a node and part of the tree metadata,
a slab containing an array of allocation elements, or part of a larger span
of allocated memory.

### Slab
A slab is a single block in size and contains a fixed number of allocation elements.

## Internal Pointer

The backing memory of the allocator is intentionally limited to 2^32 bytes.
Pointers within it are then also aligned to 2^8 = 256 byte blocks.
This means that any block in the backing memory can be addressed with only 32 - 8 = 24 bits.
Then, the remaining 8 bits are used to store pointer metadata.

Often, 32bits of metadata are stored alongside a pointer.

| Bit | Size | Description                    |
|-----|------|--------------------------------|
| 0   | 24   | Block offset in backing memory |
| 24  | 8    | Metadata                       |

| Metadata   | Ptr Type | Description                                                      |
|------------|----------|------------------------------------------------------------------|
| 0b00000001 | Node     | Pointer to block containing a node                               |
| 0b00000010 | Span     | Pointer to block at the start of a span                          |
| 0bSSSS0011 | Slab     | Pointer to block containing a slab. S = Size class of slab       |
| 0bBBBBB100 | Branch   | Pointer to branch within a node. B = index of branch within node |

## Slabs

| Size Class | Type       | Elem Count   | Elem Size |
|------------|------------|--------------|-----------|
| 0          | Split slab | 28 * 8 = 224 | 1         |
| 1          | Split slab | 30 * 4 = 120 | 2         |
| 2          | Split slab | 31 * 2 = 62  | 4         |
| 3          | Slab       | 32           | 8         |
| 4          | Slab       | 32           | 12 + 4    |
| 5          | Slab       | 16           | 16        |
| 6          | Slab       | 16           | 24 + 8    |
| 7          | Slab       | 8            | 32        |
| 8          | Slab       | 8            | 48 + 16   |
| 9          | Slab       | 4            | 64        |
| 10         | Slab       | 4            | 96 + 32   |
| 11         | Slab       | 2            | 128       |
| 12         | Slab       | 2            | 192 + 64  |

### Split slab
Split slabs store used flags in the memory they point to instead of the pointer metadata.
The pointer metadata then instead represents if a group is completely used or if one or
more elements in the group are free.

| Elem Size | Num Groups | Num Elems | Used Flags Bytes |
|-----------|------------|-----------|------------------|
| 1         | 28         | 224       | 32               |
| 2         | 30         | 120       | 16               |
| 4         | 31         | 62        | 8                |
