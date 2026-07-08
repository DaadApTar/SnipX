#include "pa_callbacks.h"
#include "circular_array.h"
#include "recorder.h"
#include "string.h"

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
  snipx_pa_state *state = (snipx_pa_state *)userdata;

  switch(pa_context_get_state(c)) {
  case PA_CONTEXT_READY: {
    pa_context_get_source_info_list(c, source_info_list_cb, userdata);
  }; break;
  case PA_CONTEXT_TERMINATED:
  case PA_CONTEXT_FAILED: {
    state->result = RESULT_ERR;
  }; break;
  case PA_CONTEXT_UNCONNECTED:
  case PA_CONTEXT_CONNECTING:
  case PA_CONTEXT_AUTHORIZING:
  case PA_CONTEXT_SETTING_NAME:
    break;
  }
}

void source_info_list_cb(pa_context *c, const pa_source_info *i, int eol, void *userdata) {
  (void) c;
  snipx_pa_state *state = (snipx_pa_state *)userdata;
  if (eol || i == NULL) {
    pa_threaded_mainloop_signal(state->ml, 0);
    state->result = RESULT_OK;
    return;
  };

  switch (state->mode) {
  case MODE_LIST_SOURCES: {
    printf("%-4c%-6d%s\n", ' ', i->index, i->description);
  }; break;
  case MODE_RECORD: {
    for (size_t iterator = 0; iterator < state->recording_params.streams_length && state->recording_params.streams[iterator]; ++iterator) {
      if (state->recording_params.streams[iterator]->stream) continue;
      if (state->recording_params.streams[iterator]->index == i->index) {

        pa_sample_spec ss = {
          .format = PA_SAMPLE_S16LE,
          .rate = 44100,
          .channels = 2
        };
        pa_buffer_attr attr = {
            .maxlength = (uint32_t)-1,
            .fragsize = state->recording_params.fragsize,
            .tlength = (uint32_t)-1,
            .minreq = (uint32_t)-1,
            .prebuf = (uint32_t)-1,

        };
        stream_info *info = state->recording_params.streams[iterator];
        info->stream = pa_stream_new(c, info->name, &ss, NULL);
        pa_stream_set_read_callback(info->stream, read_cb,
                                    (void *)info);

        pa_stream_flags_t flags = PA_STREAM_AUTO_TIMING_UPDATE |
                                  PA_STREAM_INTERPOLATE_TIMING |
                                  PA_STREAM_ADJUST_LATENCY;

        pa_stream_connect_record(info->stream,
                                  i->name, &attr, flags);
        }
    }
  }; break;
  }
}
