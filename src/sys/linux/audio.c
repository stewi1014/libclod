#include <clod/stream/audio.h>
#include "config.h"
#include "debug.h"

#if CLOD_USE_ALSA
#include <alsa/asoundlib.h>

int clod_audio_read(clod_stream *self, struct clod_string *dst) {
	if (dst->cap <= 0) {
		return CLOD_ERR_INVALID;
	}

	if (dst->len >= dst->cap) {
		return CLOD_ERR_INVALID;
	}

again:
	int64_t n = snd_pcm_readi(((clod_audio*)self)->device, dst->ptr + dst->len, (size_t)(dst->cap - dst->len) / sizeof(float));
	if (n < 0 && n != -EAGAIN) {
		debug(CLOD_DEBUG, "Audio read error: %s", snd_strerror((int)n));
		n = snd_pcm_recover(((clod_audio*)self)->device, (int)n, 0);
		if (n < 0) {
			return -(int)n;
		}

		goto again;
	}

	dst->len += n * (ptrdiff_t)sizeof(float);
	return CLOD_ERR_OK;
}
int clod_audio_write(clod_stream *self, struct clod_string *src) {
	if (src->len < 0) {
		return CLOD_ERR_INVALID;
	}

again:
	int64_t n = snd_pcm_writei(((clod_audio*)self)->device, src->ptr, (size_t)(src->len) / sizeof(float));
	if (n < 0 && n != -EAGAIN) {
		debug(CLOD_DEBUG, "Audio write error: %s", snd_strerror((int)n));
		n = snd_pcm_recover(((clod_audio*)self)->device, (int)n, 0);
		if (n < 0) {
			return -(int)n;
		}

		goto again;
	}

	src->ptr += n * (ptrdiff_t)sizeof(float);
	src->len -= n * (ptrdiff_t)sizeof(float);
	return CLOD_ERR_OK;
}
int clod_audio_close(clod_stream *self) {
	int res = CLOD_ERR_OK;

	if (((clod_audio*)self)->device) {
		snd_pcm_drain(((clod_audio*)self)->device);
		res = snd_pcm_close(((clod_audio*)self)->device);
		if (res != 0) {
			debug(CLOD_DEBUG, "Audio stream close error: %s", snd_strerror(res));
		}
	}

	return res;
}
int clod_stream_audio(clod_audio *audio_out, int flags) {
	if (flags & CLOD_AUDIO_IN && flags & CLOD_AUDIO_OUT) {
		debug(CLOD_DEBUG, "A single audio stream cannot be both input and output.");
		return CLOD_ERR_INVALID;
	}

	int res;
	#define check_error(expr) if((res = (expr)) < 0) {\
		debug(CLOD_DEBUG, "Failed to initialise audio. "#expr" returned %i: %s.", res, snd_strerror(res));\
		goto error;\
	};

	snd_pcm_t *pcm;
	check_error(snd_pcm_open(
		&pcm,
		"default",
		flags & CLOD_AUDIO_IN ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK,
		0
	));

	snd_pcm_hw_params_t *hw_params;
	snd_pcm_hw_params_alloca(&hw_params);
	check_error(snd_pcm_hw_params_any(pcm, hw_params));
	check_error(snd_pcm_hw_params_set_access(pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED));
	check_error(snd_pcm_hw_params_set_format(pcm, hw_params, SND_PCM_FORMAT_FLOAT));
	check_error(snd_pcm_hw_params_set_channels(pcm, hw_params, 1));
	check_error(snd_pcm_hw_params_set_rate(pcm, hw_params, 48000, 0));

	check_error(snd_pcm_hw_params(pcm, hw_params));
	check_error(snd_pcm_prepare(pcm));

	if (flags & CLOD_AUDIO_IN) {
		audio_out->device = pcm;
		audio_out->stream.read = clod_audio_read;
		audio_out->stream.write = nullptr;
		audio_out->stream.close = clod_audio_close;
	} else {
		audio_out->device = pcm;
		audio_out->stream.read = nullptr;
		audio_out->stream.write = clod_audio_write;
		audio_out->stream.close = clod_audio_close;
	}
	return 0;

	#undef check_error
error:
	if (pcm) {
		snd_pcm_close(pcm);
	}
	return -res;
}
#else
int clod_stream_audio(clod_audio *, int) {
	return CLOD_ERR_INVALID;
}
#endif
