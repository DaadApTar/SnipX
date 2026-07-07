#ifndef PA_CALLBACKS_H_
#define PA_CALLBACKS_H_

#define SOURCES_INFO_CAPACITY 256

#include <pulse/pulseaudio.h>
#include "recorder.h"

typedef enum {
  MODE_LIST_SOURCES,
  MODE_RECORD
} snipx_pa_mode;

typedef enum {
  RESULT_WAIT,
  RESULT_OK,
  RESULT_ERR,
} snipx_pa_state_result;

typedef struct {
  snipx_pa_mode mode;
  audio_capturing_params recording_params;
  snipx_pa_state_result result;
  pa_threaded_mainloop *ml;
} snipx_pa_state;

/** @brief Read samples callback. */
void read_cb(pa_stream *, size_t, void *);

/** @brief Change context state callback.
 *  @param state must be a pointer to #snipx_pa_state.
 */
void context_state_cb(pa_context*, void* state);

/** @brief Prints available sources. */
void source_info_list_cb(pa_context *, const pa_source_info *, int, void *);

#endif
