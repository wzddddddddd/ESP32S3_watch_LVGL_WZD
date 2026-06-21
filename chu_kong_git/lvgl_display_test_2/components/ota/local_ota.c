#include "local_ota.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_app_format.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include "lvgl_display.h"

static const char *TAG = "local_ota";

// OTA取消标志：true=用户取消下载
static volatile bool s_ota_cancelled = false;
static volatile bool s_rebooting = false;
static TaskHandle_t s_reboot_task_handle = NULL;

void local_ota_cancel(void)
{
    s_ota_cancelled = true;
}

static void local_ota_reboot_task(void *param)
{
    (void)param;

    /* Keep reboot path minimal and non-blocking to avoid deadlocks in UI/flush paths. */
    ESP_LOGI(TAG, "ota reboot: restarting...");
    vTaskDelay(pdMS_TO_TICKS(50));
    esp_restart();

    vTaskDelete(NULL);
}

static void local_ota_request_reboot(void)
{
    if (s_rebooting) {
        ESP_LOGW(TAG, "ota reboot already running");
        return;
    }

    s_rebooting = true;
    if (xTaskCreate(local_ota_reboot_task,
                    "ota_reboot",
                    4096,
                    NULL,
                    8,
                    &s_reboot_task_handle) != pdPASS) {
        s_reboot_task_handle = NULL;
        s_rebooting = false;
        ESP_LOGE(TAG, "create ota reboot task failed");
        esp_restart();
    }
}

// 本地OTA后台任务
void local_ota_task(void *pvParameter)
{
    // 重置取消标志
    s_ota_cancelled = false;

    char *file_path = (char *)pvParameter;
    ESP_LOGI(TAG, "本地OTA后台任务启动, 文件: %s", file_path);

    // ===== 通知UI：开始下载 =====
    lvgl_msg_send(LVGL_MSG_OTA_STATUS, 0, "Downloading...");

    // 打开文件
    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", file_path);
        lvgl_msg_send(LVGL_MSG_OTA_STATUS, 0, "Failed: open file");
        lvgl_msg_send(LVGL_MSG_OTA_COMPLETE, LVGL_OTA_RESULT_FAILED, NULL);  // ★★★ 解锁UI ★★★
        vTaskDelete(NULL);
        return;
    }

    // 获取当前运行分区
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running == NULL) {
        ESP_LOGE(TAG, "Failed to get running partition");
        fclose(fp);
        lvgl_msg_send(LVGL_MSG_OTA_STATUS, 0, "Failed: running partition");
        lvgl_msg_send(LVGL_MSG_OTA_COMPLETE, LVGL_OTA_RESULT_FAILED, NULL);  // ★★★ 解锁UI ★★★
        vTaskDelete(NULL);
        return;
    }

    // 获取OTA运行时分区
    const esp_partition_t *update = esp_ota_get_next_update_partition(NULL);
    if (update == NULL) {
        ESP_LOGE(TAG, "Failed to get update partition");
        fclose(fp);
        lvgl_msg_send(LVGL_MSG_OTA_STATUS, 0, "Failed: update partition");
        lvgl_msg_send(LVGL_MSG_OTA_COMPLETE, LVGL_OTA_RESULT_FAILED, NULL);  // ★★★ 解锁UI ★★★
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Running: %s @ 0x%" PRIx32, running->label, running->address);
    ESP_LOGI(TAG, "Update: %s @ 0x%" PRIx32 " size: %" PRIu32, update->label, update->address, update->size);

    // 擦除更新分区
    ESP_LOGI(TAG, "Erasing update partition...");
    lvgl_msg_send(LVGL_MSG_OTA_STATUS, 0, "Erasing...");
    if (esp_partition_erase_range(update, 0, update->size) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase update region");
        fclose(fp);
        lvgl_msg_send(LVGL_MSG_OTA_STATUS, 0, "Failed: erase");
        lvgl_msg_send(LVGL_MSG_OTA_COMPLETE, LVGL_OTA_RESULT_FAILED, NULL);  // ★★★ 解锁UI ★★★
        vTaskDelete(NULL);
        return;
    }

    // 开始OTA写入
    esp_ota_handle_t update_handle = 0;
    esp_err_t err = esp_ota_begin(update, update->size, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        fclose(fp);
        lvgl_msg_send(LVGL_MSG_OTA_STATUS, 0, "Failed: ota begin");
        lvgl_msg_send(LVGL_MSG_OTA_COMPLETE, LVGL_OTA_RESULT_FAILED, NULL);  // ★★★ 解锁UI ★★★
        vTaskDelete(NULL);
        return;
    }

    // ===== 提速：使用大缓冲区（内部RAM） =====
    #define OTA_BUFF_SIZE 8192
    char *ota_buf = (char *)heap_caps_malloc(OTA_BUFF_SIZE, MALLOC_CAP_INTERNAL);
    if (ota_buf == NULL) {
        ESP_LOGW(TAG, "内部RAM分配失败，降级到普通malloc");
        ota_buf = malloc(OTA_BUFF_SIZE);
    }

    if (ota_buf == NULL) {
        ESP_LOGE(TAG, "Buffer分配失败");
        esp_ota_abort(update_handle);
        fclose(fp);
        lvgl_msg_send(LVGL_MSG_OTA_STATUS, 0, "Failed: no memory");
        lvgl_msg_send(LVGL_MSG_OTA_COMPLETE, LVGL_OTA_RESULT_FAILED, NULL);  // ★★★ 解锁UI ★★★
        vTaskDelete(NULL);
        return;
    }

    // 获取文件大小用于进度计算
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    size_t total_read = 0;
    int last_progress = -1;

    // ===== 累积写入，减少esp_ota_write调用次数 =====
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10)); // ★ 必须给CPU喘口气，否则看门狗会复位
        // 检查用户是否取消了下载
        if (s_ota_cancelled) {
            ESP_LOGI(TAG, "用户取消下载，停止OTA");
            free(ota_buf);
            esp_ota_abort(update_handle);
            fclose(fp);
            lvgl_msg_send(LVGL_MSG_OTA_STATUS, 0, "Cancelled");
            lvgl_msg_send(LVGL_MSG_OTA_COMPLETE, LVGL_OTA_RESULT_CANCELLED, NULL);  // ★★★ 解锁UI ★★★
            vTaskDelete(NULL);
            return;
        }

        size_t bytes_read = fread(ota_buf, 1, OTA_BUFF_SIZE, fp);
        if (bytes_read == 0) {
            if (feof(fp)) {
                ESP_LOGI(TAG, "Read file complete, total bytes: %d", total_read);
                break;
            }
            ESP_LOGE(TAG, "File read error");
            free(ota_buf);
            esp_ota_abort(update_handle);
            fclose(fp);
            lvgl_msg_send(LVGL_MSG_OTA_STATUS, 0, "Failed: read file");
            lvgl_msg_send(LVGL_MSG_OTA_COMPLETE, LVGL_OTA_RESULT_FAILED, NULL);  // ★★★ 解锁UI ★★★
            vTaskDelete(NULL);
            return;
        }

        err = esp_ota_write(update_handle, ota_buf, bytes_read);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
            free(ota_buf);
            esp_ota_abort(update_handle);
            fclose(fp);
            lvgl_msg_send(LVGL_MSG_OTA_STATUS, 0, "Failed: write flash");
            lvgl_msg_send(LVGL_MSG_OTA_COMPLETE, LVGL_OTA_RESULT_FAILED, NULL);  // ★★★ 解锁UI ★★★
            vTaskDelete(NULL);
            return;
        }

        total_read += bytes_read;

        // 计算并发送进度（每5%更新一次，避免消息过载）
        if (file_size > 0) {
            int progress = (int)((total_read * 100) / file_size);
            if (progress != last_progress && progress % 5 == 0) {
                last_progress = progress;
                lvgl_msg_send(LVGL_MSG_OTA_PROGRESS, progress, NULL);
                char progress_text[32];
                snprintf(progress_text, sizeof(progress_text), "Downloading %d%%", progress);
                lvgl_msg_send(LVGL_MSG_OTA_STATUS, 0, progress_text);
            }
        }
    }

    free(ota_buf);
    fclose(fp);

    // 完成OTA
    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        lvgl_msg_send(LVGL_MSG_OTA_STATUS, 0, "Failed: verify");
        lvgl_msg_send(LVGL_MSG_OTA_COMPLETE, LVGL_OTA_RESULT_FAILED, NULL);  // ★★★ 解锁UI ★★★
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "OTA download complete!");

    // ===== 通知UI：下载成功（不自动设置启动分区，由用户按"跳转"决定）=====
    lvgl_msg_send(LVGL_MSG_OTA_STATUS, 0, "Done. Please reboot.");
    lvgl_msg_send(LVGL_MSG_OTA_COMPLETE, LVGL_OTA_RESULT_SUCCESS, NULL);  // ★★★ 解锁UI ★★★

    vTaskDelete(NULL);
}

