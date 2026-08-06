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
#include "ffmpeg.h"
#include "directory_manager.h"
#include "defaults.h"
#include "sender.h"
#include "pulseaudio.h"
#include "deferred_render.h"
#include "autocompletion.h"
#include "notification.h"
#include <version.h>
#include "x11.h"
#include "compression.h"

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
 *  @param[in] ring_buffer the ring buffer itself
 *  @param[in] fps video framerate
 *  @param[in] length length of video in seconds
 *  @param[in] file_path path to file to render.
 */
void render_sound(logger *logger, dynamic_circular_array *ring_buffer, unsigned int fps, unsigned int length, char *file_path) {
  uint8_t *audio;

  ffmpeg *sound = ffmpeg_init_sound(file_path);

  size_t audio_start;
  if (ring_buffer->next_index < (size_t)(fps * length)) audio_start = 0;
  else audio_start = ring_buffer->next_index - fps * length;

  log_print(logger, LOG_INFO, "Flushing sound into %s\n", file_path);
  for (size_t i = audio_start; i < ring_buffer->next_index; ++i) {
    size_t size = 0;
    audio = dynamic_circular_array_get(ring_buffer, i, &size);
    /* ffmpeg_push_frame(sound, audio, SNIPX_PA_AUDIO_BYTES_PER_FRAME(fps)); */
    ffmpeg_push_frame(sound, audio, size);
    free(audio);
  }

  ffmpeg_close(sound);
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
                  video_capture video_capture) {
  // ffmpeg

  char *sound_files[SOUND_FILES_CAPACITY] = {0};

  size_t sound_files_index = 0;
  for (size_t i = 0; i < audio_capture->length; ++i) {
    if (audio_capture->streams[i]->stream == 0) continue;
    char audio_filename[PATH_MAX];
    snprintf(audio_filename, sizeof(audio_filename), "%s/%zu.aac", temp_directory, i);
    render_sound(logger,
                  &audio_capture->streams[i]->ring_buffer, opts->fps,
                  opts->length, audio_filename);
    sound_files[sound_files_index++] = strdup(audio_filename);
  }

  // Video file
  char *video_filename = (char*)malloc(PATH_MAX);
#ifdef FEATURE_SENDER
  if (opts->locally) snprintf(video_filename, PATH_MAX, "%s/%s.mp4", !opts->output ? "." : opts->output, log_get_time());
  else snprintf(video_filename, PATH_MAX, "%s/%s.mp4", temp_directory, log_get_time());
#else
  snprintf(video_filename, PATH_MAX, "%s/%s.mp4", !opts->output ? "." : opts->output, log_get_time());
#endif

  void *compressed_frame;

  ffmpeg *video = ffmpeg_init_video(video_filename, video_capture.screen_width, video_capture.screen_height, opts->fps, opts->bitrate, SOUND_FILES_CAPACITY, sound_files);
  size_t video_start;
  if (video_capture.ring_buffer->next_index < (size_t)(opts->fps * opts->length)) video_start = 0;
  else video_start = video_capture.ring_buffer->next_index - opts->fps * opts->length;

  log_print(logger, LOG_INFO, "Flushing video into %s\n", video_filename);
  void *decompressed_frame = malloc(video_capture.framesize);
  void *delta_frame = malloc(video_capture.framesize);
  void *prev = malloc(video_capture.framesize);
  for (size_t i = video_start; i < video_capture.ring_buffer->next_index; ++i) {
    size_t compressed_framesize = 0;
    compressed_frame = dynamic_circular_array_get(video_capture.ring_buffer, i, &compressed_framesize);
    video_capture.decompression(decompressed_frame, video_capture.framesize, compressed_frame, compressed_framesize);

    if (i == video_start) {
      xor_delta(delta_frame, decompressed_frame, video_capture.keyframe, video_capture.framesize);
    }
    else {
      xor_delta(delta_frame, prev, decompressed_frame, video_capture.framesize);
    }
    memcpy(prev, delta_frame, video_capture.framesize);

    ffmpeg_push_frame(video, delta_frame, video_capture.framesize);
    free(compressed_frame);
  }
  free(decompressed_frame);
  free(delta_frame);
  free(prev);

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
#ifdef FEATURE_SENDER
  if (opts->locally) snprintf(video_filename, PATH_MAX, "%s/%s.mp4", !opts->output ? "." : opts->output, log_get_time());
  else snprintf(video_filename, PATH_MAX, "%s/%s.mp4", temp_directory, log_get_time());
#else
  (void) temp_directory;
  snprintf(video_filename, PATH_MAX, "%s/%s.mp4", !opts->output ? "." : opts->output, log_get_time());
#endif

  log_print(logger, LOG_INFO, "Flushing video into %s\n", video_filename);
  ffmpeg *video = ffmpeg_render_video(video_filename, screen_width, screen_height, opts->fps, opts->bitrate, components);

  ffmpeg_close(video);

  return video_filename;
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
void send_command(logger *logger, uint16_t command) {
  log_print(logger, LOG_INFO, "Connecting to process.\n");

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    ERROR_ERRNO(logger);
  }

  struct sockaddr_in addr = {
    .sin_family        = AF_INET,
    .sin_port          = htons(DEFAULT_PORT),
    .sin_addr.s_addr   = htonl(INADDR_ANY)
  };
  socklen_t addrlen = sizeof(addr);


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

  close(socket_fd);
}

