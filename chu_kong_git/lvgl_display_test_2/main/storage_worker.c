#include "storage_worker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "SD_card.h"
#include "novel_progress.h"
#include "lvgl_display.h"

#define TAG "STORAGE_WORKER"

#define STORAGE_QUEUE_LEN        12
#define STORAGE_CACHE_MAX        96
#define STORAGE_PAGE_TEXT_MAX    1200
#define STORAGE_WORKER_STACK_SIZE 12288

typedef enum {
    STORAGE_CMD_SCAN_NOVELS = 0,
    STORAGE_CMD_SCAN_VIDEOS,
    STORAGE_CMD_NOVEL_OPEN_NAME,
    STORAGE_CMD_NOVEL_OPEN_PATH,
    STORAGE_CMD_NOVEL_CLOSE,
    STORAGE_CMD_PREPARE_SLEEP,
    STORAGE_CMD_NOVEL_PAGE_SYNC,
    STORAGE_CMD_NOVEL_PAGE_NEXT,
    STORAGE_CMD_NOVEL_PAGE_PREV,
    STORAGE_CMD_VIDEO_RESOLVE_NAME,
} storage_cmd_type_t;

typedef struct {
    storage_cmd_type_t type;
    char arg[STORAGE_WORKER_PATH_MAX];
} storage_cmd_t;

extern FILE *fp;
extern long g_file_offset;
extern char display_buf[1200];

static QueueHandle_t s_cmd_queue = NULL;
static TaskHandle_t s_task_handle = NULL;
static SemaphoreHandle_t s_data_mutex = NULL;
static SemaphoreHandle_t s_sleep_prep_sem = NULL;

static storage_file_entry_t *s_novel_cache = NULL;
static size_t s_novel_cache_count = 0;
static size_t s_novel_cache_cap = 0;
static storage_file_entry_t *s_video_cache = NULL;
static size_t s_video_cache_count = 0;
static size_t s_video_cache_cap = 0;
static storage_file_entry_t *s_scan_tmp = NULL;
static size_t s_scan_tmp_cap = 0;

static char s_active_novel_path[STORAGE_WORKER_PATH_MAX] = {0};
static char s_last_page_text[STORAGE_PAGE_TEXT_MAX] = {0};
static long s_last_page_offset = 0;

static char s_last_video_path[STORAGE_WORKER_PATH_MAX] = {0};
static video_format_t s_last_video_format = VIDEO_FORMAT_MJPEG;

static inline void lock_data(void)
{
    if (s_data_mutex != NULL) {
        xSemaphoreTake(s_data_mutex, portMAX_DELAY);
    }
}

static inline void unlock_data(void)
{
    if (s_data_mutex != NULL) {
        xSemaphoreGive(s_data_mutex);
    }
}

static bool ext_match(const char *name, const char *filters)
{
    if (name == NULL) return false;
    if (filters == NULL || filters[0] == '\0') return true;

    const char *ext = strrchr(name, '.');
    if (ext == NULL) return false;

    char token[24] = {0};
    const char *p = filters;
    while (*p != '\0') {
        size_t n = 0;
        while (*p != '\0' && *p != '|' && n < sizeof(token) - 1) {
            token[n++] = *p++;
        }
        token[n] = '\0';
        if (token[0] != '\0' && strcasecmp(token, ext) == 0) {
            return true;
        }
        if (*p == '|') p++;
    }
    return false;
}

static int scan_dir(const char *dir_path, const char *filters, storage_file_entry_t *out, size_t cap)
{
    if (dir_path == NULL || out == NULL || cap == 0) return -1;

    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        ESP_LOGW(TAG, "open dir failed: %s", dir_path);
        return -1;
    }

    size_t count = 0;
    struct dirent *entry = NULL;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (entry->d_type == DT_DIR) continue;
        if (!ext_match(entry->d_name, filters)) continue;
        if (count >= cap) break;

        strncpy(out[count].name, entry->d_name, sizeof(out[count].name) - 1);
        out[count].name[sizeof(out[count].name) - 1] = '\0';
        snprintf(out[count].full_path, sizeof(out[count].full_path), "%s/%s", dir_path, entry->d_name);
        count++;
    }

    closedir(dir);
    return (int)count;
}

