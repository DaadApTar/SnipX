#ifndef PA_CALLBACKS_H_
#define PA_CALLBACKS_H_

#include <pulse/pulseaudio.h>
#include "recorder.h"

/** @brief Read samples callback. */
void read_cb(pa_stream *, size_t, void *);

/** @brief Change context state callback. */
void context_state_cb(pa_context*, void*);

/** @brief The function returns a private global variable.
 *  @return pointer to pa_stream.
 */
pa_stream* pa_get_stream();

#endif
