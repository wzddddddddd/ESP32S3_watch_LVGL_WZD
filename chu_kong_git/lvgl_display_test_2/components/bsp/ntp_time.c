#include "ntp_time.h"

#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "time.h"
#include "lvgl.h"
#include "esp_wifi.h"
#include "gui_guider.h"
#include "wifi_manager.h"
#include "lvgl_display.h"
#include "rtc_time_service.h"

static const char *TAG = "ntp";

extern lv_ui guider_ui;

TimerHandle_t sntp_sync_timer = NULL;
SemaphoreHandle_t sntp_trigger_sem = NULL;
static SemaphoreHandle_t sntp_sync_done_sem = NULL;

time_t now;
struct tm timeinfo;

static void ntp_quick_status(const char *text, uint32_t color_hex)
{
    (void)lvgl_msg_send_nonblocking(LVGL_MSG_NTP_SYNC_STATUS, (int32_t)color_hex, text);
}

void ntp_refresh_time_cache(void)
{
    time(&now);
    localtime_r(&now, &timeinfo);
}

static bool ntp_wait_sync_done(uint32_t timeout_ms)
{
    TickType_t start = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

    while ((xTaskGetTickCount() - start) < timeout_ticks) {
        sntp_sync_status_t status = esp_sntp_get_sync_status();
        if (status == SNTP_SYNC_STATUS_COMPLETED ||
            status == SNTP_SYNC_STATUS_IN_PROGRESS) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return false;
}

static void ntp_time_sync_notify_cb(struct timeval *tv)
{
    (void)tv;
    if (sntp_sync_done_sem != NULL) {
        xSemaphoreGive(sntp_sync_done_sem);
    }
}

static void esp_initialize_sntp(void)
{
    setenv("TZ", "CST-8", 1);
    tzset();

    esp_sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    esp_sntp_set_time_sync_notification_cb(ntp_time_sync_notify_cb);
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "203.107.6.88");
    esp_sntp_setservername(1, "pool.ntp.org");
    esp_sntp_setservername(2, "cn.ntp.org.cn");

    esp_sntp_init();
    esp_sntp_set_sync_interval(60 * 60 * 1000);
}

void ntp_time_init(void)
{
    esp_initialize_sntp();
    ntp_refresh_time_cache();
}

void sntp_time_task(void *param)
{
    (void)param;

    ntp_time_init();
    ESP_LOGI(TAG, "SNTP init done");

    while (1) {
        ntp_refresh_time_cache();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void sntp_interval_task(void *param)
{
    (void)param;

    sntp_trigger_sem = xSemaphoreCreateBinary();
    if (sntp_trigger_sem == NULL) {
        ESP_LOGE(TAG, "create sntp trigger failed");
        vTaskDelete(NULL);
        return;
    }
    sntp_sync_done_sem = xSemaphoreCreateBinary();
    if (sntp_sync_done_sem == NULL) {
        ESP_LOGE(TAG, "create sntp done sem failed");
        vTaskDelete(NULL);
        return;
    }

    xSemaphoreGive(sntp_trigger_sem);
    ESP_LOGI(TAG, "trigger first NTP sync");

    while (1) {
        if (xSemaphoreTake(sntp_trigger_sem, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG, "received sync trigger, syncing NTP...");
            ntp_quick_status("Syncing...", 0xF0B429);

            wifi_ap_record_t ap_info;
            esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
            if (ret == ESP_OK) {
                if (!esp_sntp_enabled()) {
                    ESP_LOGW(TAG, "SNTP not enabled, skip this sync");
                    ntp_quick_status("SNTP off", 0xFF3333);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    xSemaphoreGive(sntp_trigger_sem);
                    continue;
                }

                while (xSemaphoreTake(sntp_sync_done_sem, 0) == pdPASS) {
                }
                esp_sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);
                if (!esp_sntp_restart()) {
                    ESP_LOGW(TAG, "SNTP restart failed");
                    ntp_quick_status("Restart fail", 0xFF3333);
                    continue;
                }

                bool cb_ok = (xSemaphoreTake(sntp_sync_done_sem, pdMS_TO_TICKS(12000)) == pdPASS);
                bool status_ok = ntp_wait_sync_done(3000);
                if (!cb_ok && !status_ok) {
                    ESP_LOGW(TAG, "NTP sync timeout");
                    ntp_quick_status("Timeout", 0xFF3333);
                    continue;
                }

                ntp_refresh_time_cache();

                esp_err_t rtc_ret = rtc_time_service_sync_rtc_from_system();
                if (rtc_ret != ESP_OK) {
                    ESP_LOGW(TAG, "sync rtc from ntp failed: %s", esp_err_to_name(rtc_ret));
                }

                ESP_LOGI(TAG, "NTP sync success");
                char status[32];
                snprintf(status, sizeof(status), "OK %02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
                ntp_quick_status(status, 0x33CC66);
            } else {
                ESP_LOGW(TAG, "WiFi not connected, skip this sync");
                ntp_quick_status("No WiFi", 0x9A9A9A);
            }
        }
    }
}

void trigger_ntp_sync(void)
{
    if (sntp_trigger_sem != NULL) {
        xSemaphoreGive(sntp_trigger_sem);
        ESP_LOGI(TAG, "wifi connected, trigger NTP sync");
        ntp_refresh_time_cache();
    }
}

struct tm* get_time(void)
{
    return &timeinfo;
}