static void set_novel_cache(const storage_file_entry_t *src, size_t count)
{
    lock_data();
    if (s_novel_cache == NULL || s_novel_cache_cap == 0) {
        s_novel_cache_count = 0;
        unlock_data();
        return;
    }
    s_novel_cache_count = (count > s_novel_cache_cap) ? s_novel_cache_cap : count;
    if (src != NULL && s_novel_cache_count > 0) {
        memcpy(s_novel_cache, src, s_novel_cache_count * sizeof(storage_file_entry_t));
    }
    unlock_data();
}

static void set_video_cache(const storage_file_entry_t *src, size_t count)
{
    lock_data();
    if (s_video_cache == NULL || s_video_cache_cap == 0) {
        s_video_cache_count = 0;
        unlock_data();
        return;
    }
    s_video_cache_count = (count > s_video_cache_cap) ? s_video_cache_cap : count;
    if (src != NULL && s_video_cache_count > 0) {
        memcpy(s_video_cache, src, s_video_cache_count * sizeof(storage_file_entry_t));
    }
    unlock_data();
}

static bool resolve_from_cache(bool novel, const char *name, char *out_path, size_t out_len)
{
    bool found = false;
    if (name == NULL || out_path == NULL || out_len == 0) return false;

    lock_data();
    if (novel) {
        for (size_t i = 0; s_novel_cache != NULL && i < s_novel_cache_count; ++i) {
            if (strcmp(s_novel_cache[i].name, name) == 0) {
                snprintf(out_path, out_len, "%s", s_novel_cache[i].full_path);
                found = true;
                break;
            }
        }
    } else {
        for (size_t i = 0; s_video_cache != NULL && i < s_video_cache_count; ++i) {
            if (strcmp(s_video_cache[i].name, name) == 0) {
                snprintf(out_path, out_len, "%s", s_video_cache[i].full_path);
                found = true;
                break;
            }
        }
    }
    unlock_data();
    return found;
}

static video_format_t detect_video_format(const char *path)
{
    const char *ext = (path != NULL) ? strrchr(path, '.') : NULL;
    if (ext != NULL) {
        if (strcasecmp(ext, ".bin") == 0 || strcasecmp(ext, ".rgb565") == 0) {
            return VIDEO_FORMAT_RGB565;
        }
    }
    return VIDEO_FORMAT_MJPEG;
}

static bool post_cmd(storage_cmd_type_t type, const char *arg, TickType_t wait_ticks)
{
    if (s_cmd_queue == NULL) return false;
    storage_cmd_t cmd = {.type = type};
    if (arg != NULL) {
        strncpy(cmd.arg, arg, sizeof(cmd.arg) - 1);
        cmd.arg[sizeof(cmd.arg) - 1] = '\0';
    }
    return (xQueueSend(s_cmd_queue, &cmd, wait_ticks) == pdTRUE);
}

static bool alloc_entry_block(storage_file_entry_t **out_ptr, size_t *out_cap, const char *label)
{
    static const size_t psram_caps[] = {STORAGE_CACHE_MAX, 64, 48, 32, 24, 16, 8};
    static const size_t internal_caps[] = {24, 16, 8};
    if (out_ptr == NULL || out_cap == NULL) return false;
    if (*out_ptr != NULL && *out_cap > 0) return true;

    for (size_t i = 0; i < sizeof(psram_caps) / sizeof(psram_caps[0]); ++i) {
        size_t cap = psram_caps[i];
        size_t bytes = cap * sizeof(storage_file_entry_t);
        storage_file_entry_t *buf =
            (storage_file_entry_t *)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (buf != NULL) {
            memset(buf, 0, bytes);
            *out_ptr = buf;
            *out_cap = cap;
            ESP_LOGI(TAG, "%s cache ready: cap=%u bytes=%u",
                     label ? label : "storage",
                     (unsigned int)cap,
                     (unsigned int)bytes);
            return true;
        }
    }

    for (size_t i = 0; i < sizeof(internal_caps) / sizeof(internal_caps[0]); ++i) {
        size_t cap = internal_caps[i];
        size_t bytes = cap * sizeof(storage_file_entry_t);
        storage_file_entry_t *buf =
            (storage_file_entry_t *)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
        if (buf != NULL) {
            memset(buf, 0, bytes);
            *out_ptr = buf;
            *out_cap = cap;
            ESP_LOGW(TAG, "%s cache fallback internal: cap=%u bytes=%u",
                     label ? label : "storage",
                     (unsigned int)cap,
                     (unsigned int)bytes);
            return true;
        }
    }

    ESP_LOGE(TAG, "%s cache alloc failed", label ? label : "storage");
    return false;
}

