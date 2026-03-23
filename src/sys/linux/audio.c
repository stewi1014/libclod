#include <clod/audio.h>
#include "clod_config.h"
#include "clod/debug.h"

#if CLOD_USE_ALSA
#include <alsa/asoundlib.h>

struct audio_stream {
	clod_stream stream;
	clod_allocator *allocator;
	snd_pcm_t *out;
	snd_pcm_t *in;
};

int clod_audio_read(clod_stream *self, struct clod_string *dst) {
	struct audio_stream *stream = (struct audio_stream*)self;
	if (dst->cap <= 0) return CLOD_STREAM_INVALID;
	if (dst->len >= dst->cap) return CLOD_STREAM_INVALID;

again:
	int64_t n = snd_pcm_readi(stream->in, dst->ptr + dst->len, (size_t)(dst->cap - dst->len) / sizeof(short));
	if (n < 0) {
		debug(CLOD_DEBUG, "Audio read error: %s", snd_strerror((int)n));
		n = snd_pcm_recover(stream->in, (int)n, 0);
		if (n < 0) {
			return -(int)n;
		}

		goto again;
	}

	dst->len += n * (ptrdiff_t)sizeof(short);
	return CLOD_STREAM_OK;
}
int clod_audio_write(clod_stream *self, struct clod_string *src) {
	struct audio_stream *stream = (struct audio_stream*)self;
	if (src->len < 0) return CLOD_STREAM_INVALID;

again:
	int64_t n = snd_pcm_writei(stream->in, src->ptr, (size_t)(src->len) / sizeof(short));
	if (n < 0) {
		debug(CLOD_DEBUG, "Audio read error: %s", snd_strerror((int)n));
		n = snd_pcm_recover(stream->in, (int)n, 0);
		if (n < 0) {
			return -(int)n;
		}

		goto again;
	}

	src->ptr += n * (ptrdiff_t)sizeof(short);
	src->len -= n * (ptrdiff_t)sizeof(short);
	return CLOD_STREAM_OK;
}
int clod_audio_close(clod_stream *self) {
	struct audio_stream *stream = (struct audio_stream*)self;
	int res = CLOD_STREAM_OK;

	if (stream->in) {
		int close_res = snd_pcm_close(stream->in);
		debug(CLOD_DEBUG, "Audio input close error: %s", snd_strerror(close_res));
		if (res == CLOD_STREAM_OK) res = close_res;
	}

	if (stream->out) {
		int drain_res = snd_pcm_drain(stream->out);
		debug(CLOD_DEBUG, "Audio output drain error: %s", snd_strerror(drain_res));
		if (res == CLOD_STREAM_OK) res = drain_res;

		int close_res = snd_pcm_close(stream->out);
		debug(CLOD_DEBUG, "Audio output close error: %s", snd_strerror(close_res));
		if (res == CLOD_STREAM_OK) res = close_res;
	}

	stream->allocator->free(stream->allocator->self, stream);
	return CLOD_STREAM_OK;
}
int clod_audio(clod_stream **stream_out, int flags, clod_allocator *allocator) {
	struct audio_stream *stream = allocator->allocate(allocator->self, sizeof(struct audio_stream));
	if (!stream) {
		debug(CLOD_DEBUG, "Provided allocator returned null.");
		return ENOMEM;
	}

	stream->stream.read = nullptr;
	stream->stream.write = nullptr;
	stream->stream.close = clod_audio_close;
	stream->allocator = allocator;
	stream->in = nullptr;
	stream->out = nullptr;

	int res;
	#define check_error(expr) if((res = (expr))) {\
		debug(CLOD_DEBUG, "Failed to initialise audio. "#expr" returned %i: %s.", res, snd_strerror(res));\
		goto error;\
	};

	if (flags & CLOD_AUDIO_IN) {
		check_error(snd_pcm_open(
			&stream->in,
			"default",
			SND_PCM_STREAM_CAPTURE,
			flags & CLOD_AUDIO_NONBLOCK ? SND_PCM_NONBLOCK : 0)
		);

		snd_pcm_hw_params_t *hw_params;
		snd_pcm_hw_params_alloca(&hw_params);
		check_error(snd_pcm_hw_params_any(stream->in, hw_params));
		check_error(snd_pcm_hw_params_set_access(stream->in, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED));
		check_error(snd_pcm_hw_params_set_format(stream->in, hw_params, SND_PCM_FORMAT_S16_LE));
		check_error(snd_pcm_hw_params_set_channels(stream->in, hw_params, 1));
		check_error(snd_pcm_hw_params_set_rate(stream->in, hw_params, 48000, 0));

		check_error(snd_pcm_hw_params(stream->in, hw_params));
		check_error(snd_pcm_prepare(stream->in));
		stream->stream.read = clod_audio_read;
	}

	if (flags & CLOD_AUDIO_OUT) {
		check_error(snd_pcm_open(
			&stream->out,
			"default",
			SND_PCM_STREAM_PLAYBACK,
			flags & CLOD_AUDIO_NONBLOCK ? SND_PCM_NONBLOCK : 0
		));

		snd_pcm_hw_params_t *hw_params;
		snd_pcm_hw_params_alloca(&hw_params);
		check_error(snd_pcm_hw_params_any(stream->out, hw_params));
		check_error(snd_pcm_hw_params_set_access(stream->out, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED));
		check_error(snd_pcm_hw_params_set_format(stream->out, hw_params, SND_PCM_FORMAT_S16_LE));
		check_error(snd_pcm_hw_params_set_channels(stream->out, hw_params, 1));
		check_error(snd_pcm_hw_params_set_rate(stream->out, hw_params, 48000, 0));
		
		check_error(snd_pcm_hw_params(stream->out, hw_params));
		check_error(snd_pcm_prepare(stream->out));
		stream->stream.write = clod_audio_write;
	}

	*stream_out = &stream->stream;
	return 0;

	#undef check_error
error:
	if (stream->in) snd_pcm_close(stream->in);
	if (stream->out) snd_pcm_close(stream->out);
	allocator->free(allocator->self, stream);
	return -res;
}
#else
int clod_audio(clod_stream **, int, clod_allocator *) {
	return CLOD_STREAM_INVALID;
}
#endif
