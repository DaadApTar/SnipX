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
#include "pulseaudio.h"
#include "deferred_render.h"
#include <libnotify/notify.h>

/** @brief Prints an error and exits with exit code 1.
 *  @param logger logger
 *  @param message error message
 */
#define ERROR(logger, message) do {                   \
    log_print(logger, LOG_ERROR, "%s\n", message);    \
                   exit(1);                           \
                 } while(0)
/** @brief Prints errno as error and exits with exit code 1.
 *  @param logger logger
 */
#define ERROR_ERRNO(logger) ERROR(logger, strerror(errno))

#define DEFAULT_PORT 4226
#define SOCKET_BUFFER_SIZE 2
#define STOP_COMMAND           0xF1FA
#define IMMEDIATE_COMMAND      0xFADE
#define DEFER_COMMAND          0xCAFE
#define RENDER_COMMAND         0xFACE
#define COMMAND_SIZE 2

#define COMMAND_TO_BYTES(x) { \
    (uint8_t)(x >> 8),        \
    (uint8_t)(x & 0xFF),      \
  }

#define SOUND_FILES_CAPACITY 8
#define DEFERRED_CLIPS_CAPACITY 256

/** @brief renders sound from ring buffer to file in AAC format.
 *  @param[in] logger logger
 *  @param[in] buffer_index index of last set element in ring buffer
 *  @param[in] ring_buffer the ring buffer itself
 *  @param[in] fps video framerate
 *  @param[in] length length of video in seconds
 *  @param[in] file_path path to file to render.
 */
void render_sound(logger *logger, size_t buffer_index, circular_array *ring_buffer, unsigned int fps, unsigned int length, char *file_path) {
  uint8_t *audio;

  ffmpeg *sound = ffmpeg_init_sound(file_path);

  size_t audio_start;
  if (buffer_index < (size_t)(fps * length)) audio_start = 0;
  else audio_start = buffer_index - fps * length;

  log_print(logger, LOG_INFO, "Flushing sound into %s\n", file_path);
  for (size_t i = audio_start; i < buffer_index; ++i) {
    audio = circular_array_get(ring_buffer, i);
    ffmpeg_push_frame(sound, audio, SNIPX_PA_AUDIO_BYTES_PER_FRAME(fps));
  }

  ffmpeg_close(sound);
}

/** @brief Initialises xinerama.
 *  @param[in] logger logger
 *  @param[in] display X11 display
 *  @return true on success.
 */
bool init_xinerama(logger *logger, Display *display) {
  int xinerama_minor, xinerama_major;
  if (!XineramaQueryExtension(display, &xinerama_minor, &xinerama_major)) {
    log_print(logger, LOG_ERROR, "Xinerama is not supported.\n");
    return 0;
  }
  if (!XineramaIsActive(display)) {
    log_print(logger, LOG_ERROR, "Xinerama is not active.\n");
    return 0;
  }

  log_print(logger, LOG_INFO, "Xinerama version: %d.%d\n", xinerama_major, xinerama_minor);

  return 1;
}

/** @brief Renders a video.
 *  @param[in] logger logger
 *  @param[in] temp_directory a directory to save temp files.
 *  @param[in] opts program options.
 *  @param[in] audio_params struct with all audio information.
 *  @param[in] video_ring_buffer video ring buffer
 *  @param[in] screen_width width of recorded screen
 *  @param[in] screen_height height of recorded screen
 *  @return path to video file.
 */
