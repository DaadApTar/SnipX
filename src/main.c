#include "defaults.h"
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
#include "log.h"
#include <unistd.h>
#include <pthread.h>
#include <pulse/simple.h>
#include <pulse/pulseaudio.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "ffmpeg.h"
#include "directory_manager.h"
#include "defaults.h"
#include "sender.h"
#include "pa_callbacks.h"

#define ERROR(logger, x) do {                         \
    log_print(logger, LOG_ERROR, "%s\n", x);          \
                   exit(1);                           \
                 } while(0)
#define ERROR_ERNO(logger) ERROR(logger, strerror(errno))

#define DEFAULT_PORT 4226
#define SOCKET_BUFFER_SIZE 2
#define STOP_COMMAND {0xFA, 0xDE}
#define STOP_COMMAND_SIZE 2

#define SOUND_FILES_CAPACITY 2

int main(int argc, char **argv) {
  char* program = *(argv++);
  options *opts = parse_flags(argv, argc-1);

  if (opts == 0) {
    print_usage(program);
    return 1;
  }
  if (opts->help) {
    print_usage(program);
    return 0;
  }

  dir_create_if_not_exists(dir_default_or_env(DEFAULT_SNIPX_DIR, ENV_SNIPX_DIR));
  char default_snipx_tmp_dir[256];
  snprintf(default_snipx_tmp_dir, 256, "%s/%s", dir_default_or_env(DEFAULT_SNIPX_DIR, ENV_SNIPX_DIR), DEFAULT_SNIPX_TMP_DIR);

  logger logger;
  log_init(&logger);

  log_print(&logger, LOG_INFO, "Opening socket.\n");
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    ERROR_ERNO(&logger);
  }

  struct sockaddr_in addr = {
    .sin_family        = AF_INET,
    .sin_port          = htons(DEFAULT_PORT),
    .sin_addr.s_addr   = htonl(INADDR_ANY)
  };
  socklen_t addrlen = sizeof(addr);

  uint8_t stop_command[] = STOP_COMMAND;

  // Brake is highest priority task
  if (opts->brake) {
    log_print(&logger, LOG_INFO, "Connecting to process.\n");
    if(connect(socket_fd, (struct sockaddr *)&addr, addrlen) < 0) {
      ERROR_ERNO(&logger);
    }
    int bytes_sent = write(socket_fd, stop_command, STOP_COMMAND_SIZE);
    if (bytes_sent < 0) {
      ERROR_ERNO(&logger);
    }
    log_print(&logger, LOG_INFO, "Exiting.\n");
    log_print(&logger, LOG_INFO, "File saved as %s\n", logger.filename);
    log_close(&logger);
    return 0;
  }

  log_print(&logger, LOG_INFO, "Setting socket to reuse address.\n");
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
    ERROR_ERNO(&logger);
  }

  log_print(&logger, LOG_INFO, "Binding address.\n");
  if(bind(socket_fd, (struct sockaddr *)&addr, addrlen) < 0) {
    ERROR_ERNO(&logger);
  }

  log_print(&logger, LOG_INFO, "Listening socket.\n");
  if (listen(socket_fd, 10) < 0) {
    ERROR_ERNO(&logger);
  }

  bool is_stopped = false;
  uint8_t socket_buffer[SOCKET_BUFFER_SIZE] = {0};

  pthread_t video_thread;
#ifndef DISABLE_SENDER
  pthread_t sender_thread;
