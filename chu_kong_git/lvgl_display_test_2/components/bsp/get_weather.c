/*
 * Weather service (Seniverse daily forecast).
 *
 * Behavior:
 * - Waits for sync requests (semaphore), no periodic polling by default.
 * - Keeps a cached snapshot for UI.
 * - Reports status/update to LVGL via lvgl_msg_send_nonblocking.
 */

#include "get_weather.h"

#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "lvgl_display.h"
#include "wifi_manager.h"

#define TAG "weather"

#define HOST "api.seniverse.com"
#define API_KEY "SxeezOBBh2Yz5B8CE"
#define LOCATION "jiaozuo"
#define LANGUAGE "zh-Hans"
#define TEMPERATURE_UNIT "c"

#define RESPONSE_BODY_MAX_SIZE 1024

static const char *s_weather_url =
    "http://" HOST "/v3/weather/daily.json?key=" API_KEY "&location=" LOCATION
    "&language=" LANGUAGE "&unit=" TEMPERATURE_UNIT "&start=0&days=3";

static TaskHandle_t s_weather_task_handle = NULL;
static SemaphoreHandle_t s_weather_trigger_sem = NULL;
static SemaphoreHandle_t s_weather_mutex = NULL;
static weather_snapshot_t s_snapshot = {
    .valid = false,
    .status = WEATHER_NO_WIFI,
};

static void weather_post_status(const char *text, uint32_t color_hex)
{
    (void)lvgl_msg_send_nonblocking(LVGL_MSG_WEATHER_STATUS, (int32_t)color_hex, text);
}

static void weather_post_updated(void)
{
    (void)lvgl_msg_send_nonblocking(LVGL_MSG_WEATHER_UPDATED, 0, NULL);
}

