#include "pulseaudio.h"

int prepare_pulseaudio(snipx_pulseaudio *pa, snipx_pa_state state) {
  pa->state = state;

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

  return 0;
}

void stop_pulseaudio(snipx_pulseaudio *pa) {
  pa_threaded_mainloop_stop(pa->ml);
}

void free_pa(snipx_pulseaudio *pa) {
  for (size_t i = 0; i < pa->state.capture.length; ++i) {
      pa_stream_disconnect(pa->state.capture.streams[i]->stream);
      pa_stream_unref(pa->state.capture.streams[i]->stream);
      pa->state.capture.streams[i]->stream = NULL;
  }

  // Free PA context.
  pa_context_disconnect(pa->context);
  pa_context_unref(pa->context);
  pa->context = NULL;

  // Free PA mainloop
  pa_threaded_mainloop_free(pa->ml);
  pa->ml = NULL;
}