#endif
  pthread_mutex_init(&lock, 0);
  atomic_store(&running_flag, 1);


  // Initialise X11

  log_print(&logger, LOG_INFO, "Opening X11 display.\n");
  Display *display = XOpenDisplay(0);

  // Initialise Xinerama
  
  int xinerama_minor, xinerama_major;
  if (!XineramaQueryExtension(display, &xinerama_minor, &xinerama_major)) {
    log_print(&logger, LOG_ERROR, "Xinerama is not supported.\n");
    close(socket_fd);
    return 1;
  }
  if (!XineramaIsActive(display)) {
    log_print(&logger, LOG_ERROR, "Xinerama is not active.\n");
    close(socket_fd);
    return 1;
  }

  log_print(&logger, LOG_INFO, "Xinerama version: %d.%d\n", xinerama_major, xinerama_minor);

  int num_screens = 0;
  log_print(&logger, LOG_INFO, "Getting existing screens.\n");
  XineramaScreenInfo *screens = XineramaQueryScreens(display, &num_screens);
  Window root = DefaultRootWindow(display);
  if (opts->screen_number > num_screens - 1) {
    log_print(&logger, LOG_ERROR, "Invalid screen number.\n");
    close(socket_fd);
    return 1;
  }
  
  XineramaScreenInfo screen_info = screens[opts->screen_number];
  short screen_x = screen_info.x_org;
  short screen_y = screen_info.y_org;
  short screen_width = screen_info.width;
  short screen_height = screen_info.height;
  log_print(&logger, LOG_INFO, "Screen x: %d, y: %d, width: %d, height: %d.\n", screen_x, screen_y, screen_width, screen_height);

  // Initialise Xshm

  int shm_minor, shm_major;
  Bool pixmaps;
  if (!XShmQueryVersion(display, &shm_minor, &shm_major, &pixmaps)) {
    log_print(&logger, LOG_ERROR, "Xshm is not supported.\n");
    close(socket_fd);
    return 1;
  }

  log_print(&logger, LOG_INFO, "Xshm version: %d.%d with shared pixmaps support: %s\n", shm_major, shm_minor, pixmaps ? "ON" : "OFF");

  int depth = DefaultDepth(display, screen_info.screen_number);
  Visual *visual = DefaultVisual(display, screen_info.screen_number);
  XShmSegmentInfo shminfo;
  XImage *shared_image = XShmCreateImage(display, visual, depth, ZPixmap, /*char *data */ 0, &shminfo, screen_width, screen_height);

  if (!shared_image) {
    log_print(&logger, LOG_ERROR, "Cannot create shared image.\n");
    close(socket_fd);
    return 1;
  }

  shminfo.shmid = shmget(IPC_PRIVATE, shared_image->bytes_per_line * shared_image->height, IPC_CREAT|0777);

  if (shminfo.shmid < 0) {
    log_print(&logger, LOG_ERROR, "Cannot get shmid: %s.\n", strerror(errno));
    close(socket_fd);
    return 1;
  }

  shared_image->data = (char *)shmat(shminfo.shmid, 0, 0);
  shminfo.shmaddr = shared_image->data;
  shminfo.readOnly = False;

  if (!XShmAttach(display, &shminfo)) {
    log_print(&logger, LOG_ERROR, "Cannot attach shared memory segment.\n");
    close(socket_fd);
    return 1;
  }
  XSync(display, False);

  video_capturing_params video_params = {
    .display = display,
    .window = root,
    .shared_image = shared_image,
    .screen_x = screen_x,
    .screen_y = screen_y,
    .framerate = opts->fps,
    .logger = &logger,
  };

  // Initialise ring buffers.

  circular_array desktop_audio_ring_buffer;
  circular_array mic_audio_ring_buffer;

  log_print(&logger, LOG_INFO, "Initialising video buffer.\n");
  circular_array_init(&video_ring_buffer, opts->fps * opts->length, sizeof(uint32_t) * screen_width * screen_height);
  log_print(&logger, LOG_INFO, "Initialising audio buffers.\n");
  circular_array_init(&desktop_audio_ring_buffer, opts->fps * opts->length, SNIPX_PA_AUDIO_BYTES_PER_FRAME(opts->fps));
  circular_array_init(&mic_audio_ring_buffer, opts->fps * opts->length, SNIPX_PA_AUDIO_BYTES_PER_FRAME(opts->fps));

  // Initialise pulseaudio
  log_print(&logger, LOG_INFO, "Initialising PulseAudio.\n");

  pa_threaded_mainloop *ml = pa_threaded_mainloop_new();
  pa_mainloop_api *pa_api = pa_threaded_mainloop_get_api(ml);

  size_t desktop_buffer_index = 0;
  size_t mic_buffer_index = 0;

  stream_info desktop_stream = {
      .monitor = opts->desktop_sound_monitor,
      .name = "Desktop Audio",
      .ring_buffer = desktop_audio_ring_buffer,
      .stream = NULL,
      .buffer_index = &desktop_buffer_index,
  };

  stream_info mic_stream = {
      .monitor = opts->mic_sound_monitor,
      .name = "Mic/Aux",
      .ring_buffer = mic_audio_ring_buffer,
      .stream = NULL,
      .buffer_index = &mic_buffer_index,
  };

  audio_capturing_params audio_params = {
      .fragsize = SNIPX_PA_AUDIO_BYTES_PER_FRAME(opts->fps),
      .logger = &logger,
      .desktop_stream = &desktop_stream,
      .mic_stream = &mic_stream,
  };

  pa_context *audio_context = pa_context_new(pa_api, "SnipX");
  pa_context_set_state_callback(audio_context, context_state_cb, (void*)&audio_params);
  pa_context_connect(audio_context, NULL, 0, NULL);

