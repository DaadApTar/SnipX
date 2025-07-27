#ifndef SENDER_H_
#define SENDER_H_

#include <sys/types.h>
#include "log.h"
typedef struct {
  logger logger;
  char *address;
  char *port;
  char *path;
} sender_params;

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

/** @brief Sends videos from temp directory which weren't sent before.
 *  @param[in] logger instance of logger.
 *  @param[in] address address to server.
 *  @param[in] port server port.
 *  @param[in] path path to temp dir.
 *  @return 0 if succeed, -1 on error.
 */
int send_temp_files(logger logger, char *address, char *port, char* path);

/** @brief Thread sends videos from temp directory which weren't sent before.
 *  @param[in] args #sender_params.
 */
void *thread_send_temp_files(void *args);
#endif
