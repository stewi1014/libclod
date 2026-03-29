#ifndef LIBCLOD_AUDIO_H
#define LIBCLOD_AUDIO_H

#include <clod/lib.h>
#include <clod/memory.h>
#include <clod/stream.h>

#define CLOD_AUDIO_OUT 1
#define CLOD_AUDIO_IN 2
#define CLOD_AUDIO_NONBLOCK 4

/**
 * Open the default input or output audio device.
 * Audio format uses a 32-bit float for each sample.
 *
 * @param[out] stream_out New audio stream.
 * @param[in] flags Audio open flags.
 * @param[in] allocator Memory allocator.
 * @return 0 on success, non-zero on error.
 */
CLOD_API CLOD_NONNULL(1, 3)
int clod_audio(clod_stream **stream_out, int flags, clod_allocator *allocator);

#endif