#ifndef DISABLE_SENDER
  char port[8];
  snprintf(port, 8, "%d", opts->port);
  sender_params temp_sender_params = {
    .logger = logger,
    .address = opts->address,
    .path = dir_default_or_env(default_snipx_tmp_dir, ENV_SNIPX_TMP_DIR),
    .port = (char *)malloc(sizeof(char) * 8)
  };
  snprintf(temp_sender_params.port, 8, "%d", opts->port);
#endif

#ifndef DISABLE_SENDER
  log_print(&logger, LOG_INFO, "Creating sender thread.\n");
  pthread_create(&sender_thread, 0, thread_send_temp_files, (void *)&temp_sender_params);
#endif
  log_print(&logger, LOG_INFO, "Creating video thread.\n");
  pthread_create(&video_thread, 0, thread_video_capturing, (void *)&video_params);
  log_print(&logger, LOG_INFO, "Creating audio thread.\n");
  pa_threaded_mainloop_start(ml);
  //pthread_create(&audio_thread, 0, thread_audio_capturing, (void *)&audio_params);

  int client_fd;
  log_print(&logger, LOG_INFO, "Accepting from socket...\n");
  while (!is_stopped) {
    if((client_fd = accept(socket_fd, (struct sockaddr *)&addr, &addrlen)) < 0) {
      ERROR_ERNO(&logger);
    }
    if (read(client_fd, socket_buffer, SOCKET_BUFFER_SIZE) < 0) {
      ERROR_ERNO(&logger);
    }
    log_print(&logger, LOG_INFO, "Received message.\n");
    if (memcmp(socket_buffer, stop_command, SOCKET_BUFFER_SIZE) == 0) {
      log_print(&logger, LOG_INFO, "Stopping recording\n");
      is_stopped = true;
      atomic_store(&running_flag, 0);
      pthread_join(video_thread, 0);
      pa_threaded_mainloop_stop(ml);

      // Free PA streams
      if (audio_params.desktop_stream->stream != NULL) {
        pa_stream_disconnect(audio_params.desktop_stream->stream);
        pa_stream_unref(audio_params.desktop_stream->stream);
        audio_params.desktop_stream->stream = NULL;
      }

      if (audio_params.mic_stream->stream != NULL) {
        pa_stream_disconnect(audio_params.mic_stream->stream);
        pa_stream_unref(audio_params.mic_stream->stream);
        audio_params.mic_stream->stream = NULL;
      }

      // Free PA context.
      pa_context_disconnect(audio_context);
      pa_context_unref(audio_context);
      audio_context = NULL;

      // Free PA mainloop
      pa_threaded_mainloop_free(ml);
      ml = NULL;

      // Create tmp directory.
      char *temp_directory = dir_default_or_env(default_snipx_tmp_dir, ENV_SNIPX_TMP_DIR);
      dir_create_if_not_exists(temp_directory);
      
      // ffmpeg

      uint8_t *audio;

      // Desktop sound file
      char desktop_sound_filename[256];
      snprintf(desktop_sound_filename, 256, "%s/desktop.aac", temp_directory);

      ffmpeg *desktop_sound = ffmpeg_init_sound(desktop_sound_filename);

      size_t audio_start;
      if (*audio_params.desktop_stream->buffer_index < (size_t)(opts->fps * opts->length)) audio_start = 0;
      else audio_start = *audio_params.desktop_stream->buffer_index - opts->fps * opts->length;

      log_print(&logger, LOG_INFO, "Flushing sound into %s\n", desktop_sound_filename);
      for (size_t i = audio_start; i < *audio_params.desktop_stream->buffer_index; ++i) {
        audio = circular_array_get(&audio_params.desktop_stream->ring_buffer, i);
        ffmpeg_push_frame(desktop_sound, audio, SNIPX_PA_AUDIO_BYTES_PER_FRAME(opts->fps));
      }
      ffmpeg_close(desktop_sound);

      // Mic sound file
      char mic_sound_filename[256];
      if (opts->mic_sound_monitor != NULL) {
        snprintf(mic_sound_filename, 256, "%s/mic.aac", temp_directory);

        ffmpeg *mic_sound = ffmpeg_init_sound(mic_sound_filename);

        if (*audio_params.mic_stream->buffer_index < (size_t)(opts->fps * opts->length)) audio_start = 0;
        else audio_start = *audio_params.mic_stream->buffer_index - opts->fps * opts->length;

        log_print(&logger, LOG_INFO, "Flushing sound into %s\n", mic_sound_filename);
        for (size_t i = audio_start; i < *audio_params.mic_stream->buffer_index;
            ++i) {
          audio = circular_array_get(&audio_params.mic_stream->ring_buffer, i);
          ffmpeg_push_frame(mic_sound, audio,
                            SNIPX_PA_AUDIO_BYTES_PER_FRAME(opts->fps));
        }
        ffmpeg_close(mic_sound);
      }

      // Video file
      char video_filename[256] = {0};
      if (opts->locally) snprintf(video_filename, sizeof(video_filename), "%s/%s.mp4", dir_default_or_env("./", ENV_SNIPX_OUTPUT_DIR), log_get_time());
      else snprintf(video_filename, sizeof(video_filename), "%s/%s.mp4", temp_directory, log_get_time());

      uint32_t *frame;

      char *sound_files[SOUND_FILES_CAPACITY];
      unsigned int sound_files_amount = 0;
      if (opts->mic_sound_monitor != NULL) {
        sound_files[0] = desktop_sound_filename;
        sound_files[1] = mic_sound_filename;
        sound_files_amount = 2;
      } else {
        sound_files[0] = desktop_sound_filename;
        sound_files[1] = NULL;
        sound_files_amount = 1;
      }

      ffmpeg *video = ffmpeg_init_video(video_filename, screen_width, screen_height, opts->fps, opts->bitrate, sound_files_amount, sound_files);
      int video_start = atomic_load(&video_frame_counter) - opts->fps * opts->length;
      if (video_start < 0) video_start = 0;

      log_print(&logger, LOG_INFO, "Flushing video into %s\n", video_filename);
      for (int i = video_start; i < atomic_load(&video_frame_counter); ++i) {
        frame = circular_array_get(&video_ring_buffer, i);
        ffmpeg_push_frame(video, frame, sizeof(uint32_t) * screen_width * screen_height);
      }

      ffmpeg_close(video);

#ifndef DISABLE_SENDER
      if (!opts->locally) {
        log_print(&logger, LOG_INFO, "Trying to send video.\n");
        if (send_video(logger, opts->address, port, video_filename) == -1)
          log_print(&logger, LOG_ERROR, "%s\n", strerror(errno));
      }
      else {
        log_print(&logger, LOG_INFO, "Video file saved as %s\n", video_filename);
      }
#else
      log_print(&logger, LOG_INFO, "Video file saved as %s\n", video_filename);
#endif
    }
    memset(socket_buffer, 0, SOCKET_BUFFER_SIZE);
    close(client_fd);
  }

#ifndef DISABLE_SENDER
  pthread_join(sender_thread, 0);
#endif
  pthread_mutex_destroy(&lock);
  close(socket_fd);
  log_print(&logger, LOG_INFO, "Exiting.\n");
  log_print(&logger, LOG_INFO, "File saved as %s\n", logger.filename);
  log_close(&logger);

  // Closing
  XShmDetach(display, &shminfo);
  XDestroyImage(shared_image);
  shmdt(shminfo.shmaddr);
  shmctl(shminfo.shmid, IPC_RMID, 0);
  return 0;
}