static void weather_snapshot_update_status(weather_status_t status)
{
    if (s_weather_mutex == NULL) return;
    if (xSemaphoreTake(s_weather_mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        s_snapshot.status = status;
        xSemaphoreGive(s_weather_mutex);
    }
}

static void weather_snapshot_write(const weather_snapshot_t *src)
{
    if (src == NULL || s_weather_mutex == NULL) return;
    if (xSemaphoreTake(s_weather_mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        s_snapshot = *src;
        xSemaphoreGive(s_weather_mutex);
    }
}

bool weather_get_snapshot(weather_snapshot_t *out)
{
    if (out == NULL) return false;
    memset(out, 0, sizeof(*out));
    if (s_weather_mutex == NULL) return false;
    if (xSemaphoreTake(s_weather_mutex, pdMS_TO_TICKS(200)) != pdTRUE) return false;
    *out = s_snapshot;
    xSemaphoreGive(s_weather_mutex);
    return true;
}

static void safe_copy_str(char *dst, size_t dst_size, const char *src)
{
    if (dst == NULL || dst_size == 0) return;
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

static bool parse_weather_json(const char *json, weather_snapshot_t *out)
{
    if (json == NULL || out == NULL) return false;

    cJSON *root = cJSON_Parse(json);
    if (root == NULL) return false;

    bool ok = false;
    cJSON *results = cJSON_GetObjectItem(root, "results");
    if (!cJSON_IsArray(results)) goto exit;

    cJSON *obj0 = cJSON_GetArrayItem(results, 0);
    if (!cJSON_IsObject(obj0)) goto exit;

    cJSON *location = cJSON_GetObjectItem(obj0, "location");
    cJSON *daily = cJSON_GetObjectItem(obj0, "daily");
    if (!cJSON_IsObject(location) || !cJSON_IsArray(daily)) goto exit;

    cJSON *loc_name = cJSON_GetObjectItem(location, "name");
    if (!cJSON_IsString(loc_name) || loc_name->valuestring == NULL) goto exit;

    cJSON *day0 = cJSON_GetArrayItem(daily, 0);
    if (!cJSON_IsObject(day0)) goto exit;

    cJSON *text_day = cJSON_GetObjectItem(day0, "text_day");
    cJSON *high = cJSON_GetObjectItem(day0, "high");
    cJSON *low = cJSON_GetObjectItem(day0, "low");
    cJSON *humidity = cJSON_GetObjectItem(day0, "humidity");

    if (!cJSON_IsString(text_day) || text_day->valuestring == NULL) goto exit;
    if (!cJSON_IsString(high) || high->valuestring == NULL) goto exit;
    if (!cJSON_IsString(low) || low->valuestring == NULL) goto exit;
    if (!cJSON_IsString(humidity) || humidity->valuestring == NULL) goto exit;

    memset(out, 0, sizeof(*out));
    out->valid = true;
    out->status = WEATHER_OK;
    safe_copy_str(out->city, sizeof(out->city), loc_name->valuestring);
    safe_copy_str(out->condition, sizeof(out->condition), text_day->valuestring);
    out->high = (int8_t)atoi(high->valuestring);
    out->low = (int8_t)atoi(low->valuestring);
    out->humidity = (uint8_t)atoi(humidity->valuestring);
    time(&out->last_update);

    ok = true;

exit:
    cJSON_Delete(root);
    return ok;
}

static esp_err_t weather_http_get(char *out_buf, size_t out_cap)
{
    if (out_buf == NULL || out_cap == 0) return ESP_ERR_INVALID_ARG;
    out_buf[0] = '\0';

    esp_http_client_config_t cfg = {
        .url = s_weather_url,
        .timeout_ms = 8000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) return ESP_FAIL;

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        return err;
    }

    int content_length = esp_http_client_fetch_headers(client);
    if (content_length < 0 || (size_t)content_length >= out_cap) {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    int read_len = esp_http_client_read_response(client, out_buf, (int)out_cap - 1);
    if (read_len < 0) {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }
    out_buf[read_len] = '\0';

    int status = esp_http_client_get_status_code(client);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (status != 200) {
        ESP_LOGW(TAG, "HTTP status=%d", status);
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void weather_task(void *param)
{
    (void)param;

    char response_body[RESPONSE_BODY_MAX_SIZE];
    memset(response_body, 0, sizeof(response_body));

    while (1) {
        if (s_weather_trigger_sem == NULL) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        if (xSemaphoreTake(s_weather_trigger_sem, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        weather_snapshot_update_status(WEATHER_SYNCING);
        weather_post_status("Syncing...", 0xF0B429);
        weather_post_updated();

        if (!wifi_manager_is_enabled()) {
            weather_snapshot_update_status(WEATHER_NO_WIFI);
            weather_post_status("No WiFi", 0x9A9A9A);
            weather_post_updated();
            continue;
        }

        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
            weather_snapshot_update_status(WEATHER_NO_WIFI);
            weather_post_status("No WiFi", 0x9A9A9A);
            weather_post_updated();
            continue;
        }

        esp_err_t err = weather_http_get(response_body, sizeof(response_body));
        if (err != ESP_OK) {
            weather_snapshot_update_status(WEATHER_HTTP_FAIL);
            weather_post_status("HTTP fail", 0xFF3333);
            weather_post_updated();
            continue;
        }

        weather_snapshot_t next;
        if (!parse_weather_json(response_body, &next)) {
            weather_snapshot_update_status(WEATHER_PARSE_FAIL);
            weather_post_status("Parse fail", 0xFF3333);
            weather_post_updated();
            continue;
        }

        weather_snapshot_write(&next);

        struct tm t;
        localtime_r(&next.last_update, &t);
        char status[32];
        snprintf(status, sizeof(status), "OK %02d:%02d", t.tm_hour, t.tm_min);
        weather_post_status(status, 0x33CC66);
        weather_post_updated();
    }
}

void weather_service_init(void)
{
    if (s_weather_task_handle != NULL) return;

    if (s_weather_mutex == NULL) {
        s_weather_mutex = xSemaphoreCreateMutex();
    }
    if (s_weather_trigger_sem == NULL) {
        s_weather_trigger_sem = xSemaphoreCreateBinary();
    }

    if (s_weather_mutex == NULL || s_weather_trigger_sem == NULL) {
        ESP_LOGE(TAG, "weather init failed (mutex/sem)");
        return;
    }

    if (xTaskCreate(weather_task, "weather_task", 4096, NULL, 4, &s_weather_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "create weather_task failed");
        s_weather_task_handle = NULL;
    }
}

void weather_request_sync(void)
{
    if (s_weather_trigger_sem != NULL) {
        xSemaphoreGive(s_weather_trigger_sem);
    }
}

