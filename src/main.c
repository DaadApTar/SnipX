#include "options.h"
#include "recorder.h"
#include <bits/pthreadtypes.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <pulse/simple.h>
#include <pulse/pulseaudio.h>
#include <X11/X.h>
#include <X11/extensions/Xinerama.h>
#include "ffmpeg.h"

#define ERROR(x) do {                                 \
                   fprintf(stderr, "ERROR: %s\n", x); \
                   exit(1);                           \
                 } while(0)
#define ERROR_ERNO ERROR(strerror(errno))

#define DEFAULT_PORT 4226
#define SOCKET_BUFFER_SIZE 2
#define STOP_COMMAND {0xFA, 0xDE}
#define STOP_COMMAND_SIZE 2

int main(int argc, char **argv) {
  char* program = *(argv++);
  options *opts = parse_flags(argv, argc-1);

  if (opts == 0) {
    print_usage(program);
    return 1;
  }

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    ERROR_ERNO;
  }

  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port   = htons(DEFAULT_PORT),
    .sin_addr   = INADDR_ANY
  };
  socklen_t addrlen = sizeof(addr);

  uint8_t stop_command[] = STOP_COMMAND;

  // Brake is highest priority task
  if (opts->brake) {
    if(connect(socket_fd, (struct sockaddr *)&addr, addrlen) < 0) {
      ERROR_ERNO;
    }
    int bytes_sent = write(socket_fd, stop_command, STOP_COMMAND_SIZE);
    if (bytes_sent < 0) {
      ERROR_ERNO;
    }
    return 0;
  }

  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
    ERROR_ERNO;
  }

  if(bind(socket_fd, (struct sockaddr *)&addr, addrlen) < 0) {
    ERROR_ERNO;
  }

  if (listen(socket_fd, 10) < 0) {
    ERROR_ERNO;
  }

  bool is_stopped = false;
  uint8_t socket_buffer[SOCKET_BUFFER_SIZE] = {0};

  pthread_t video_thread, audio_thread;
  pthread_mutex_init(&lock, 0);
  atomic_store(&running_flag, 1);

  // Initialise ring buffers.

  circular_array_init(&video_ring_buffer, opts->fps * 10, sizeof(XImage));
  circular_array_init(&audio_ring_buffer, opts->fps * 10, SNIPX_PA_AUDIO_BYTES_PER_FRAME(opts->fps));

  // Initialise pulseaudio
  static const pa_sample_spec sample_spec = {
    .format   = PA_SAMPLE_S16LE,
    .rate     = SNIPX_PA_SAMPLE_RATE,
    .channels = SNIPX_PA_CHANNELS
  };
  pa_simple *simple = 0;
  int error;

  if ((simple = pa_simple_new(0, "snipx", PA_STREAM_RECORD,
                              0, "record", &sample_spec, 0, 0, &error)) == 0) {
    fprintf(stderr, "Cannot create PulseAudio connection: %s.\n", pa_strerror(error));
    close(socket_fd);
    return 1;
  }

  // Initialise X11

  Display *display = XOpenDisplay(0);

  // Initialise Xinerama
  
  int minor, major;
  if (!XineramaQueryExtension(display, &minor, &major)) {
    fprintf(stderr, "Xinerama is not supported.\n");
    close(socket_fd);
    return 1;
  }
  if (!XineramaIsActive(display)) {
    fprintf(stderr, "Xinerama is not active.\n");
    close(socket_fd);
    return 1;
  }

  int num_screens = 0;
  XineramaScreenInfo *screens = XineramaQueryScreens(display, &num_screens);
  Window root = DefaultRootWindow(display);
  if (opts->screen_number > num_screens - 1) {
    fprintf(stderr, "Invalid screen number.\n");
    close(socket_fd);
    return 1;
  }
  short screen_x = screens[opts->screen_number].x_org;
  short screen_y = screens[opts->screen_number].y_org;
  short screen_width = screens[opts->screen_number].width;
  short screen_height = screens[opts->screen_number].height;

  video_capturing_params video_params = {
    .display = display,
    .window = root,
    .screen_x = screen_x,
    .screen_y = screen_y,
    .screen_width = screen_width,
    .screen_height = screen_height,
    .framerate = opts->fps
  };

  audio_capturing_params audio_params = {
    .monitor = opts->sound_monitor,
    .simple = simple,
    .framerate = opts->fps,
  };

  pthread_create(&video_thread, 0, thread_video_capturing, (void *)&video_params);
  pthread_create(&audio_thread, 0, thread_audio_capturing, (void *)&audio_params);

  int client_fd;
  while (!is_stopped) {
    if((client_fd = accept(socket_fd, (struct sockaddr *)&addr, &addrlen)) < 0) {
      ERROR_ERNO;
    }
    if (read(client_fd, socket_buffer, SOCKET_BUFFER_SIZE) < 0) {
      ERROR_ERNO;
    }
    if (memcmp(socket_buffer, stop_command, SOCKET_BUFFER_SIZE) == 0) {
      is_stopped = true;
      atomic_store(&running_flag, 0);
      pthread_join(video_thread, 0);
      pthread_join(audio_thread, 0);
      pa_simple_free(simple);

      // ffmpeg

      ffmpeg *sound = ffmpeg_init_sound("output.aac");

      uint8_t *audio;
      int audio_start = atomic_load(&audio_frame_counter) - opts->fps * 10;
      if (audio_start < 0) audio_start = 0;
      for (int i = audio_start; i < atomic_load(&audio_frame_counter); ++i) {
        audio = circular_array_get(&audio_ring_buffer, i);
        ffmpeg_push_frame(sound, audio, SNIPX_PA_AUDIO_BYTES_PER_FRAME(opts->fps));
      }
      ffmpeg_close(sound);

      XImage *frame;
      ffmpeg *video = ffmpeg_init_video("output.aac", "output.mp4", screen_width, screen_height, opts->fps);
      int video_start = atomic_load(&video_frame_counter) - opts->fps * 10;
      if (video_start < 0) video_start = 0;
      for (int i = video_start; i < atomic_load(&video_frame_counter); ++i) {
        frame = circular_array_get(&video_ring_buffer, i);
        ffmpeg_push_frame(video, frame->data, sizeof(uint32_t) * screen_width * screen_height);
      }
      ffmpeg_close(video);
      
    }
    memset(socket_buffer, 0, SOCKET_BUFFER_SIZE);
    close(client_fd);
  }

  pthread_mutex_destroy(&lock);
  close(socket_fd);

  return 0;
}
