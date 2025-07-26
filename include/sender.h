#ifndef SENDER_H_
#define SENDER_H_

#include <sys/types.h>
#include "log.h"
typedef struct {
  pid_t pid;
  int pipefd;
} sender;

/** @brief Sends a video and deletes. Prints address to file or error.
 *  @param[in] logger instance of logger.
 *  @param[in] address address to server.
 *  @param[in] port server port.
 *  @param[in] filepath filepath.
 *  @return 0 if succeed, -1 on error.
 */
int send_video(logger logger, char *address, char *port, char *filepath);

/** @brief Deletes video.
 *  @param[in] filepath filepath.
 *  @return 0 if succeed, -1 on error.
 */
int delete_video(char *filepath);
#endif
