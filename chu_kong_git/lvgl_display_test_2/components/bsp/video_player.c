#include "video_player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "img_converters.h"
#include "lvgl_display.h"
#include "mjpeg_frame.h"

#define TAG "VIDEO_PLAYER"

#define VIDEO_WIDTH    240
#define VIDEO_HEIGHT   284
#define FRAME_SIZE     (VIDEO_WIDTH * VIDEO_HEIGHT * 2)
#define VIDEO_BUF_CNT  3

typedef enum {
    FRAME_BUF_FREE = 0,
    FRAME_BUF_FILLING,
    FRAME_BUF_READY,
    FRAME_BUF_FRONT,
} frame_buf_state_t;

static TaskHandle_t s_video_task_handle = NULL;
static SemaphoreHandle_t s_video_exit_sem = NULL;
static SemaphoreHandle_t s_ctrl_mutex = NULL;
static SemaphoreHandle_t s_state_mutex = NULL;

static volatile bool s_video_exit = false;
static volatile bool s_video_paused = false;
static volatile bool s_video_running = false;
static video_format_t s_current_format = VIDEO_FORMAT_RGB565;

static uint8_t *s_frame_buf[VIDEO_BUF_CNT] = {NULL};
static lv_img_dsc_t s_img_dsc[VIDEO_BUF_CNT];
static frame_buf_state_t s_buf_state[VIDEO_BUF_CNT] = {0};
static int s_front_idx = -1;

static video_player_diag_t s_diag = {0};

/* Shared symbol kept for compatibility with existing code. */
void *g_video_frame_mutex = NULL;

static inline void ctrl_lock(void)
{
    if (s_ctrl_mutex != NULL) {
        xSemaphoreTake(s_ctrl_mutex, portMAX_DELAY);
    }
}

static inline void ctrl_unlock(void)
{
    if (s_ctrl_mutex != NULL) {
        xSemaphoreGive(s_ctrl_mutex);
    }
}

static inline void state_lock(void)
{
    if (s_state_mutex != NULL) {
        xSemaphoreTake(s_state_mutex, portMAX_DELAY);
    }
}

static inline void state_unlock(void)
{
    if (s_state_mutex != NULL) {
        xSemaphoreGive(s_state_mutex);
    }
}

static int read_one_rgb565_frame(FILE *fp, uint8_t *rgb565_buf)
{
    size_t bytes = fread(rgb565_buf, 1, FRAME_SIZE, fp);
    return (bytes == FRAME_SIZE) ? 0 : -1;
}

static void init_frame_meta(void)
{
    for (int i = 0; i < VIDEO_BUF_CNT; ++i) {
        s_img_dsc[i].header.always_zero = 0;
        s_img_dsc[i].header.w = VIDEO_WIDTH;
        s_img_dsc[i].header.h = VIDEO_HEIGHT;
        s_img_dsc[i].header.cf = LV_IMG_CF_TRUE_COLOR;
        s_img_dsc[i].data_size = FRAME_SIZE;
        s_img_dsc[i].data = s_frame_buf[i];
        s_buf_state[i] = FRAME_BUF_FREE;
    }
    s_front_idx = -1;
}

