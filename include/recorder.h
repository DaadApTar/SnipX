#ifndef RECORDER_H_
#define RECORDER_H_

#include <X11/X.h>
#include <X11/Xlib.h>
#include <bits/pthreadtypes.h>
#include <pulse/stream.h>
#include <stdatomic.h>
#include "log.h"

#include "circular_array.h"
#include "dynamic_circular_array.h"
#include "compression.h"

#include <string.h>

#define FRAME_NS(framerate) (long)((double)1/(double)framerate*1e9)

#define SNIPX_PA_SAMPLE_RATE 44100
#define SNIPX_PA_CHANNELS 2
#define SNIPX_PA_BYTES_PER_SAMPLE 2
#define SNIPX_PA_AUDIO_BYTES_PER_FRAME(framerate) ((SNIPX_PA_SAMPLE_RATE * SNIPX_PA_CHANNELS * SNIPX_PA_BYTES_PER_SAMPLE) / framerate)

#define STREAMS_CAPACITY 8

typedef struct {
  int screen_x;
  int screen_y;
  int screen_width;
  int screen_height;
  int framerate;
  size_t framesize;
  decompression_wrapper decompression;
  dynamic_circular_array *ring_buffer;
  compression_context *compression_ctx;
  unsigned int gop_length;
} video_capture;

typedef enum {
  FRAME_KEYFRAME,
  FRAME_PREDICTED,
} frame_type;

typedef struct {
  frame_type type;
  // TODO: timestamp
  uint64_t timestamp;
  uint64_t framesize;
} frame_header;

/** @brief PulseAudio stream info for reading.
 *  @note stream is set inside read callback.
 */
typedef struct {
  unsigned int index;
  const char *name;
  dynamic_circular_array ring_buffer;
  pa_stream *stream;
  size_t fragsize;
} audio_stream;

typedef struct {
  audio_stream *streams[STREAMS_CAPACITY];
  size_t length;
} audio_capture;

extern atomic_bool     video_capturing_running_flag;
extern pthread_mutex_t video_capturing_lock;

/** Sends frame with header as one element.
 *  @param[in] ring_buffer buffer to push element.
 *  @param[in] data frame data.
 *  @param[in] frame_size frame size.
 *  @param[in] header frame header.
 *  @param[in] pack_buffer preallocated buffer for packing frame and header.
 *  @param[in] buffer_size size size of pack buffer. Must be >= header size + frame size.
 *  @return 0 on success, -1 on error.
 */
int push_frame_with_header(dynamic_circular_array *ring_buffer, void *data, size_t frame_size, frame_header header, void *pack_buffer, size_t buffer_size);

/** Gets header of frame.
 *  @param[in] packed_frame pointer to frame.
 *  @return header of given packed frame
 */
frame_header get_frame_header(void *packed_frame);

/** Gets data of frame without header.
 *  @param[in] packed_frame pointer to frame.
 *  @return frame.
 */
void *get_frame_data(void *packed_frame);

/** @brief Thread for capturing video.
 *  @param[in] arg #video_capturing_params.
 */
void *thread_video_capturing(void *arg);

#endif