// 启动本地OTA（创建后台任务，不阻塞）
esp_err_t local_ota_start(const char *file_path)
{
    static char s_file_path[512] = {0};
    strncpy(s_file_path, file_path, sizeof(s_file_path) - 1);
    s_file_path[sizeof(s_file_path) - 1] = '\0';

    ESP_LOGI(TAG, "创建本地OTA后台任务: %s", s_file_path);

    BaseType_t ret = xTaskCreate(
        local_ota_task,          // 任务函数
        "local_ota_task",        // 任务名称
        8192,                    // 栈大小
        (void *)s_file_path,     // 参数
        5,                       // 优先级
        NULL                     // 句柄
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "创建OTA任务失败");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t local_ota_switch_partition(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *update = esp_ota_get_next_update_partition(NULL);

    if (running == NULL || update == NULL) {
        ESP_LOGE(TAG, "Failed to get partition info");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Switching from %s to %s", running->label, update->label);

    esp_err_t err = esp_ota_set_boot_partition(update);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "boot partition set, scheduling reboot");

    local_ota_request_reboot();
#if 0
    vTaskDelay(pdMS_TO_TICKS(500));  // ★ 延迟500ms，保证SD卡内部状态机复位
    esp_restart();

#endif

    return ESP_OK;
}

void local_ota_get_partition_info(char *running_label, char *standby_label, char *running_version)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *standby = esp_ota_get_next_update_partition(NULL);

    if (running) {
        if (running_label) {
            snprintf(running_label, 32, "%s", running->label);
        }
        if (running_version) {
            esp_app_desc_t app_desc;
            if (esp_ota_get_partition_description(running, &app_desc) == ESP_OK) {
                snprintf(running_version, 32, "%s", app_desc.version);
            } else {
                snprintf(running_version, 32, "%s", "unknown");
            }
        }
    }

    if (standby && standby_label) {
        snprintf(standby_label, 32, "%s", standby->label);
    }
}
