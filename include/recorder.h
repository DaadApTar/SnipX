#ifndef RECORDER_H_
#define RECORDER_H_

#include <X11/X.h>
#include <X11/Xlib.h>
#include <bits/pthreadtypes.h>
#include <pulse/stream.h>
#include <stdatomic.h>
#include "log.h"

#include "circular_array.h"

#define FRAME_NS(framerate) (long)((double)1/(double)framerate*1e9)

#define SNIPX_PA_SAMPLE_RATE 44100
#define SNIPX_PA_CHANNELS 2
#define SNIPX_PA_BYTES_PER_SAMPLE 2
#define SNIPX_PA_AUDIO_BYTES_PER_FRAME(framerate) ((SNIPX_PA_SAMPLE_RATE * SNIPX_PA_CHANNELS * SNIPX_PA_BYTES_PER_SAMPLE) / framerate)

#define STREAMS_CAPACITY 8

typedef struct {
  Display *display;
  Window window;
  XImage *shared_image;
  int screen_x;
  int screen_y;
  int framerate;
  logger *logger;
  circular_array *ring_buffer;
} video_capturing_params;

/** @brief PulseAudio stream info for reading.
 *  @note stream and buffer_index are set inside read callback.
 */
typedef struct {
  unsigned int index;
  const char *name;
  circular_array ring_buffer;
  pa_stream *stream;
  size_t buffer_index;
} audio_stream;

typedef struct {
  size_t fragsize;
  audio_stream *streams[STREAMS_CAPACITY];
  size_t length;
  size_t capacity;
} audio_capture;

extern atomic_bool     running_flag;
extern pthread_mutex_t lock;
extern atomic_int      video_frame_counter;

/** @brief Thread for capturing video.
 *  @param[in] arg #video_capturing_params.
 */
void *thread_video_capturing(void *arg);

#endif
