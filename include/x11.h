#ifndef X11_H_
#define X11_H_

#include "recorder.h"
#include <X11/X.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

typedef struct {
  video_capture capture;
  Display *display;
  Window window;
  XImage *shared_image;
  logger *logger;
  unsigned int xinerama_screen_number;
  XShmSegmentInfo shminfo;
} snipx_x11;

/** @brief Initialies X11 display and window.
 *  @param[in] logger logger.
 *  @return X11 context.
 */
snipx_x11 init_x11(logger *logger);

/** @brief Initialises xinerama.
 *  @param[in] x11 X11 context.
 *  @return 0 on success, -1 on error.
 */
int init_xinerama(snipx_x11 x11);

/** @brief Sets coordinates of selected screen.
 *  @param[out] x11 X11 context.
 *  @param[in] screen_number index of screen to record
 *  @return 0 on success, -1 on error.
 */
int get_screen_data(snipx_x11 *x11, int screen_number);

/** @brief Initialises XShm extension and shared image.
 *  @param[out] x11 X11 context.
 *  @return 0 on success, -1 on error.
 */
int init_xshm(snipx_x11 *x11);

void free_x11(snipx_x11 *x11);

#endif
