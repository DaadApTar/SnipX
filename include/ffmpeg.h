#ifndef FFMPEG_H_
#define FFMPEG_H_

#include <sys/types.h>
#include "deferred_render.h"
typedef struct {
  pid_t pid;
  int pipefd;
} ffmpeg;

/** @brief Inits a ffmpeg instance for sound.
 *  @return Pointer to ffmpeg instance if succeed, NULL on error.
 */
ffmpeg *ffmpeg_init_sound(char *soundname);
/** @brief Inits a ffmpeg instance for video and sound.
 *  @return Pointer to ffmpeg instance if succeed, NULL on error.
 */
ffmpeg *ffmpeg_init_video(char *videoname, int screen_width, int screen_height, int fps, int bitrate, unsigned int sound_files_amount, char **filenames);

ffmpeg *ffmpeg_render_video(char *videoname, int screen_width, int screen_height, int fps, int bitrate, video_components components);

/** @brief Pushes frame into ffmpeg output file.
 *  @param[in] instance ffmpeg instance.
 *  @param[in] frame frame for pushing.
 *  @return 0 if succeed, -1 on error.
 */
int ffmpeg_push_frame(ffmpeg *instance, void *data, ssize_t size);

/** @brief Closes fffmpeg instance.
 *  @param[in] instance ffmpeg instance.
 *  @return 0 if succeed, -1 on error.
 */
int ffmpeg_close(ffmpeg *instance);

#endif
