#ifndef PULSEAUDIO_H_
#define PULSEAUDIO_H_

#include <pulse/pulseaudio.h>
#include <pa_callbacks.h>
#include "log.h"

typedef struct {
  snipx_pa_state state;
  pa_threaded_mainloop *ml;
  pa_context *context;
  logger *logger;
} snipx_pulseaudio;

/** @brief prepares pulseaudio context.
 *  @param[out] pa pulseaudio context
 *  @param[in] state state
 *  @return 0 on success, -1 on error.
 */
int prepare_pulseaudio(snipx_pulseaudio *pa, snipx_pa_state state);

/** @brief starts pulseaudio thread.
 *  @param[in] pa pulseaudio context
 */
void start_pulseaudio(snipx_pulseaudio *pa);

/** @brief stop pulseaudio thread.
 *  @param[in] pa pulseaudio context
 */
void stop_pulseaudio(snipx_pulseaudio *pa);

/** @brief frees pulseaudio context.
 *  @param[in] pa pulseaudio context
 */
void free_pa(snipx_pulseaudio *pa);

#endif
