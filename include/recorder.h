#ifndef RECORDER_H_
#define RECORDER_H_

#include <X11/X.h>
#include <X11/Xlib.h>
#include <bits/pthreadtypes.h>
#include <pulse/simple.h>
#include <stdatomic.h>

#include "circular_array.h"

#define FRAME_NS(framerate) (long)((double)1/(double)framerate*1e9)

#define SNIPX_PA_SAMPLE_RATE 44100
#define SNIPX_PA_CHANNELS 2
#define SNIPX_PA_BYTES_PER_SAMPLE 2
#define SNIPX_PA_AUDIO_BYTES_PER_FRAME(framerate) ((SNIPX_PA_SAMPLE_RATE * SNIPX_PA_CHANNELS * SNIPX_PA_BYTES_PER_SAMPLE) / framerate)

typedef struct {
  Display *display;
  Window window;
  int screen_x;
  int screen_y;
  int screen_width;
  int screen_height;
  int framerate;
} video_capturing_params;

typedef struct {
  pa_simple *simple;
  char *monitor;
  int framerate;
} audio_capturing_params;

extern circular_array *video_ring_buffer;
extern circular_array *audio_ring_buffer;
extern atomic_bool     running_flag;
extern pthread_mutex_t lock;
extern atomic_int      video_frame_counter, audio_frame_counter;

/** @brief Thread for capturing video.
 *  @param[in] arg #video_capturing_params.
 */
void *thread_video_capturing(void *arg);

/** @brief Thread for capturing audio.
 *  @param[in] arg #audio_capturing_params.
 */
void *thread_audio_capturing(void *arg);

/** @todo: implement later.
 *  @brief encodes video and audio.
 */
void *thread_nvenc_encoding(void *arg);

#endif