char *render_video(logger *logger, char *temp_directory,
                  options *opts, audio_capture *audio_capture,
                  circular_array video_ring_buffer, unsigned int screen_width,
                  unsigned int screen_height) {
  // ffmpeg

  char *sound_files[SOUND_FILES_CAPACITY] = {0};

  size_t sound_files_index = 0;
  for (size_t i = 0; i < audio_capture->length; ++i) {
    char audio_filename[PATH_MAX];
    snprintf(audio_filename, sizeof(audio_filename), "%s/%zu.aac", temp_directory, i);
    render_sound(logger, audio_capture->streams[i]->buffer_index,
                  &audio_capture->streams[i]->ring_buffer, opts->fps,
                  opts->length, audio_filename);
    sound_files[sound_files_index++] = strdup(audio_filename);
  }

  // Video file
  char *video_filename = (char*)malloc(PATH_MAX);
#ifndef DISABLE_SENDER
  if (opts->locally) snprintf(video_filename, PATH_MAX, "%s/%s.mp4", dir_default_or_env("./", ENV_SNIPX_OUTPUT_DIR), log_get_time());
  else snprintf(video_filename, PATH_MAX, "%s/%s.mp4", temp_directory, log_get_time());
#else
  snprintf(video_filename, PATH_MAX, "%s/%s.mp4", dir_default_or_env("./", ENV_SNIPX_OUTPUT_DIR), log_get_time());
#endif

  uint32_t *frame;

  ffmpeg *video = ffmpeg_init_video(video_filename, screen_width, screen_height, opts->fps, opts->bitrate, SOUND_FILES_CAPACITY, sound_files);
  int video_start = atomic_load(&video_frame_counter) - opts->fps * opts->length;
  if (video_start < 0) video_start = 0;

  log_print(logger, LOG_INFO, "Flushing video into %s\n", video_filename);
  for (int i = video_start; i < atomic_load(&video_frame_counter); ++i) {
    frame = circular_array_get(&video_ring_buffer, i);
    ffmpeg_push_frame(video, frame, sizeof(uint32_t) * screen_width * screen_height);
  }

  ffmpeg_close(video);

  return video_filename;
}

/** @brief Renders a video.
 *  @param[in] logger logger
 *  @param[in] temp_directory a directory to save temp files.
 *  @param[in] opts program options.
 *  @param[in] components deferred video info.
 *  @param[in] screen_width width of recorded screen
 *  @param[in] screen_height height of recorded screen
 *  @return path to video file.
 */
char *render_deferred_video(logger *logger, char *temp_directory,
                            options *opts, video_components components, unsigned int screen_width,
                            unsigned int screen_height) {
  char *video_filename = (char*)malloc(PATH_MAX);
#ifndef DISABLE_SENDER
  if (opts->locally) snprintf(video_filename, PATH_MAX, "%s/%s.mp4", dir_default_or_env("./", ENV_SNIPX_OUTPUT_DIR), log_get_time());
  else snprintf(video_filename, PATH_MAX, "%s/%s.mp4", temp_directory, log_get_time());
#else
  (void) temp_directory;
  snprintf(video_filename, PATH_MAX, "%s/%s.mp4", dir_default_or_env("./", ENV_SNIPX_OUTPUT_DIR), log_get_time());
#endif

  log_print(logger, LOG_INFO, "Flushing video into %s\n", video_filename);
  ffmpeg *video = ffmpeg_render_video(video_filename, screen_width, screen_height, opts->fps, opts->bitrate, components);

  ffmpeg_close(video);

  return video_filename;
}

/** @brief Sets coordinates of selected screen.
 *  @param[in] logger logger
 *  @param[in] display X11 display
 *  @param[in] screen_number index of screen to record
 *  @param[out] screen_x selected screen x
 *  @param[out] screen_y selected screen y
 *  @param[out] screen_width selected screen width
 *  @param[out] screen_height selected screen height
 *  @param[out] xinerama_screen_number selected screen number given by xinerama
 *  @return true on success.
 */
bool get_screen_data(logger *logger, Display *display,
                     int screen_number, short *screen_x,
                     short *screen_y, short *screen_width,
                     short *screen_height, unsigned int *xinerama_screen_number) {
  int num_screens = 0;
  log_print(logger, LOG_INFO, "Getting existing screens.\n");
  XineramaScreenInfo *screens = XineramaQueryScreens(display, &num_screens);
  if (screen_number > num_screens - 1) {
    log_print(logger, LOG_ERROR, "Invalid screen number.\n");
    return 0;
  }

  XineramaScreenInfo screen_info = screens[screen_number];
  *screen_x = screen_info.x_org;
  *screen_y = screen_info.y_org;
  *screen_width = screen_info.width;
  *screen_height = screen_info.height;
  *xinerama_screen_number = screen_info.screen_number;
  log_print(logger, LOG_INFO, "Screen x: %d, y: %d, width: %d, height: %d.\n", *screen_x, *screen_y, *screen_width, *screen_height);

  return 1;
}

/** @brief Initialises XShm extension and shared image.
 *  @param[in] logger logger
 *  @param[in] display X11 display
 *  @param[in] screen_width selected screen width
 *  @param[in] screen_height selected screen height
 *  @param[in] xinerama_screen_number selected screen number given by xinerama.
 *  @param[out] shminfo shared memory with X11 info
 *  @param[out] shared_image image shared with X11
 *  @return true on success.
 */
