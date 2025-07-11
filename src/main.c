#include "options.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>

#define ERROR(x) do {                                 \
                   fprintf(stderr, "ERROR: %s\n", x); \
                   exit(1);                           \
                 } while(0)
#define ERROR_ERNO ERROR(strerror(errno))

#define DEFAULT_PORT 4226
#define SOCKET_BUFFER_SIZE 2
#define STOP_COMMAND {0xFA, 0xDE}

int main(int argc, char **argv) {
  char* program = *(argv++);
  options *opts = parse_flags(argv, argc-1);

  // Brake is highest priority task
  if (opts->brake) {
    // TODO: connect to socket and send stop command
    return 0;
  }

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    ERROR_ERNO;
  }

  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)));

  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port   = htons(DEFAULT_PORT),
    .sin_addr   = INADDR_ANY
  };
  socklen_t addrlen = sizeof(addr);

  if(bind(socket_fd, (struct sockaddr *)&addr, addrlen) < 0) {
    ERROR_ERNO;
  }

  if (listen(socket_fd, 10) < 0) {
    ERROR_ERNO;
  }

  bool is_stopped = false;
  uint8_t socket_buffer[SOCKET_BUFFER_SIZE] = {0};
  uint8_t command[] = STOP_COMMAND;

  int accept_fd;
  while (!is_stopped) {
    if((accept_fd = accept(socket_fd, (struct sockaddr *)&addr, &addrlen)) < 0) {
      ERROR_ERNO;
    }
    int bytes_read = read(accept_fd, socket_buffer, SOCKET_BUFFER_SIZE);
    if (bytes_read < 0) {
      ERROR_ERNO;
    }
    if (memcmp(socket_buffer, command, 2) == 0) {
      is_stopped = true;
    }
    memset(socket_buffer, 0, SOCKET_BUFFER_SIZE);
    close(accept_fd);
  }

  close(socket_fd);

  return 0;
}