static bool alloc_buffers(void)
{
    for (int i = 0; i < VIDEO_BUF_CNT; ++i) {
        if (s_frame_buf[i] == NULL) {
            s_frame_buf[i] = heap_caps_malloc(FRAME_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (s_frame_buf[i] == NULL) {
                ESP_LOGE(TAG, "frame buffer alloc failed idx=%d", i);
                return false;
            }
        }
    }
    init_frame_meta();
    return true;
}

static int acquire_fill_buffer(void)
{
    int idx = -1;
    state_lock();
    for (int i = 0; i < VIDEO_BUF_CNT; ++i) {
        if (s_buf_state[i] == FRAME_BUF_FREE) {
            s_buf_state[i] = FRAME_BUF_FILLING;
            idx = i;
            break;
        }
    }
    state_unlock();
    return idx;
}

static void release_fill_buffer(int idx)
{
    if (idx < 0 || idx >= VIDEO_BUF_CNT) return;
    state_lock();
    if (s_buf_state[idx] == FRAME_BUF_FILLING) {
        s_buf_state[idx] = FRAME_BUF_FREE;
    }
    state_unlock();
}

static void publish_filled_frame(int idx)
{
    bool queued = false;

    if (idx < 0 || idx >= VIDEO_BUF_CNT) return;

    state_lock();
    if (s_buf_state[idx] == FRAME_BUF_FILLING) {
        s_buf_state[idx] = FRAME_BUF_READY;
        queued = (lvgl_msg_send_nonblocking(LVGL_MSG_VIDEO_FRAME, idx, NULL) == pdTRUE);
        if (!queued) {
            s_buf_state[idx] = FRAME_BUF_FREE;
        }
    }
    state_unlock();

    if (!queued) {
        s_diag.frame_drop_count++;
    }
}

static void reset_frame_states(void)
{
    state_lock();
    for (int i = 0; i < VIDEO_BUF_CNT; ++i) {
        s_buf_state[i] = FRAME_BUF_FREE;
    }
    s_front_idx = -1;
    state_unlock();
}

static void video_task(void *param)
{
    char *filepath = (char *)param;

    if (filepath == NULL) {
        goto cleanup;
    }

    if (s_current_format == VIDEO_FORMAT_RGB565) {
        FILE *fp = fopen(filepath, "rb");
        if (fp == NULL) {
            ESP_LOGE(TAG, "cannot open file: %s", filepath);
            goto cleanup;
        }

        while (!s_video_exit) {
            if (s_video_paused) {
                vTaskDelay(pdMS_TO_TICKS(30));
                continue;
            }

            int idx = acquire_fill_buffer();
            if (idx < 0) {
                s_diag.frame_drop_count++;
                vTaskDelay(1);
                continue;
            }

            if (read_one_rgb565_frame(fp, s_frame_buf[idx]) != 0) {
                release_fill_buffer(idx);
                fseek(fp, 0, SEEK_SET);
                continue;
            }

            publish_filled_frame(idx);
            vTaskDelay(1);
        }
        fclose(fp);
    } else {
        jpeg_frame_cfg_t cfg = {.buff_size = 80 * 1024};
        jpeg_frame_config(&cfg);
        jpeg_frame_start(filepath);

        TickType_t last_frame_tick = xTaskGetTickCount();
        const TickType_t target_delay = pdMS_TO_TICKS(33); /* ~30 FPS */

        while (!s_video_exit) {
            if (s_video_paused) {
                vTaskDelay(pdMS_TO_TICKS(30));
                last_frame_tick = xTaskGetTickCount();
                continue;
            }

            int idx = acquire_fill_buffer();
            if (idx < 0) {
                s_diag.frame_drop_count++;
                vTaskDelay(1);
                continue;
            }

            jpeg_frame_data_t j_data = {0};
            jpeg_frame_get_one(&j_data);
            if (j_data.len == 0 || j_data.frame == NULL) {
                release_fill_buffer(idx);
                vTaskDelay(pdMS_TO_TICKS(8));
            } else {
                uint16_t w = 0;
                uint16_t h = 0;
                uint8_t *out_ptr = s_frame_buf[idx];
                bool ok = jpg2rgb565(j_data.frame, j_data.len, &out_ptr, &w, &h, JPG_SCALE_NONE);
                free(j_data.frame);

                uint32_t rgb565_size = (uint32_t)w * (uint32_t)h * 2U;
                if (!ok || out_ptr != s_frame_buf[idx] || w == 0 || h == 0 || rgb565_size > FRAME_SIZE) {
                    release_fill_buffer(idx);
                    s_diag.frame_drop_count++;
                } else {
                    s_img_dsc[idx].header.w = w;
                    s_img_dsc[idx].header.h = h;
                    s_img_dsc[idx].data_size = rgb565_size;
                    publish_filled_frame(idx);
                }
            }

            TickType_t now = xTaskGetTickCount();
            TickType_t elapsed = now - last_frame_tick;
            if (elapsed < target_delay) {
                vTaskDelay(target_delay - elapsed);
            }
            last_frame_tick = xTaskGetTickCount();
        }
        jpeg_frame_stop();
    }

cleanup:
    reset_frame_states();
    s_video_running = false;
    s_video_task_handle = NULL;

    if (s_video_exit_sem != NULL) {
        xSemaphoreGive(s_video_exit_sem);
    }
    free(filepath);
    vTaskDelete(NULL);
}

void video_player_start(const char *filepath, video_format_t format)
{
    if (filepath == NULL) return;

    if (s_ctrl_mutex == NULL) {
        s_ctrl_mutex = xSemaphoreCreateMutex();
        if (s_ctrl_mutex == NULL) {
            ESP_LOGE(TAG, "ctrl mutex create failed");
            return;
        }
    }
    ctrl_lock();

    if (s_state_mutex == NULL) {
        s_state_mutex = xSemaphoreCreateMutex();
        if (s_state_mutex == NULL) {
            ESP_LOGE(TAG, "state mutex create failed");
            ctrl_unlock();
            return;
        }
        g_video_frame_mutex = s_state_mutex;
    }
    if (s_video_exit_sem == NULL) {
        s_video_exit_sem = xSemaphoreCreateBinary();
        if (s_video_exit_sem == NULL) {
            ESP_LOGE(TAG, "exit sem create failed");
            ctrl_unlock();
            return;
        }
    }

    if (s_video_running || s_video_task_handle != NULL) {
        ctrl_unlock();
        return;
    }

    if (!alloc_buffers()) {
        ctrl_unlock();
        return;
    }

    if (s_video_exit_sem != NULL) {
        while (xSemaphoreTake(s_video_exit_sem, 0) == pdTRUE) {
        }
    }

    char *path_copy = strdup(filepath);
    if (path_copy == NULL) {
        ctrl_unlock();
        return;
    }

    s_current_format = format;
    s_video_exit = false;
    s_video_paused = false;
    s_video_running = true;

    if (xTaskCreate(video_task, "video_task", 8192, path_copy, 5, &s_video_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "create video_task failed");
        s_video_running = false;
        s_video_task_handle = NULL;
        free(path_copy);
    }

    ctrl_unlock();
}

bool video_player_stop_async(void)
{
    bool was_running = false;

    ctrl_lock();
    was_running = (s_video_running || s_video_task_handle != NULL);
    if (was_running) {
        s_video_exit = true;
    }
    ctrl_unlock();

    return was_running;
}

bool video_player_stop_wait(uint32_t timeout_ms)
{
    bool need_wait = video_player_stop_async();
    if (!need_wait) {
        return true;
    }

    if (s_video_exit_sem == NULL) {
        return false;
    }

    TickType_t t0 = xTaskGetTickCount();
    bool ok = (xSemaphoreTake(s_video_exit_sem, pdMS_TO_TICKS(timeout_ms)) == pdTRUE);
    TickType_t t1 = xTaskGetTickCount();
    uint32_t wait_ms = (uint32_t)((t1 - t0) * portTICK_PERIOD_MS);

    s_diag.stop_wait_last_ms = wait_ms;
    if (wait_ms > s_diag.stop_wait_max_ms) {
        s_diag.stop_wait_max_ms = wait_ms;
    }
    if (!ok) {
        s_diag.stop_timeout_count++;
        ESP_LOGW(TAG, "video stop wait timeout (%lu ms)", (unsigned long)timeout_ms);
    }
    return ok;
}

void video_player_stop(void)
{
    (void)video_player_stop_wait(3000);
}

void video_player_pause(void)
{
    s_video_paused = true;
}

void video_player_resume(void)
{
    s_video_paused = false;
}

void video_player_toggle_pause(void)
{
    s_video_paused = !s_video_paused;
}

bool video_player_is_playing(void)
{
    return s_video_running;
}

const lv_img_dsc_t *video_player_get_frame_desc(uint8_t frame_idx)
{
    if (frame_idx >= VIDEO_BUF_CNT) return NULL;
    return &s_img_dsc[frame_idx];
}

void video_player_mark_frame_presented(uint8_t frame_idx)
{
    if (frame_idx >= VIDEO_BUF_CNT) return;

    state_lock();
    if (s_buf_state[frame_idx] == FRAME_BUF_READY || s_buf_state[frame_idx] == FRAME_BUF_FILLING) {
        if (s_front_idx >= 0 && s_front_idx < VIDEO_BUF_CNT && s_front_idx != frame_idx) {
            s_buf_state[s_front_idx] = FRAME_BUF_FREE;
        }
        s_buf_state[frame_idx] = FRAME_BUF_FRONT;
        s_front_idx = frame_idx;
    }
    state_unlock();
}

void video_player_get_diag(video_player_diag_t *out)
{
    if (out == NULL) return;
    *out = s_diag;
}