static void publish_novel_page_result(void)
{
    lock_data();
    strncpy(s_last_page_text, display_buf, sizeof(s_last_page_text) - 1);
    s_last_page_text[sizeof(s_last_page_text) - 1] = '\0';
    s_last_page_offset = g_file_offset;
    unlock_data();
    lvgl_msg_send_nonblocking(LVGL_MSG_NOVEL_PAGE_READY, 0, NULL);
}

static void handle_novel_open(const char *path)
{
    if (path == NULL || path[0] == '\0') {
        lvgl_msg_send_nonblocking(LVGL_MSG_NOVEL_OPEN_ERROR, 0, NULL);
        return;
    }

    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }
    fp = fopen(path, "r");
    if (fp == NULL) {
        ESP_LOGE(TAG, "open novel failed: %s", path);
        lvgl_msg_send_nonblocking(LVGL_MSG_NOVEL_OPEN_ERROR, 0, NULL);
        return;
    }

    g_file_offset = 0;
    novel_progress_load(path, &g_file_offset);

    lock_data();
    snprintf(s_active_novel_path, sizeof(s_active_novel_path), "%s", path);
    unlock_data();

    lvgl_msg_send_nonblocking(LVGL_MSG_NOVEL_OPEN_READY, 0, NULL);
}

static void close_active_novel(bool notify_ui)
{
    if (fp != NULL) {
        char path_copy[STORAGE_WORKER_PATH_MAX] = {0};
        lock_data();
        snprintf(path_copy, sizeof(path_copy), "%s", s_active_novel_path);
        unlock_data();
        if (path_copy[0] != '\0') {
            novel_progress_save(path_copy, g_file_offset);
        }
        fclose(fp);
        fp = NULL;
    }
    lock_data();
    s_active_novel_path[0] = '\0';
    unlock_data();
    if (notify_ui) {
        lvgl_msg_send_nonblocking(LVGL_MSG_NOVEL_CLOSE_DONE, 0, NULL);
    }
}

static void handle_novel_close(void)
{
    close_active_novel(true);
}

static void handle_prepare_sleep(void)
{
    close_active_novel(false);
    if (s_sleep_prep_sem != NULL) {
        xSemaphoreGive(s_sleep_prep_sem);
    }
}

static void handle_novel_page(storage_cmd_type_t type)
{
    if (fp == NULL) {
        lvgl_msg_send_nonblocking(LVGL_MSG_NOVEL_OPEN_ERROR, 0, NULL);
        return;
    }

    esp_err_t ret = ESP_FAIL;
    if (type == STORAGE_CMD_NOVEL_PAGE_SYNC) {
        ret = novel_read_at_offset(g_file_offset);
    } else if (type == STORAGE_CMD_NOVEL_PAGE_NEXT) {
        ret = novel_next_page();
    } else if (type == STORAGE_CMD_NOVEL_PAGE_PREV) {
        ret = novel_prev_page();
    }

    if (ret == ESP_OK || ret == ESP_ERR_NOT_FOUND) {
        publish_novel_page_result();
    } else {
        lvgl_msg_send_nonblocking(LVGL_MSG_NOVEL_OPEN_ERROR, 0, NULL);
    }
}

