#ifndef VIDEO_PLAYER_H_
#define VIDEO_PLAYER_H_

#include <stdbool.h>
#include <stdint.h>
#include "lvgl.h"

typedef enum {
    VIDEO_FORMAT_MJPEG,
    VIDEO_FORMAT_RGB565,
} video_format_t;

typedef struct {
    uint32_t frame_drop_count;
    uint32_t stop_timeout_count;
    uint32_t stop_wait_last_ms;
    uint32_t stop_wait_max_ms;
} video_player_diag_t;

void video_player_start(const char *filepath, video_format_t format);
void video_player_stop(void);
bool video_player_stop_async(void);
bool video_player_stop_wait(uint32_t timeout_ms);
void video_player_pause(void);
void video_player_resume(void);
void video_player_toggle_pause(void);
bool video_player_is_playing(void);

const lv_img_dsc_t *video_player_get_frame_desc(uint8_t frame_idx);
void video_player_mark_frame_presented(uint8_t frame_idx);
void video_player_get_diag(video_player_diag_t *out);

#endif
