#include "pa_callbacks.h"
#include "circular_array.h"
#include "recorder.h"

circular_array audio_ring_buffer;
size_t buffer_index;

void read_cb(pa_stream *s, size_t nbytes, void *userdata) {
  audio_capturing_params *params = (audio_capturing_params *)userdata;
  pa_usec_t usec_t;
  int res = pa_stream_get_time(params->stream, &usec_t); if (res == 0) {
    //printf("%.3f\n", usec_t / 1000000.0);
  } else {
    //printf("Error: %s\n", pa_strerror(pa_context_errno(pa_stream_get_context(stream))));
  }

  const void *data;

  pa_stream_peek(s, &data, &nbytes);

  if (data && nbytes > 0) {
    circular_array_push(&audio_ring_buffer, (void *)data, buffer_index);
  }

  pa_stream_drop(s);
  buffer_index++;
}

void context_state_cb(pa_context *c, void *userdata) {
  audio_capturing_params *params = (audio_capturing_params *)userdata;
  if (pa_context_get_state(c) != PA_CONTEXT_READY) return;

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

  params->stream = pa_stream_new(c, "record", &ss, NULL);
  pa_stream_set_read_callback(params->stream, read_cb, userdata);

  pa_stream_connect_record(params->stream, NULL, &attr, PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_ADJUST_LATENCY);
}

void reset_audio_buffer_index() {
  buffer_index = 0;
}