int main(int argc, char **argv) {
  char* program = *(argv++);
  options *opts = parse_flags(argv, argc-1);

  if (opts == 0) {
    print_usage(program);
    return 1;
  }

  logger logger;
  log_init(&logger, opts->debug);

  if (opts->help) {
    print_usage(program);
    return 0;
  }
  if (opts->version) {
    print_version();
    return 0;
  }
  if (opts->sources) {

    snipx_pa_state state = {
      .mode = MODE_LIST_SOURCES,
    };

    snipx_pulseaudio pa;

    prepare_pulseaudio(&pa, &logger, state);

    if (start_pulseaudio(&pa) < 0) {
      log_print(&logger, LOG_ERROR,
                "Could not start PulseAudio.\n");
    }

    free_pa(&pa);

    return 0;
  }
  if (opts->autocompletion != 0) {
    if (generate_autocompletion_script(&logger, opts->autocompletion) < 0) return 1;
    else return 0;
  }

  compression_algorithm algorithm = dispatch_string(opts->compression);
  if (algorithm == COMPRESSION_INVALID) {
    log_print(&logger, LOG_ERROR, "Unknown compression algorithm `%s`\n", opts->compression);
    exit(1);
  }
  log_print(&logger, LOG_DEBUG, "Compression algorithm: `%s`\n", opts->compression == 0 ? NONE_STRING : opts->compression);
  compression_wrapper compression = dispatch_compression_algorithm(algorithm);
  decompression_wrapper decompression = dispatch_decompression_algorithm(algorithm);

  dbus_conn_new(&logger);

  dir_create_if_not_exists(dir_default_or_env(DEFAULT_SNIPX_DIR, ENV_SNIPX_DIR));
  char default_snipx_tmp_dir[256];
  snprintf(default_snipx_tmp_dir, 256, "%s/%s", dir_default_or_env(DEFAULT_SNIPX_DIR, ENV_SNIPX_DIR), DEFAULT_SNIPX_TMP_DIR);

  if (opts->immediate) send_command(&logger, IMMEDIATE_COMMAND);
  if (opts->render) send_command(&logger, RENDER_COMMAND);
  if (opts->defer) send_command(&logger, DEFER_COMMAND);
  if (opts->stop) send_command(&logger, STOP_COMMAND);

  if (opts->close_after) {
    log_print(&logger, LOG_INFO, "Exiting.\n");
    log_close(&logger);
    return 0;
  }

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

  log_print(&logger, LOG_DEBUG, "Setting socket to reuse address.\n");
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
#ifdef FEATURE_SENDER
  pthread_t sender_thread;
#endif
  pthread_mutex_init(&video_capturing_lock, 0);
  atomic_store(&video_capturing_running_flag, 1);

  // Initialise X11

  log_print(&logger, LOG_INFO, "Opening X11 display.\n");
  snipx_x11 x11 = init_x11(&logger);

  if (init_xinerama(x11) < 0) {
    close(socket_fd);
    return 1;
  };

  if (get_screen_data(&x11, opts->screen_number) < 0) {
    close(socket_fd);
    return 1;
  };

  if (init_xshm(&x11) < 0) {
    close(socket_fd);
    return 1;
  }

  // Initialise ring buffers.

  dynamic_circular_array video_ring_buffer;
  dynamic_circular_array desktop_audio_ring_buffer;
  dynamic_circular_array mic_audio_ring_buffer;
  // TODO: if compression is enabled
  circular_array compression_queue;

  size_t frame_size = x11.shared_image->bytes_per_line * x11.shared_image->height;
  size_t items_amount = opts->fps * opts->length;
  size_t compression_bound = get_compression_bound(algorithm, frame_size);
  log_print(&logger, LOG_INFO, "Initialising video buffer.\n");
  // TODO: reconsider sizes when compression is added.
  dynamic_circular_array_init(&video_ring_buffer, get_compression_bound(algorithm, frame_size) * items_amount / 8, items_amount);
  log_print(&logger, LOG_INFO, "Initialising audio buffers.\n");
  dynamic_circular_array_init(&desktop_audio_ring_buffer, SNIPX_PA_AUDIO_BYTES_PER_FRAME(opts->fps) * items_amount, items_amount);
  dynamic_circular_array_init(&mic_audio_ring_buffer, SNIPX_PA_AUDIO_BYTES_PER_FRAME(opts->fps) * items_amount, items_amount);
  // TODO: if compression is enabled
  circular_array_init(&compression_queue, opts->fps, frame_size);

  x11.capture.ring_buffer = &video_ring_buffer;
  x11.capture.framerate = opts->fps;
  x11.capture.decompression = decompression;
  x11.capture.framesize = frame_size;
  x11.capture.keyframe = malloc(frame_size);

  // Initialise pulseaudio
  log_print(&logger, LOG_INFO, "Preparing PulseAudio.\n");

  audio_stream desktop_stream = {
      .index = opts->desktop_sound_monitor,
      .name = "Desktop Audio",
      .ring_buffer = desktop_audio_ring_buffer,
      .fragsize = SNIPX_PA_AUDIO_BYTES_PER_FRAME(opts->fps),
  };

  audio_stream mic_stream = {
      .index = opts->mic_sound_monitor,
      .name = "Mic/Aux",
      .ring_buffer = mic_audio_ring_buffer,
      .fragsize = SNIPX_PA_AUDIO_BYTES_PER_FRAME(opts->fps),
  };

  audio_capture audio_capture;

  audio_capture.streams[audio_capture.length++] = &desktop_stream;
  if (opts->mic_sound_monitor != -1)
    audio_capture.streams[audio_capture.length++] = &mic_stream;

  snipx_pa_state state = {
    .mode = MODE_RECORD,
    .capture = audio_capture,
  };

  snipx_pulseaudio pa;

  prepare_pulseaudio(&pa, &logger, state);

#ifdef FEATURE_SENDER
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

  // Initialising compression
  // TODO: if compression is enabled
  log_print(&logger, LOG_INFO, "Creating compession thread.\n");
  compression_args compression_args = {
    .queue = &compression_queue,
    .compression_bound = compression_bound,
    .dst = &video_ring_buffer,
    .compression = compression,
    .decompression = decompression,
    .compression_level = opts->compression_level,
    .keyframe = x11.capture.keyframe,
  };
  compression_context compression_ctx;
  compression_init(&compression_ctx, compression_args);
  x11.capture.compression_ctx = &compression_ctx;
  compression_start(&compression_ctx);

  log_print(&logger, LOG_INFO, "Creating video thread.\n");
  pthread_create(&video_thread, 0, thread_video_capturing, (void *)&x11);
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

  char *temp_directory = dir_default_or_env(default_snipx_tmp_dir, ENV_SNIPX_TMP_DIR);
  video_components deferred_clips[DEFERRED_CLIPS_CAPACITY] = {0};
  size_t deferred_clips_amount = load_deferred_files(temp_directory, deferred_clips, DEFERRED_CLIPS_CAPACITY);
  free(temp_directory);

  log_print(&logger, LOG_INFO, "Accepting from socket...\n");
  while (!is_stopped) {
    if((client_fd = accept(socket_fd, (struct sockaddr *)&addr, &addrlen)) < 0) {
      ERROR_ERRNO(&logger);
    }
    if (read(client_fd, socket_buffer, SOCKET_BUFFER_SIZE) < 0) {
      ERROR_ERRNO(&logger);
    }

    log_print(&logger, LOG_DEBUG, "Received message.\n");

    uint8_t stop_command[] = COMMAND_TO_BYTES(STOP_COMMAND);
    uint8_t brake_command[] = COMMAND_TO_BYTES(IMMEDIATE_COMMAND);
    uint8_t defer_command[] = COMMAND_TO_BYTES(DEFER_COMMAND);
    uint8_t render_command[] = COMMAND_TO_BYTES(RENDER_COMMAND);

    if (memcmp(socket_buffer, stop_command, SOCKET_BUFFER_SIZE) == 0) {
      log_print(&logger, LOG_INFO, "Stopping recording.\n");
      is_stopped = true;

      atomic_store(&video_capturing_running_flag, 0);
      pthread_join(video_thread, 0);
      compression_stop(&compression_ctx);
    }
    if (memcmp(socket_buffer, brake_command, SOCKET_BUFFER_SIZE) == 0) {
      log_print(&logger, LOG_INFO, "Starting immediate rendering.\n");

      // TODO: thread mess.
      atomic_store(&video_capturing_running_flag, 0);

      stop_pulseaudio(&pa);

      pthread_join(video_thread, 0);
      compression_stop(&compression_ctx);

      // Create tmp directory.
      char *temp_directory = dir_default_or_env(default_snipx_tmp_dir, ENV_SNIPX_TMP_DIR);
      dir_create_if_not_exists(temp_directory);

      send_notification(&logger, "Rendering has started.");

      char *video_filepath = render_video(&logger, temp_directory, opts, &audio_capture, x11.capture);

#ifdef FEATURE_SENDER
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

      dynamic_circular_array_clear(&video_ring_buffer);
      circular_array_clear(&compression_queue);

      for (size_t i = 0; i < audio_capture.length; ++i) {
        dynamic_circular_array_clear(&audio_capture.streams[i]->ring_buffer);
      }
      // TODO: thread mess.
      atomic_store(&video_capturing_running_flag, 1);
      pthread_create(&video_thread, 0, thread_video_capturing, (void *)&x11);
      compression_start(&compression_ctx);

      proceed_pulseaudio(&pa);
    }
    if (memcmp(socket_buffer, defer_command, SOCKET_BUFFER_SIZE) == 0) {
      log_print(&logger, LOG_INFO, "Deferring clip.\n");
      
      // TODO: thread mess.
      atomic_store(&video_capturing_running_flag, 0);
      pthread_join(video_thread, 0);

      stop_pulseaudio(&pa);
      compression_stop(&compression_ctx);

      // Create tmp directory.
      char *temp_directory = dir_default_or_env(default_snipx_tmp_dir, ENV_SNIPX_TMP_DIR);
      dir_create_if_not_exists(temp_directory);

      if (deferred_clips_amount < DEFERRED_CLIPS_CAPACITY) {
        defer_video_args *dva = alloc_defer_video_args(&video_ring_buffer, audio_capture.streams, audio_capture.length, temp_directory, deferred_clips_amount, opts->mbps, &deferred_clips[deferred_clips_amount]);
        pthread_t defer_video_thread;
        pthread_create(&defer_video_thread, NULL, thread_defer_video, dva);
        pthread_detach(defer_video_thread);
        deferred_clips_amount++;

        send_notification(&logger, "Clip was deferred.");
      }
      else {
        log_print(&logger, LOG_WARNING, "Ran out of clips capacity.");

        send_notification(&logger, "Ran out of clips capacity.");
      }

      free(temp_directory);

      dynamic_circular_array_clear(&video_ring_buffer);
      circular_array_clear(&compression_queue);

      for (size_t i = 0; i < audio_capture.length; ++i) {
        dynamic_circular_array_clear(&audio_capture.streams[i]->ring_buffer);
      }
      // TODO: thread mess.
      compression_start(&compression_ctx);
      atomic_store(&video_capturing_running_flag, 1);
      pthread_create(&video_thread, 0, thread_video_capturing, (void *)&x11);

      proceed_pulseaudio(&pa);
    }
    if (memcmp(socket_buffer, render_command, SOCKET_BUFFER_SIZE) == 0) {
      log_print(&logger, LOG_INFO, "Rendering %d clips.\n", deferred_clips_amount);

      // Create tmp directory.
      char *temp_directory = dir_default_or_env(default_snipx_tmp_dir, ENV_SNIPX_TMP_DIR);
      dir_create_if_not_exists(temp_directory);

      send_notification(&logger, "Rendering has started.");

      for (size_t i = 0; i < DEFERRED_CLIPS_CAPACITY; ++i) {
        if (deferred_clips[i].video_file == 0) continue;
        char *video_filepath = render_deferred_video(&logger, temp_directory, opts, deferred_clips[i], x11.capture.screen_width, x11.capture.screen_height);

#ifdef FEATURE_SENDER
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

#ifdef FEATURE_SENDER
  pthread_join(sender_thread, 0);
#endif

  // WARNING: high-quality code operation. Goto opponents should not read the code below
free_app:

  pthread_mutex_destroy(&video_capturing_lock);
  close(socket_fd);
  log_print(&logger, LOG_INFO, "Exiting.\n");
  log_close(&logger);

  // Freeing ring buffers
  dynamic_circular_array_free(&video_ring_buffer);
  dynamic_circular_array_free(&desktop_audio_ring_buffer);
  dynamic_circular_array_free(&mic_audio_ring_buffer);

  free_pa(&pa);
  free(x11.capture.keyframe);

  // Closing
  free_x11(&x11);

  return 0;
}
