#include "pulseaudio.h"

int prepare_pulseaudio(snipx_pulseaudio *pa, logger *logger, snipx_pa_state state) {
  pa->state = state;
  pa->logger = logger;

  pa->ml = pa_threaded_mainloop_new();
  if (!pa->ml) {
    return -1;
  }

  pa->state.ml = pa->ml;

  return 0;
}

int start_pulseaudio(snipx_pulseaudio *pa) {
  pa_threaded_mainloop_lock(pa->ml);
  pa_threaded_mainloop_start(pa->ml);

  pa_mainloop_api *pa_api = pa_threaded_mainloop_get_api(pa->ml);

  pa->context = pa_context_new(pa_api, "SnipX");

  pa_context_set_state_callback(pa->context, context_state_cb, (void*)&pa->state);
  if (pa_context_connect(pa->context, NULL, 0, NULL) < 0) {
    return -1;
  }

  while (pa->state.result != RESULT_OK) {
    pa_threaded_mainloop_wait(pa->ml);
  }

  pa_threaded_mainloop_unlock(pa->ml);

  audio_stream **streams = pa->state.capture.streams;
  size_t length = pa->state.capture.length;

  for (size_t i = 0; i < length; ++i) {
    if (streams[i]->stream == 0) log_print(pa->logger, LOG_WARNING, "Source %d is not available.\n", streams[i]->index);
  }

  return 0;
}

void stop_pulseaudio(snipx_pulseaudio *pa) {
  pa_threaded_mainloop_lock(pa->ml);
}

void proceed_pulseaudio(snipx_pulseaudio *pa) {
  for (size_t i = 0; i < pa->state.capture.length; ++i) {
    if (pa->state.capture.streams[i]->stream == 0) continue;
    pa_operation *op = pa_stream_flush(pa->state.capture.streams[i]->stream, flush_cb, (void *)pa->ml);
    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) pa_threaded_mainloop_wait(pa->ml);
    pa_operation_unref(op);
  }
  pa_threaded_mainloop_unlock(pa->ml);
}

void free_pa(snipx_pulseaudio *pa) {
  if (pa->ml) {
    pa_threaded_mainloop_lock(pa->ml);
  }
  for (size_t i = 0; i < pa->state.capture.length; ++i) {
    pa_stream *s = pa->state.capture.streams[i]->stream;
    if (s) {
      pa_stream_set_state_callback(s, NULL, NULL);
      pa_stream_set_read_callback(s, NULL, NULL);
      pa_stream_disconnect(s);
      pa_stream_unref(s);
      pa->state.capture.streams[i]->stream = NULL;
    }
  }

  // Free PA context.

  if (pa->context) {
    pa_context_set_state_callback(pa->context, NULL, NULL);
    pa_context_disconnect(pa->context);
    pa_context_unref(pa->context);
    pa->context = NULL;
  }

  // Free PA mainloop
  if (pa->ml) {
    pa_threaded_mainloop_unlock(pa->ml);
    pa_threaded_mainloop_free(pa->ml);
    pa->ml = NULL;
  }
}
