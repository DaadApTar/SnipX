#include "pa_callbacks.h"
#include "circular_array.h"
#include "recorder.h"

void read_cb(pa_stream *s, size_t nbytes, void *userdata) {
  stream_info *info = (stream_info *)userdata;
  /*
    NOTE: This block was intended to store samples timestamp.
    Whenever I encounter a video&audio desynchronization, I
    will uncomment and use this.

  pa_usec_t usec_t;
  int res = pa_stream_get_time(info->stream, &usec_t);
  if (res == 0) {
    printf("%.3f\n", usec_t / 1000000.0);
  } else {
    log_print(info->logger, LOG_ERROR, "Error: %s\n", pa_strerror(pa_context_errno(pa_stream_get_context(info->stream))));
  }
  */

  const void *data;

  pa_stream_peek(s, &data, &nbytes);

  if (data && nbytes > 0) {
    circular_array_push(&info->ring_buffer, (void *)data, info->buffer_index);
  }

  pa_stream_drop(s);
  info->buffer_index++;
}

void context_state_cb(pa_context *c, void *userdata) {
  audio_capturing_params *params = (audio_capturing_params *)userdata;
  if (pa_context_get_state(c) != PA_CONTEXT_READY) return;
  if (params->desktop_stream->stream || params->mic_stream->stream) return;

  pa_sample_spec ss = {
    .format = PA_SAMPLE_S16LE,
    .rate = 44100,
    .channels = 2
  };

  pa_buffer_attr attr = {
    .maxlength = (uint32_t) -1,
    .fragsize = params->fragsize,
    .tlength = (uint32_t) -1,
    .minreq = (uint32_t) -1,
    .prebuf = (uint32_t) -1,
  };

  // Desktop stream
  params->desktop_stream->stream = pa_stream_new(c, params->desktop_stream->name, &ss, NULL);
  pa_stream_set_read_callback(params->desktop_stream->stream, read_cb, (void *)params->desktop_stream);

  pa_stream_connect_record(params->desktop_stream->stream, params->desktop_stream->monitor, &attr, PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_ADJUST_LATENCY);

  // Mic stream
   params->mic_stream->stream = pa_stream_new(c, params->mic_stream->name, &ss, NULL);
   pa_stream_set_read_callback(params->mic_stream->stream, read_cb, (void *)params->mic_stream);

   pa_stream_connect_record(params->mic_stream->stream, params->mic_stream->monitor, &attr, PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_ADJUST_LATENCY);
}