bool init_xshm(logger *logger, Display *display,
               unsigned int screen_width, unsigned int screen_height,
               unsigned int xinerama_screen_number, XShmSegmentInfo *shminfo,
               XImage **shared_image) {
  int shm_minor, shm_major;
  Bool pixmaps;
  if (!XShmQueryVersion(display, &shm_minor, &shm_major, &pixmaps)) {
    log_print(logger, LOG_ERROR, "Xshm is not supported.\n");
    return false;
  }

  log_print(logger, LOG_INFO, "Xshm version: %d.%d with shared pixmaps support: %s\n", shm_major, shm_minor, pixmaps ? "ON" : "OFF");

  int depth = DefaultDepth(display, xinerama_screen_number);
  Visual *visual = DefaultVisual(display, xinerama_screen_number);
  *shared_image = XShmCreateImage(display, visual, depth, ZPixmap, 0, shminfo, screen_width, screen_height);

  if (!shared_image) {
    log_print(logger, LOG_ERROR, "Cannot create shared image.\n");
    return false;
  }

  shminfo->shmid = shmget(IPC_PRIVATE, (*shared_image)->bytes_per_line * (*shared_image)->height, IPC_CREAT|0777);

  if (shminfo->shmid < 0) {
    log_print(logger, LOG_ERROR, "Cannot get shmid: %s.\n", strerror(errno));
    return false;
  }

  (*shared_image)->data = (char *)shmat(shminfo->shmid, 0, 0);
  shminfo->shmaddr = (*shared_image)->data;
  shminfo->readOnly = False;

  if (!XShmAttach(display, shminfo)) {
    log_print(logger, LOG_ERROR, "Cannot attach shared memory segment.\n");
    return false;
  }
  XSync(display, False);

  return true;
}

void send_command(logger *logger, int socket_fd, struct sockaddr_in addr, int addrlen, uint16_t command) {
  log_print(logger, LOG_INFO, "Connecting to process.\n");

  uint8_t command_bytes[] = {
    (uint8_t)(command >> 8),
    (uint8_t)(command & 0xFF),
  };

  if(connect(socket_fd, (struct sockaddr *)&addr, addrlen) < 0) {
    ERROR_ERRNO(logger);
  }
  int bytes_sent = write(socket_fd, command_bytes, COMMAND_SIZE);
  if (bytes_sent < 0) {
    ERROR_ERRNO(logger);
  }
}

