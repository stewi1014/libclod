#ifndef LIBCLOD_KEEPALIVE_H
#define LIBCLOD_KEEPALIVE_H

#include <clod/thread.h>

/// Keeps a read lock alive until the process exists.
void clod_rwseq_rd_keepalive_start(int *ptr);

/// Finish keeping a read lock alive.
void clod_rwseq_rd_keepalive_end(int *ptr);

/// Keeps a write lock alive until the process exists.
void clod_rwseq_wr_keepalive_start(int *ptr);

/// Finish keeping a write lock alive.
void clod_rwseq_wr_keepalive_end(int *ptr);

/// Keepalive main method.
int keepalive_main(int argc, char **argv);

extern struct {
	int count;
	int **read_locks;
	int **write_locks;
} keepalive_locks;

#endif