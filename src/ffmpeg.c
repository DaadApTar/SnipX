#include <stdlib.h>
#include "ffmpeg.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

#define VIDEO_ARG_CAPACITY 64

ffmpeg *ffmpeg_init_sound(char *soundname) {
  ffmpeg *result = (ffmpeg *)malloc(sizeof(ffmpeg));
  int ffmpeg_pipe[2];

  if (pipe(ffmpeg_pipe) < 0) {
    free(result);
    return 0;
  }

  pid_t pid = fork();
  if (pid < 0) {
    free(result);
    return 0;
  }

  result->pid = pid;

  if (pid == 0) {
    dup2(ffmpeg_pipe[0], 0);
    close(ffmpeg_pipe[1]);
    close(ffmpeg_pipe[0]);
    int status_code = execlp("ffmpeg",
                             "ffmpeg",
                             /* "-loglevel", "verbose", */
                             "-y",

                             "-f", "s16le",
                             "-ar", "44100",
                             "-ac", "2",
                             "-i", "-",

                             "-c:a", "aac",
                             "-ab", "200k",
                             soundname, (char *)NULL);
    if (status_code < 0) {
      free(result);
      return 0;
    }
  }

  close(ffmpeg_pipe[0]);

  result->pipefd = ffmpeg_pipe[1];
  result->pid = pid;

  return result;
}

ffmpeg *ffmpeg_init_video(char *videoname, int screen_width, int screen_height, int fps, int bitrate, unsigned int sound_files_cap, char **filenames) {
  ffmpeg *result = (ffmpeg *)malloc(sizeof(ffmpeg));
  int ffmpeg_pipe[2];

  if (pipe(ffmpeg_pipe) < 0) {
    free(result);
    return 0;
  }

  pid_t pid = fork();
  if (pid < 0) {
    free(result);
    return 0;
  }

  result->pid = pid;

  if (pid == 0) {
    dup2(ffmpeg_pipe[0], 0);
    close(ffmpeg_pipe[1]);
    close(ffmpeg_pipe[0]);

    char resolution[64];
    snprintf(resolution, sizeof(resolution), "%dx%d", screen_width, screen_height);

    char framerate[64];
    snprintf(framerate, sizeof(framerate), "%d", fps);

    char bitrate_str[64];
    snprintf(bitrate_str, sizeof(bitrate_str), "%d", bitrate);

    char *argv[VIDEO_ARG_CAPACITY];
    int n = 0;
    argv[n++] = "ffmpeg";
    argv[n++] = "-y";

    argv[n++] = "-f";
    argv[n++] = "rawvideo";

    argv[n++] = "-pix_fmt";
    argv[n++] = "bgr0";

    argv[n++] = "-s";
    argv[n++] = resolution;

    argv[n++] = "-r";
    argv[n++] = framerate;

    argv[n++] = "-i";
    argv[n++] = "-";
    
    unsigned int amount = 0;
    for (size_t i = 0; i < sound_files_cap && filenames[i] != NULL; ++i) {
      argv[n++] = "-i";
      argv[n++] = filenames[i];
      amount++;
    }

    argv[n++] = "-map";
    argv[n++] = "0:v";

    size_t offset = 0;
    size_t cap = 128;
    char *filter = (char*)malloc(cap);
    for (size_t i = 0; i < amount; ++i) {
      int needed = snprintf(NULL, 0, "[%zu:a]", i + 1);
      if (offset + (size_t)needed + 1 > cap) {
        cap = (offset + needed) * 2;
        filter = realloc(filter, cap);
      }
      offset += snprintf(filter+offset, 128 - offset, "[%zu:a]", i+1);
    }
    int needed = snprintf(NULL, 0, "amix=inputs=%du[a]", amount);
    if (offset + (size_t)needed + 1 > cap) {
      cap = (offset + needed) * 2;
      filter = realloc(filter, cap);
    }
    snprintf(filter+offset, cap - offset, "amix=inputs=%d[a]", amount);

    argv[n++] = "-filter_complex";
    argv[n++] = filter;

    argv[n++] = "-map";
    argv[n++] = "[a]";

    argv[n++] = "-c:v";
    argv[n++] = "libx264";

    argv[n++] = "-vb";
    argv[n++] = bitrate_str;

    argv[n++] = "-c:a";
    argv[n++] = "aac";

    argv[n++] = "-ab";
    argv[n++] = "200k";

    argv[n++] = "-pix_fmt";
    argv[n++] = "yuv420p";

    argv[n++] = videoname;
    argv[n++] = NULL;

    int status_code = execvp("ffmpeg", argv);
    if (status_code < 0) {
      free(result);
      return 0;
    }
  }

  close(ffmpeg_pipe[0]);

  result->pipefd = ffmpeg_pipe[1];
  result->pid = pid;

  return result;
}

int ffmpeg_push_frame(ffmpeg *instance, void *data, ssize_t size) {
  int written = write(instance->pipefd, data, size);
  return written == size ? 0 : -1;
}

int ffmpeg_close(ffmpeg *instance) {
  int ret = close(instance->pipefd);
  if (ret == -1) {
    return -1;
  }
  ret = waitpid(instance->pid, 0, 0);
  if (ret == -1) {
    return -1;
  }
  free(instance);
  return 0;
}
