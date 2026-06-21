#ifndef _LVGL_DISPLAY_H__
#define _LVGL_DISPLAY_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lv_port.h"

#define LCD_WIDTH       240
#define LCD_HEIGHT      284

#define DEFAULT_WIFI_SSID           "wzdwifi"
#define DEFAULT_WIFI_PASSWORD       "17733196157"

extern EventGroupHandle_t event_group;
extern EventGroupHandle_t lvgl_runtime_event_group;

#define LVGL_RT_BIT_TASK_READY   BIT0
#define LVGL_RT_BIT_PAUSED_ACK   BIT1
#define LVGL_RT_BIT_IN_HANDLER   BIT2

#define WIFI_CONNECTED_BIT   (1 << 0)
#define NOVEL_DISPLAY_BIT    (1 << 1)

void lvgl_diaplay_task(void *param);
void print_memory_info(const char *label);
void system_diag_snapshot(const char *reason);
void lvgl_runtime_health_tick(void);
bool lvgl_wait_for_pause_ack(uint32_t timeout_ms);
void lvgl_runtime_clear_pause_ack(void);
bool lvgl_is_owner_task(void);
bool lvgl_thread_guard(const char *tag);
void lvgl_mark_novel_swipe_back_guard(uint32_t hold_ms);
bool lvgl_should_block_novel_click(void);

extern TaskHandle_t g_lvgl_task_handle;
extern TaskHandle_t g_sntp_time_task_handle;
extern TaskHandle_t g_sntp_interval_task_handle;

typedef enum {
    LVGL_OTA_RESULT_UNKNOWN = 0,
    LVGL_OTA_RESULT_SUCCESS = 1,
    LVGL_OTA_RESULT_FAILED = 2,
    LVGL_OTA_RESULT_CANCELLED = 3,
} lvgl_ota_result_t;

typedef enum {
    LVGL_MSG_NONE = 0,
    LVGL_MSG_WIFI_CONNECTED,
    LVGL_MSG_WIFI_DISCONNECTED,
    LVGL_MSG_KEY_NEXT_PAGE,
    LVGL_MSG_KEY_PREV_PAGE,
    LVGL_MSG_KEY_CONFIRM,
    LVGL_MSG_KEY_UP,
    LVGL_MSG_KEY_DOWN,
    LVGL_MSG_OTA_STATUS,
    LVGL_MSG_OTA_PROGRESS,
    LVGL_MSG_OTA_COMPLETE,
    LVGL_MSG_NTP_SYNC_STATUS,
    LVGL_MSG_WEATHER_STATUS,
    LVGL_MSG_WEATHER_UPDATED,

    /* Request messages (cross-thread). */
    LVGL_MSG_NOVEL_LIST_REFRESH_REQ,
    LVGL_MSG_VIDEO_LIST_REFRESH_REQ,
    LVGL_MSG_NOVEL_OPEN_REQ,
    LVGL_MSG_NOVEL_CLOSE_REQ,
    LVGL_MSG_NOVEL_PAGE_SYNC_REQ,
    LVGL_MSG_NOVEL_PAGE_NEXT_REQ,
    LVGL_MSG_NOVEL_PAGE_PREV_REQ,
    LVGL_MSG_VIDEO_OPEN_REQ,
    LVGL_MSG_VIDEO_STOP_REQ,

    /* Result messages (worker -> LVGL). */
    LVGL_MSG_NOVEL_LIST_READY,
    LVGL_MSG_VIDEO_LIST_READY,
    LVGL_MSG_NOVEL_OPEN_READY,
    LVGL_MSG_NOVEL_OPEN_ERROR,
    LVGL_MSG_NOVEL_PAGE_READY,
    LVGL_MSG_VIDEO_OPEN_READY,
    LVGL_MSG_VIDEO_OPEN_ERROR,
    LVGL_MSG_NOVEL_CLOSE_DONE,

    /* Legacy ids kept for compatibility. */
    LVGL_MSG_NOVEL_OPEN,
    LVGL_MSG_NOVEL_OPEN_DONE,

    LVGL_MSG_VIDEO_FRAME,
    LVGL_MSG_RETURN_TO_VIDEO_LIST,
} lvgl_msg_type_t;

typedef struct {
    lvgl_msg_type_t type;
    int32_t         param;
    union {
        void       *data;
        char        str_data[128];
    };
    bool            need_free;
} lvgl_msg_t;

extern QueueHandle_t lvgl_msg_queue;

void lvgl_msg_queue_init(void);
bool lvgl_msg_send(lvgl_msg_type_t type, int32_t param, const void *data);
BaseType_t lvgl_msg_send_nonblocking(lvgl_msg_type_t type, int32_t param, const void *data);

extern void *g_video_frame_mutex;

#include "esp_heap_caps.h"
void *lvgl_psram_alloc(size_t size);
void  lvgl_psram_free(void *p);
void *lvgl_psram_realloc(void *p, size_t new_size);

#undef LV_MEM_CUSTOM_ALLOC
#undef LV_MEM_CUSTOM_FREE
#undef LV_MEM_CUSTOM_REALLOC
#define LV_MEM_CUSTOM_ALLOC   lvgl_psram_alloc
#define LV_MEM_CUSTOM_FREE    lvgl_psram_free
#define LV_MEM_CUSTOM_REALLOC lvgl_psram_realloc

#endif
