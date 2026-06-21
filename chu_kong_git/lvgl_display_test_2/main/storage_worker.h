#ifndef STORAGE_WORKER_H_
#define STORAGE_WORKER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "video_player.h"

#define STORAGE_WORKER_NAME_MAX 256
#define STORAGE_WORKER_PATH_MAX 512

typedef struct {
    char name[STORAGE_WORKER_NAME_MAX];
    char full_path[STORAGE_WORKER_PATH_MAX];
} storage_file_entry_t;

void storage_worker_init(void);

bool storage_request_novel_list_refresh(void);
bool storage_request_video_list_refresh(void);
bool storage_request_novel_open_by_name(const char *name);
bool storage_request_novel_open_by_path(const char *path);
bool storage_request_novel_close(void);
bool storage_request_novel_page_sync(void);
bool storage_request_novel_page_next(void);
bool storage_request_novel_page_prev(void);
bool storage_request_video_resolve_by_name(const char *name);
bool storage_prepare_for_sleep(uint32_t timeout_ms);

size_t storage_get_novel_list(storage_file_entry_t *out, size_t cap);
size_t storage_get_video_list(storage_file_entry_t *out, size_t cap);
bool storage_get_last_novel_page(char *text, size_t text_len, long *offset);
bool storage_get_last_video_open(char *path, size_t path_len, video_format_t *format);
UBaseType_t storage_worker_stack_hwm(void);
UBaseType_t storage_worker_queue_waiting(void);

#endif
