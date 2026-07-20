#include "x11.h"
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>

snipx_x11 init_x11(logger *logger) {
  snipx_x11 res;
  res.logger = logger;
  res.display = XOpenDisplay(0);
  res.window = DefaultRootWindow(res.display);

  return res;
}

int init_xinerama(snipx_x11 x11) {
  int xinerama_minor, xinerama_major;
  if (!XineramaQueryExtension(x11.display, &xinerama_minor, &xinerama_major)) {
    log_print(x11.logger, LOG_ERROR, "Xinerama is not supported.\n");
    return -1;
  }
  if (!XineramaIsActive(x11.display)) {
    log_print(x11.logger, LOG_ERROR, "Xinerama is not active.\n");
    return -1;
  }

  log_print(x11.logger, LOG_DEBUG, "Xinerama version: %d.%d\n", xinerama_major, xinerama_minor);

  return 0;
}

int get_screen_data(snipx_x11 *x11, int screen_number) {
  int num_screens = 0;
  log_print(x11->logger, LOG_INFO, "Getting existing screens.\n");
  XineramaScreenInfo *screens = XineramaQueryScreens(x11->display, &num_screens);
  if (screen_number > num_screens - 1) {
    log_print(x11->logger, LOG_ERROR, "Invalid screen number.\n");
    return 0;
  }

  XineramaScreenInfo screen_info = screens[screen_number];
  x11->capture.screen_x = screen_info.x_org;
  x11->capture.screen_y = screen_info.y_org;
  x11->capture.screen_width = screen_info.width;
  x11->capture.screen_height = screen_info.height;
  x11->xinerama_screen_number = screen_info.screen_number;
  log_print(x11->logger, LOG_DEBUG, "Screen x: %d, y: %d, width: %d, height: %d.\n", x11->capture.screen_x, x11->capture.screen_y, x11->capture.screen_width, x11->capture.screen_height);

  return 1;
}

int init_xshm(snipx_x11 *x11) {
  int shm_minor, shm_major;
  Bool pixmaps;
  if (!XShmQueryVersion(x11->display, &shm_minor, &shm_major, &pixmaps)) {
    log_print(x11->logger, LOG_ERROR, "Xshm is not supported.\n");
    return -1;
  }

  log_print(x11->logger, LOG_DEBUG, "Xshm version: %d.%d with shared pixmaps support: %s\n", shm_major, shm_minor, pixmaps ? "ON" : "OFF");

  int depth = DefaultDepth(x11->display, x11->xinerama_screen_number);
  Visual *visual = DefaultVisual(x11->display, x11->xinerama_screen_number);
  x11->shared_image = XShmCreateImage(x11->display, visual, depth, ZPixmap, 0, &x11->shminfo, x11->capture.screen_width, x11->capture.screen_height);

  if (!x11->shared_image) {
    log_print(x11->logger, LOG_ERROR, "Cannot create shared image.\n");
    return -1;
  }

  x11->shminfo.shmid = shmget(IPC_PRIVATE, x11->shared_image->bytes_per_line * x11->shared_image->height, IPC_CREAT|0777);

  if (x11->shminfo.shmid < 0) {
    log_print(x11->logger, LOG_ERROR, "Cannot get shmid: %s.\n", strerror(errno));
    return -1;
  }

  x11->shared_image->data = (char *)shmat(x11->shminfo.shmid, 0, 0);
  x11->shminfo.shmaddr = x11->shared_image->data;
  x11->shminfo.readOnly = False;

  if (!XShmAttach(x11->display, &x11->shminfo)) {
    log_print(x11->logger, LOG_ERROR, "Cannot attach shared memory segment.\n");
    return -1;
  }
  XSync(x11->display, False);

  return 0;
}

void free_x11(snipx_x11 *x11) {
  XShmDetach(x11->display, &x11->shminfo);
  XDestroyImage(x11->shared_image);
  shmdt(x11->shminfo.shmaddr);
  shmctl(x11->shminfo.shmid, IPC_RMID, 0);
}
