#ifndef LIBCLOD_AUDIO_H
#define LIBCLOD_AUDIO_H

#include <clod/lib.h>
#include <clod/stream.h>

#define CLOD_AUDIO_OUT 1
#define CLOD_AUDIO_IN 2

#define CLOD_AUDIO_CHANNELS 0x100
#define CLOD_AUDIO_1_CHANNELS (CLOD_AUDIO_CHANNELS * 1)
#define CLOD_AUDIO_2_CHANNELS (CLOD_AUDIO_CHANNELS * 2)

/**
 * Open the default input or output audio device.
 * Audio format uses a 32-bit float for each sample.
 *
 * @param[out] stream_out New audio stream.
 * @param[in] flags Audio open flags.
 * @return 0 on success, non-zero on error.
 */
CLOD_API CLOD_NONNULL(1)
int clod_stream_audio(clod_stream *stream_out, int flags);

#endif
