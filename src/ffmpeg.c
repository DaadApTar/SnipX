#include <stdlib.h>
#include "ffmpeg.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

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
                             "-loglevel", "verbose",
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

ffmpeg *ffmpeg_init_video(char *soundname, char *videoname, int screen_width, int screen_height, int fps) {
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

    int status_code = execlp("ffmpeg",
                             "ffmpeg",
                             "-loglevel", "verbose",
                             "-y",

                             "-f", "rawvideo",
                             "-pix_fmt", "bgr0",
                             "-s", resolution,
                             "-r", framerate,
                             "-i", "-",
                             "-i", soundname,

                             "-c:v", "libx264",
                             "-vb", "2500k",
                             "-c:a", "aac",
                             "-ab", "200k",
                             "-pix_fmt", "yuv420p",
                             videoname, (char *)NULL);
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