static void worker_task(void *param)
{
    (void)param;
    storage_cmd_t cmd = {0};
    uint32_t loop_count = 0;
    if (s_scan_tmp == NULL || s_scan_tmp_cap == 0) {
        ESP_LOGE(TAG, "scan buffer not ready");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        if (s_cmd_queue == NULL || xQueueReceive(s_cmd_queue, &cmd, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        loop_count++;
        if ((loop_count % 64U) == 0U) {
            UBaseType_t hwm = uxTaskGetStackHighWaterMark(NULL);
            if (hwm < 512U) {
                ESP_LOGW(TAG, "low stack headroom: %u bytes", (unsigned int)hwm);
            }
        }

        switch (cmd.type) {
            case STORAGE_CMD_SCAN_NOVELS: {
                int n = scan_dir("/sdcard/novels", ".txt", s_scan_tmp, s_scan_tmp_cap);
                if (n < 0) n = 0;
                set_novel_cache(s_scan_tmp, (size_t)n);
                lvgl_msg_send_nonblocking(LVGL_MSG_NOVEL_LIST_READY, n, NULL);
                break;
            }
            case STORAGE_CMD_SCAN_VIDEOS: {
                int n = scan_dir("/sdcard/videos", ".mjpeg|.avi|.mp4|.mpg|.mjp|.bin|.rgb565", s_scan_tmp, s_scan_tmp_cap);
                if (n < 0) n = 0;
                set_video_cache(s_scan_tmp, (size_t)n);
                lvgl_msg_send_nonblocking(LVGL_MSG_VIDEO_LIST_READY, n, NULL);
                break;
            }
            case STORAGE_CMD_NOVEL_OPEN_NAME: {
                char path[STORAGE_WORKER_PATH_MAX] = {0};
                if (!resolve_from_cache(true, cmd.arg, path, sizeof(path))) {
                    int n = scan_dir("/sdcard/novels", ".txt", s_scan_tmp, s_scan_tmp_cap);
                    if (n < 0) n = 0;
                    set_novel_cache(s_scan_tmp, (size_t)n);
                    if (!resolve_from_cache(true, cmd.arg, path, sizeof(path))) {
                        lvgl_msg_send_nonblocking(LVGL_MSG_NOVEL_OPEN_ERROR, 0, NULL);
                        break;
                    }
                }
                handle_novel_open(path);
                break;
            }
            case STORAGE_CMD_NOVEL_OPEN_PATH:
                handle_novel_open(cmd.arg);
                break;
            case STORAGE_CMD_NOVEL_CLOSE:
                handle_novel_close();
                break;
            case STORAGE_CMD_PREPARE_SLEEP:
                handle_prepare_sleep();
                break;
            case STORAGE_CMD_NOVEL_PAGE_SYNC:
            case STORAGE_CMD_NOVEL_PAGE_NEXT:
            case STORAGE_CMD_NOVEL_PAGE_PREV:
                handle_novel_page(cmd.type);
                break;
            case STORAGE_CMD_VIDEO_RESOLVE_NAME: {
                char path[STORAGE_WORKER_PATH_MAX] = {0};
                if (!resolve_from_cache(false, cmd.arg, path, sizeof(path))) {
                    int n = scan_dir("/sdcard/videos", ".mjpeg|.avi|.mp4|.mpg|.mjp|.bin|.rgb565", s_scan_tmp, s_scan_tmp_cap);
                    if (n < 0) n = 0;
                    set_video_cache(s_scan_tmp, (size_t)n);
                    if (!resolve_from_cache(false, cmd.arg, path, sizeof(path))) {
                        lvgl_msg_send_nonblocking(LVGL_MSG_VIDEO_OPEN_ERROR, 0, NULL);
                        break;
                    }
                }
                lock_data();
                snprintf(s_last_video_path, sizeof(s_last_video_path), "%s", path);
                s_last_video_format = detect_video_format(path);
                unlock_data();
                lvgl_msg_send_nonblocking(LVGL_MSG_VIDEO_OPEN_READY, 0, NULL);
                break;
            }
            default:
                break;
        }
    }
}

void storage_worker_init(void)
{
    if (s_cmd_queue == NULL) {
        s_cmd_queue = xQueueCreate(STORAGE_QUEUE_LEN, sizeof(storage_cmd_t));
    }
    if (s_data_mutex == NULL) {
        s_data_mutex = xSemaphoreCreateMutex();
    }
    if (s_sleep_prep_sem == NULL) {
        s_sleep_prep_sem = xSemaphoreCreateBinary();
    }
    if (s_novel_cache == NULL) {
        (void)alloc_entry_block(&s_novel_cache, &s_novel_cache_cap, "novel");
    }
    if (s_video_cache == NULL) {
        (void)alloc_entry_block(&s_video_cache, &s_video_cache_cap, "video");
    }
    if (s_scan_tmp == NULL) {
        (void)alloc_entry_block(&s_scan_tmp, &s_scan_tmp_cap, "scan");
    }
    if (s_cmd_queue == NULL || s_data_mutex == NULL || s_sleep_prep_sem == NULL ||
        s_novel_cache == NULL || s_video_cache == NULL || s_scan_tmp == NULL) {
        ESP_LOGE(TAG, "storage worker init incomplete");
        return;
    }
    if (s_task_handle == NULL && s_cmd_queue != NULL && s_data_mutex != NULL) {
        if (xTaskCreatePinnedToCore(worker_task, "storage_worker", STORAGE_WORKER_STACK_SIZE, NULL, 5, &s_task_handle, 0) != pdPASS) {
            s_task_handle = NULL;
            ESP_LOGE(TAG, "create storage worker failed");
        }
    }
}

bool storage_request_novel_list_refresh(void) { return post_cmd(STORAGE_CMD_SCAN_NOVELS, NULL, 0); }
bool storage_request_video_list_refresh(void) { return post_cmd(STORAGE_CMD_SCAN_VIDEOS, NULL, 0); }
bool storage_request_novel_open_by_name(const char *name) { return post_cmd(STORAGE_CMD_NOVEL_OPEN_NAME, name, pdMS_TO_TICKS(50)); }
bool storage_request_novel_open_by_path(const char *path) { return post_cmd(STORAGE_CMD_NOVEL_OPEN_PATH, path, pdMS_TO_TICKS(50)); }
bool storage_request_novel_close(void) { return post_cmd(STORAGE_CMD_NOVEL_CLOSE, NULL, pdMS_TO_TICKS(20)); }
bool storage_request_novel_page_sync(void) { return post_cmd(STORAGE_CMD_NOVEL_PAGE_SYNC, NULL, 0); }
bool storage_request_novel_page_next(void) { return post_cmd(STORAGE_CMD_NOVEL_PAGE_NEXT, NULL, 0); }
bool storage_request_novel_page_prev(void) { return post_cmd(STORAGE_CMD_NOVEL_PAGE_PREV, NULL, 0); }
bool storage_request_video_resolve_by_name(const char *name) { return post_cmd(STORAGE_CMD_VIDEO_RESOLVE_NAME, name, pdMS_TO_TICKS(50)); }
bool storage_prepare_for_sleep(uint32_t timeout_ms)
{
    if (s_cmd_queue == NULL || s_task_handle == NULL) {
        return true;
    }
    if (s_sleep_prep_sem == NULL) {
        return false;
    }

    while (xSemaphoreTake(s_sleep_prep_sem, 0) == pdTRUE) {
    }

    if (!post_cmd(STORAGE_CMD_PREPARE_SLEEP, NULL, pdMS_TO_TICKS(100))) {
        return false;
    }

    return (xSemaphoreTake(s_sleep_prep_sem, pdMS_TO_TICKS(timeout_ms)) == pdTRUE);
}

size_t storage_get_novel_list(storage_file_entry_t *out, size_t cap)
{
    size_t n = 0;
    if (out == NULL || cap == 0) return 0;
    lock_data();
    if (s_novel_cache != NULL) {
        n = (s_novel_cache_count < cap) ? s_novel_cache_count : cap;
    }
    if (n > 0) {
        memcpy(out, s_novel_cache, n * sizeof(storage_file_entry_t));
    }
    unlock_data();
    return n;
}

size_t storage_get_video_list(storage_file_entry_t *out, size_t cap)
{
    size_t n = 0;
    if (out == NULL || cap == 0) return 0;
    lock_data();
    if (s_video_cache != NULL) {
        n = (s_video_cache_count < cap) ? s_video_cache_count : cap;
    }
    if (n > 0) {
        memcpy(out, s_video_cache, n * sizeof(storage_file_entry_t));
    }
    unlock_data();
    return n;
}

bool storage_get_last_novel_page(char *text, size_t text_len, long *offset)
{
    if (text == NULL || text_len == 0) return false;
    lock_data();
    strncpy(text, s_last_page_text, text_len - 1);
    text[text_len - 1] = '\0';
    if (offset != NULL) {
        *offset = s_last_page_offset;
    }
    unlock_data();
    return true;
}

bool storage_get_last_video_open(char *path, size_t path_len, video_format_t *format)
{
    if (path == NULL || path_len == 0 || format == NULL) return false;
    lock_data();
    snprintf(path, path_len, "%s", s_last_video_path);
    *format = s_last_video_format;
    unlock_data();
    return (path[0] != '\0');
}

UBaseType_t storage_worker_stack_hwm(void)
{
    if (s_task_handle == NULL) return 0;
    return uxTaskGetStackHighWaterMark(s_task_handle);
}

UBaseType_t storage_worker_queue_waiting(void)
{
    if (s_cmd_queue == NULL) return 0;
    return uxQueueMessagesWaiting(s_cmd_queue);
}