int main(int argc, char **argv) {
  char* program = *(argv++);
  options *opts = parse_flags(argv, argc-1);

  logger logger;
  log_init(&logger);
  notify_init("SnipX");

  if (opts == 0) {
    print_usage(program);
    return 1;
  }
  if (opts->help) {
    print_usage(program);
    return 0;
  }
  if (opts->sources) {

    snipx_pa_state state = {
      .mode = MODE_LIST_SOURCES,
    };

    snipx_pulseaudio pa;

    prepare_pulseaudio(&pa, state);

    if (start_pulseaudio(&pa) < 0) {
      log_print(&logger, LOG_ERROR,
                "Could not start PulseAudio.\n");
    }

    free_pa(&pa);

    return 0;
  }

  dir_create_if_not_exists(dir_default_or_env(DEFAULT_SNIPX_DIR, ENV_SNIPX_DIR));
  char default_snipx_tmp_dir[256];
  snprintf(default_snipx_tmp_dir, 256, "%s/%s", dir_default_or_env(DEFAULT_SNIPX_DIR, ENV_SNIPX_DIR), DEFAULT_SNIPX_TMP_DIR);

  log_print(&logger, LOG_INFO, "Opening socket.\n");
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    ERROR_ERRNO(&logger);
  }

  struct sockaddr_in addr = {
    .sin_family        = AF_INET,
    .sin_port          = htons(DEFAULT_PORT),
    .sin_addr.s_addr   = htonl(INADDR_ANY)
  };
  socklen_t addrlen = sizeof(addr);

  if (opts->immediate) {
    send_command(&logger, socket_fd, addr, addrlen, IMMEDIATE_COMMAND);
    log_print(&logger, LOG_INFO, "Exiting.\n");
    log_print(&logger, LOG_INFO, "File saved as %s\n", logger.filename);
    log_close(&logger);
    close(socket_fd);

    return 0;
  }
  if (opts->stop) {
    send_command(&logger, socket_fd, addr, addrlen, STOP_COMMAND);
    log_print(&logger, LOG_INFO, "Exiting.\n");
    log_print(&logger, LOG_INFO, "File saved as %s\n", logger.filename);
    log_close(&logger);
    close(socket_fd);

    return 0;
  }
  if (opts->defer) {
    send_command(&logger, socket_fd, addr, addrlen, DEFER_COMMAND);
    log_print(&logger, LOG_INFO, "Exiting.\n");
    log_print(&logger, LOG_INFO, "File saved as %s\n", logger.filename);
    log_close(&logger);
    close(socket_fd);

    return 0;
  }
  if (opts->render) {
    send_command(&logger, socket_fd, addr, addrlen, RENDER_COMMAND);
    log_print(&logger, LOG_INFO, "Exiting.\n");
    log_print(&logger, LOG_INFO, "File saved as %s\n", logger.filename);
    log_close(&logger);
    close(socket_fd);

    return 0;
  }

  log_print(&logger, LOG_INFO, "Setting socket to reuse address.\n");
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
    ERROR_ERRNO(&logger);
  }

  log_print(&logger, LOG_INFO, "Binding address.\n");
  if(bind(socket_fd, (struct sockaddr *)&addr, addrlen) < 0) {
    ERROR_ERRNO(&logger);
  }

  log_print(&logger, LOG_INFO, "Listening socket.\n");
  if (listen(socket_fd, 10) < 0) {
    ERROR_ERRNO(&logger);
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
  Window root = DefaultRootWindow(display);

  if (!init_xinerama(&logger, display)) {
    close(socket_fd);
    return 1;
  };

  short screen_x, screen_y, screen_width, screen_height;
  unsigned int xinerama_screen_number;
  if (!get_screen_data(&logger, display, opts->screen_number, &screen_x, &screen_y, &screen_width, &screen_height, &xinerama_screen_number)) {
    close(socket_fd);
    return 1;
  };

  XShmSegmentInfo shminfo;
  XImage *shared_image = NULL;
  if (!init_xshm(&logger, display, screen_width, screen_height, xinerama_screen_number, &shminfo, &shared_image)) {
    close(socket_fd);
    return 1;
  }

  // Initialise ring buffers.

  circular_array video_ring_buffer;
  circular_array desktop_audio_ring_buffer;
  circular_array mic_audio_ring_buffer;

  log_print(&logger, LOG_INFO, "Initialising video buffer.\n");
  circular_array_init(&video_ring_buffer, opts->fps * opts->length, shared_image->bytes_per_line * shared_image->height);
  log_print(&logger, LOG_INFO, "Initialising audio buffers.\n");
  circular_array_init(&desktop_audio_ring_buffer, opts->fps * opts->length, SNIPX_PA_AUDIO_BYTES_PER_FRAME(opts->fps));
  circular_array_init(&mic_audio_ring_buffer, opts->fps * opts->length, SNIPX_PA_AUDIO_BYTES_PER_FRAME(opts->fps));

  video_capturing_params video_params = {
    .display = display,
    .window = root,
    .shared_image = shared_image,
    .screen_x = screen_x,
    .screen_y = screen_y,
    .framerate = opts->fps,
    .logger = &logger,
    .ring_buffer = &video_ring_buffer,
  };

  // Initialise pulseaudio
  log_print(&logger, LOG_INFO, "Preparing PulseAudio.\n");

  audio_stream desktop_stream = {
      .index = opts->desktop_sound_monitor,
      .name = "Desktop Audio",
      .ring_buffer = desktop_audio_ring_buffer,
  };

  audio_stream mic_stream = {
      .index = opts->mic_sound_monitor,
      .name = "Mic/Aux",
      .ring_buffer = mic_audio_ring_buffer,
  };

  audio_capture audio_capture = {
      .fragsize = SNIPX_PA_AUDIO_BYTES_PER_FRAME(opts->fps),
  };

  audio_capture.streams[audio_capture.length++] = &desktop_stream;
  if (opts->mic_sound_monitor != -1)
    audio_capture.streams[audio_capture.length++] = &mic_stream;

  snipx_pa_state state = {
    .mode = MODE_RECORD,
    .capture = audio_capture,
  };

  snipx_pulseaudio pa;

  prepare_pulseaudio(&pa, state);

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

  log_print(&logger, LOG_INFO, "Creating sender thread.\n");
  pthread_create(&sender_thread, 0, thread_send_temp_files, (void *)&temp_sender_params);
#endif
  log_print(&logger, LOG_INFO, "Creating video thread.\n");
  pthread_create(&video_thread, 0, thread_video_capturing, (void *)&video_params);
  log_print(&logger, LOG_INFO, "Creating audio thread.\n");
  if (start_pulseaudio(&pa) < 0) {
    log_print(&logger, LOG_ERROR,
              "Could not start PulseAudio.\n");
    goto free_app;
  }

  if (state.result == RESULT_ERR) {
    log_print(&logger, LOG_ERROR,
              "Could not start PulseAudio.\n");
    goto free_app;
  }

  int client_fd;

  video_components deferred_clips[DEFERRED_CLIPS_CAPACITY] = {0};
  size_t deferred_clips_amount = 0;
  log_print(&logger, LOG_INFO, "Accepting from socket...\n");
  while (!is_stopped) {
    if((client_fd = accept(socket_fd, (struct sockaddr *)&addr, &addrlen)) < 0) {
      ERROR_ERRNO(&logger);
    }
    if (read(client_fd, socket_buffer, SOCKET_BUFFER_SIZE) < 0) {
      ERROR_ERRNO(&logger);
    }

    log_print(&logger, LOG_INFO, "Received message.\n");

    uint8_t stop_command[] = COMMAND_TO_BYTES(STOP_COMMAND);
    uint8_t brake_command[] = COMMAND_TO_BYTES(IMMEDIATE_COMMAND);
    uint8_t defer_command[] = COMMAND_TO_BYTES(DEFER_COMMAND);
    uint8_t render_command[] = COMMAND_TO_BYTES(RENDER_COMMAND);

    if (memcmp(socket_buffer, stop_command, SOCKET_BUFFER_SIZE) == 0) {
      log_print(&logger, LOG_INFO, "Stopping recording.\n");
      is_stopped = true;

      atomic_store(&running_flag, 0);
      pthread_join(video_thread, 0);
    }
    if (memcmp(socket_buffer, brake_command, SOCKET_BUFFER_SIZE) == 0) {
      log_print(&logger, LOG_INFO, "Starting immediate rendering.\n");

      atomic_store(&running_flag, 0);
      pthread_join(video_thread, 0);

      stop_pulseaudio(&pa);

      // Create tmp directory.
      char *temp_directory = dir_default_or_env(default_snipx_tmp_dir, ENV_SNIPX_TMP_DIR);
      dir_create_if_not_exists(temp_directory);

      NotifyNotification *notify;
      notify = notify_notification_new("SnipX", "Rendering has started.", NULL);

      notify_notification_show(notify, NULL);

      g_object_unref(notify);

      char *video_filepath = render_video(&logger, temp_directory, opts, &audio_capture, video_ring_buffer, screen_width, screen_height);

#ifndef DISABLE_SENDER
      if (!opts->locally) {
        log_print(&logger, LOG_INFO, "Trying to send video.\n");
        if (send_video(logger, opts->address, port, video_filepath) == -1)
          log_print(&logger, LOG_ERROR, "%s\n", strerror(errno));
      }
      else {
        log_print(&logger, LOG_INFO, "Video file saved as %s\n", video_filepath);
      }
#else
      log_print(&logger, LOG_INFO, "Video file saved as %s\n", video_filepath);
#endif

      free(video_filepath);
      free(temp_directory);

      atomic_store(&video_frame_counter, 0);

      circular_array_clear(&video_ring_buffer);

      for (size_t i = 0; i < audio_capture.length; ++i) {
        circular_array_clear(&audio_capture.streams[i]->ring_buffer);
        audio_capture.streams[i]->buffer_index = 0;
      }
      atomic_store(&running_flag, 1);
      atomic_store(&video_frame_counter, 1);
      pthread_create(&video_thread, 0, thread_video_capturing, (void *)&video_params);

      proceed_pulseaudio(&pa);
    }
    if (memcmp(socket_buffer, defer_command, SOCKET_BUFFER_SIZE) == 0) {
      log_print(&logger, LOG_INFO, "Deferring clip.\n");
      
      atomic_store(&running_flag, 0);
      pthread_join(video_thread, 0);

      stop_pulseaudio(&pa);

      // Create tmp directory.
      char *temp_directory = dir_default_or_env(default_snipx_tmp_dir, ENV_SNIPX_TMP_DIR);
      dir_create_if_not_exists(temp_directory);

      if (deferred_clips_amount < DEFERRED_CLIPS_CAPACITY) {
        defer_video_args *dva = alloc_defer_video_args(&video_ring_buffer, atomic_load(&video_frame_counter), audio_capture.streams, audio_capture.length, temp_directory, deferred_clips_amount, &deferred_clips[deferred_clips_amount]);
        pthread_t defer_video_thread;
        pthread_create(&defer_video_thread, NULL, thread_defer_video, dva);
        pthread_detach(defer_video_thread);
        deferred_clips_amount++;

        NotifyNotification *notify;
        notify = notify_notification_new("SnipX", "Clip was deferred.", NULL);
        notify_notification_show(notify, NULL);
        g_object_unref(notify);
      }
      else {
        log_print(&logger, LOG_WARNING, "Ran out of clips capacity.");

        NotifyNotification *notify;
        notify = notify_notification_new("SnipX", "Ran out of clips capacity.", NULL);
        notify_notification_show(notify, NULL);
        g_object_unref(notify);
      }

      free(temp_directory);

      atomic_store(&video_frame_counter, 0);

      circular_array_clear(&video_ring_buffer);

      for (size_t i = 0; i < audio_capture.length; ++i) {
        circular_array_clear(&audio_capture.streams[i]->ring_buffer);
        audio_capture.streams[i]->buffer_index = 0;
      }
      atomic_store(&running_flag, 1);
      atomic_store(&video_frame_counter, 1);
      pthread_create(&video_thread, 0, thread_video_capturing, (void *)&video_params);

      proceed_pulseaudio(&pa);
    }
    if (memcmp(socket_buffer, render_command, SOCKET_BUFFER_SIZE) == 0) {
      log_print(&logger, LOG_INFO, "Rendering %d clips.\n", deferred_clips_amount);

      // Create tmp directory.
      char *temp_directory = dir_default_or_env(default_snipx_tmp_dir, ENV_SNIPX_TMP_DIR);
      dir_create_if_not_exists(temp_directory);

      NotifyNotification *notify;
      notify = notify_notification_new("SnipX", "Rendering has started.", NULL);

      notify_notification_show(notify, NULL);

      g_object_unref(notify);

      for (size_t i = 0; i < DEFERRED_CLIPS_CAPACITY; ++i) {
        if (deferred_clips[i].video_file == 0) continue;
        char *video_filepath = render_deferred_video(&logger, temp_directory, opts, deferred_clips[i], screen_width, screen_height);

#ifndef DISABLE_SENDER
        if (!opts->locally) {
          log_print(&logger, LOG_INFO, "Trying to send video.\n");
          if (send_video(logger, opts->address, port, video_filepath) == -1)
            log_print(&logger, LOG_ERROR, "%s\n", strerror(errno));
        }
        else {
          log_print(&logger, LOG_INFO, "Video file saved as %s\n", video_filepath);
        }
#else
        log_print(&logger, LOG_INFO, "Video file saved as %s\n", video_filepath);
#endif

        free(video_filepath);

        delete_video_components(&deferred_clips[i]);
        deferred_clips_amount--;
      }
      free(temp_directory);
    }
    memset(socket_buffer, 0, SOCKET_BUFFER_SIZE);
    close(client_fd);
  }

#ifndef DISABLE_SENDER
  pthread_join(sender_thread, 0);
#endif

  // WARNING: high-quality code operation. Goto opponents should not read the code below
free_app:

  pthread_mutex_destroy(&lock);
  close(socket_fd);
  log_print(&logger, LOG_INFO, "Exiting.\n");
  log_print(&logger, LOG_INFO, "Log file saved as %s\n", logger.filename);
  log_close(&logger);

  // Freeing ring buffers
  circular_array_free(&video_ring_buffer);
  circular_array_free(&desktop_audio_ring_buffer);
  circular_array_free(&mic_audio_ring_buffer);

  free_pa(&pa);

  // Closing
  XShmDetach(display, &shminfo);
  XDestroyImage(shared_image);
  shmdt(shminfo.shmaddr);
  shmctl(shminfo.shmid, IPC_RMID, 0);

  notify_uninit();
  return 0;
}
