#include "ffmpeg.h"
#include "options.h"
#include "log.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* Defined in main.c; the test links it with the application entry point renamed. */
char *render_deferred_video(logger *, char *, options *, video_components,
                            unsigned int, unsigned int);

static void check(int condition, const char *message) {
  if (!condition) {
    fprintf(stderr, "FAIL: %s\n", message);
    exit(1);
  }
}

static void check_render(logger *log, options *opts, video_components components,
                         const char *mode, int expect_success) {
  check(setenv("SNIPX_TEST_ENCODER_MODE", mode, 1) == 0, "set encoder mode");
  char *output = render_deferred_video(log, opts->output, opts, components, 2, 2);
  check((output != NULL) == expect_success, mode);
  check(access(components.video_file, F_OK) == 0, "raw replay remains available");
  if (output) {
    check(unlink(output) == 0, "remove test output");
    free(output);
  }
}

int main(void) {
  char directory[] = "/tmp/snipx-render-test-XXXXXX";
  check(mkdtemp(directory) != NULL, "create test directory");
  char executable[512], raw[512];
  snprintf(executable, sizeof executable, "%s/ffmpeg", directory);
  snprintf(raw, sizeof raw, "%s/input.raw", directory);
  FILE *fixture = fopen(executable, "w");
  check(fixture != NULL, "create encoder fixture");
  fputs("#!/bin/sh\n"
        "for output do :; done\n"
        "case \"$SNIPX_TEST_ENCODER_MODE\" in\n"
        " failure) exit 1;;\n"
        " missing) exit 0;;\n"
        " empty) : > \"$output\";;\n"
        " success) printf video > \"$output\";;\n"
        "esac\n", fixture);
  check(fclose(fixture) == 0 && chmod(executable, 0700) == 0, "prepare encoder fixture");
  fixture = fopen(raw, "w");
  check(fixture != NULL, "create raw replay");
  fputs("raw replay", fixture);
  check(fclose(fixture) == 0, "close raw replay");
  check(setenv("PATH", directory, 1) == 0, "use fixture encoder");

  logger log = {.file = tmpfile()};
  check(log.file != NULL, "open test log");
  options opts = {.output = directory, .locally = true, .fps = 30, .bitrate = 1000};
  video_components components = {.video_file = raw};
  check_render(&log, &opts, components, "failure", 0);
  check_render(&log, &opts, components, "missing", 0);
  check_render(&log, &opts, components, "empty", 0);
  check_render(&log, &opts, components, "success", 1);
  check(unlink(executable) == 0, "remove encoder fixture");
  check_render(&log, &opts, components, "unavailable executable", 0);

  /* A child killed by a signal must also be reported as unsuccessful. */
  int pipefd[2];
  check(pipe(pipefd) == 0, "create test pipe");
  ffmpeg *encoder = malloc(sizeof *encoder);
  check(encoder != NULL, "allocate test encoder");
  encoder->pid = fork();
  check(encoder->pid >= 0, "fork test encoder");
  if (encoder->pid == 0) {
    for (;;) pause();
  }
  close(pipefd[0]);
  encoder->pipefd = pipefd[1];
  pid_t child = encoder->pid;
  check(kill(child, SIGKILL) == 0, "terminate test encoder");
  check(ffmpeg_close(encoder) == -1, "signal termination is failure");
  check(waitpid(child, NULL, WNOHANG) == -1 && errno == ECHILD, "failed encoder is reaped");
  check(ffmpeg_close(NULL) == -1, "NULL encoder is failure");

  check(unlink(raw) == 0, "remove raw fixture");
  check(fclose(log.file) == 0 && rmdir(directory) == 0, "clean test resources");
  puts("Render failure regression tests passed.");
  return 0;
}
