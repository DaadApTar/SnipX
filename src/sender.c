#include "sender.h"
#include "log.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

int send_video(logger logger, char *address, char *port, char *filepath) {
  int sender_pipe[2];
  char *options[9] = {0};
  int options_index = 0;
  char buffer[256] = {0};
  int status;
  int count;
  options[options_index++] = "snipx-sender";
  if (address != 0) {
    options[options_index++] = "-a";
    options[options_index++] = address;
  }
  if (port != 0) {
    options[options_index++] = "-p";
    options[options_index++] = port;
  }
  if (filepath != 0) {
    options[options_index++] = "-f";
    options[options_index++] = filepath;
  }

  if (pipe(sender_pipe) < 0) {
    return -1;
  }

  pid_t pid = fork();
  if (pid < 0) {
    return -1;
  }

  if (pid == 0) {
    dup2(sender_pipe[1], STDOUT_FILENO);
    close(sender_pipe[0]);
    close(sender_pipe[1]);

    log_print(&logger, LOG_INFO, "[CHILD %d] Sending file.\n", getpid());

    execvp("snipx-sender", options);
    perror("execvp failed");
    exit(127);
  }
  else {
    close(sender_pipe[1]);
    log_print(&logger, LOG_INFO, "[PARENT %d] Reading STDOUT.\n", getpid());

    while ((count = read(sender_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
      buffer[count] = '\0';
    }

    close(sender_pipe[0]);

    if (waitpid(pid, &status, 0) == 01) {
      return -1;
    }
    if (WIFEXITED(status)) {
      int exit_code = WEXITSTATUS(status);
      log_print(&logger, exit_code == 1 ? LOG_WARNING : exit_code == 0 ? LOG_INFO : LOG_ERROR, "%s", buffer);
      log_print(&logger, exit_code == 1 ? LOG_WARNING : exit_code == 0 ? LOG_INFO : LOG_ERROR, "Sender exited with code %d\n", exit_code);

      if (exit_code == 0) {
        delete_video(filepath);
      }
    }
    else return -1;
  }

  return 0;
}

int delete_video(char *filepath) {
  return 0;
}
