/**
 * @file clod/db.h
 * @defgroup database Clod Database
 * @{
 *
 * The Clod DB is a key->value storage database optimised for large values.
 *
 * Originally inspired by a want to improve Minecraft region files, the clod db is an entirely novel
 * database utilising B-Tree indexing and shadow paging to provide new features such as data integrity.
 *
 * It has a compatability mode which comes with a performance and feature tradeoff that
 * is backwards compatible with vanilla region files. If a vanilla region file implementation
 * writes to a Clod DB file, all data will be lost except the vanilla chunk key-value pairs.
 */
#ifndef CLOD_DB_H
#define CLOD_DB_H


/** @} */
#endif