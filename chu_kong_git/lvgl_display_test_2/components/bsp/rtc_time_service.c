#include "rtc_time_service.h"

#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "sd3078.h"

#define RTC_SERVICE_TAG "rtc_service"

#define RTC_SERVICE_I2C_PORT         I2C_NUM_0
#define RTC_SYNC_INTERVAL_MS         (60UL * 60UL * 1000UL)

static bool s_ready = false;
static SemaphoreHandle_t s_service_mutex = NULL;
static TimerHandle_t s_hourly_timer = NULL;

/* shared with ntp_time.c */
extern time_t now;
extern struct tm timeinfo;

static inline void refresh_time_cache_from_system(void)
{
    time(&now);
    localtime_r(&now, &timeinfo);
}

static bool service_lock_take(TickType_t timeout_ticks)
{
    if (s_service_mutex == NULL) {
        s_service_mutex = xSemaphoreCreateMutex();
        if (s_service_mutex == NULL) {
            return false;
        }
    }
    return (xSemaphoreTake(s_service_mutex, timeout_ticks) == pdTRUE);
}

static inline void service_lock_give(void)
{
    if (s_service_mutex != NULL) {
        xSemaphoreGive(s_service_mutex);
    }
}

static bool rtc_time_sanity_check(const sd3078_time_t *time_value)
{
    return sd3078_time_is_valid(time_value);
}

static bool manual_time_roundtrip_valid(int year, int month, int day,
                                        int hour, int minute, int second,
                                        time_t ts)
{
    struct tm verify_tm = {0};
    if (localtime_r(&ts, &verify_tm) == NULL) {
        return false;
    }

    return ((verify_tm.tm_year + 1900) == year) &&
           ((verify_tm.tm_mon + 1) == month) &&
           (verify_tm.tm_mday == day) &&
           (verify_tm.tm_hour == hour) &&
           (verify_tm.tm_min == minute) &&
           (verify_tm.tm_sec == second);
}

static esp_err_t set_system_time_from_unix(time_t ts)
{
    struct timeval tv = {
        .tv_sec = ts,
        .tv_usec = 0
    };
    if (settimeofday(&tv, NULL) != 0) {
        return ESP_FAIL;
    }
    refresh_time_cache_from_system();
    return ESP_OK;
}

static void rtc_hourly_timer_cb(TimerHandle_t xTimer)
{
    (void)xTimer;
    esp_err_t ret = rtc_time_service_hourly_sync_system_from_rtc();
    if (ret != ESP_OK) {
        ESP_LOGW(RTC_SERVICE_TAG, "hourly sync failed: %s", esp_err_to_name(ret));
    }
}

bool rtc_time_service_is_ready(void)
{
    return s_ready;
}

esp_err_t rtc_time_service_init_shared_i2c(void)
{
    if (s_ready) {
        return ESP_OK;
    }

    if (!service_lock_take(pdMS_TO_TICKS(200))) {
        return ESP_ERR_TIMEOUT;
    }

    setenv("TZ", "CST-8", 1);
    tzset();

    esp_err_t ret = sd3078_init(RTC_SERVICE_I2C_PORT);
    if (ret == ESP_OK) {
        if (s_hourly_timer == NULL) {
            s_hourly_timer = xTimerCreate("rtc_hourly_sync",
                                          pdMS_TO_TICKS(RTC_SYNC_INTERVAL_MS),
                                          pdTRUE,
                                          NULL,
                                          rtc_hourly_timer_cb);
            if (s_hourly_timer == NULL) {
                ret = ESP_ERR_NO_MEM;
            } else if (xTimerStart(s_hourly_timer, pdMS_TO_TICKS(200)) != pdPASS) {
                ret = ESP_FAIL;
            }
        }
        if (ret == ESP_OK) {
            s_ready = true;
        }
    }

    service_lock_give();

    if (ret != ESP_OK) {
        ESP_LOGW(RTC_SERVICE_TAG, "init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(RTC_SERVICE_TAG, "RTC service init ok");
    return ESP_OK;
}

esp_err_t rtc_time_service_boot_sync_system_from_rtc(void)
{
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!service_lock_take(pdMS_TO_TICKS(200))) {
        return ESP_ERR_TIMEOUT;
    }

    sd3078_time_t rtc_time = {0};
    esp_err_t ret = sd3078_get_time(&rtc_time);
    if (ret != ESP_OK) {
        service_lock_give();
        return ret;
    }

    if (!rtc_time_sanity_check(&rtc_time)) {
        sd3078_time_t default_time = {
            .year = 2026,
            .month = 1,
            .day = 1,
            .week = 4,   /* 2026-01-01 is Thursday */
            .hour = 0,
            .minute = 0,
            .second = 0,
        };

        ret = sd3078_set_time(&default_time);
        if (ret != ESP_OK) {
            service_lock_give();
            return ret;
        }

        struct tm tm_default = {0};
        tm_default.tm_year = default_time.year - 1900;
        tm_default.tm_mon = default_time.month - 1;
        tm_default.tm_mday = default_time.day;
        tm_default.tm_hour = default_time.hour;
        tm_default.tm_min = default_time.minute;
        tm_default.tm_sec = default_time.second;
        tm_default.tm_isdst = -1;

        time_t ts_default = mktime(&tm_default);
        ret = (ts_default == (time_t)-1) ? ESP_FAIL : set_system_time_from_unix(ts_default);
        service_lock_give();
        if (ret == ESP_OK) {
            ESP_LOGW(RTC_SERVICE_TAG, "RTC invalid, fallback to default 2026-01-01 00:00:00");
        }
        return ret;
    }

    time_t ts = 0;
    ret = sd3078_get_unix_time(&ts);
    if (ret == ESP_OK) {
        ret = set_system_time_from_unix(ts);
    }
    service_lock_give();
    return ret;
}

esp_err_t rtc_time_service_sync_rtc_from_system(void)
{
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!service_lock_take(pdMS_TO_TICKS(200))) {
        return ESP_ERR_TIMEOUT;
    }

    time_t current = 0;
    time(&current);
    esp_err_t ret = sd3078_set_unix_time(current);
    if (ret == ESP_OK) {
        refresh_time_cache_from_system();
    }

    service_lock_give();
    return ret;
}

esp_err_t rtc_time_service_prepare_for_sleep(void)
{
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!service_lock_take(pdMS_TO_TICKS(20))) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t ret = sd3078_prepare_for_sleep();
    service_lock_give();
    return ret;
}

esp_err_t rtc_time_service_set_manual_and_sync(int year, int month, int day,
                                               int hour, int minute, int second)
{
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!service_lock_take(pdMS_TO_TICKS(200))) {
        return ESP_ERR_TIMEOUT;
    }

    sd3078_time_t input_time = {
        .year = (uint16_t)year,
        .month = (uint8_t)month,
        .day = (uint8_t)day,
        .week = 0,
        .hour = (uint8_t)hour,
        .minute = (uint8_t)minute,
        .second = (uint8_t)second,
    };
    if (!sd3078_time_is_valid(&input_time)) {
        service_lock_give();
        return ESP_ERR_INVALID_ARG;
    }

    struct tm tm_set = {0};
    tm_set.tm_year = year - 1900;
    tm_set.tm_mon = month - 1;
    tm_set.tm_mday = day;
    tm_set.tm_hour = hour;
    tm_set.tm_min = minute;
    tm_set.tm_sec = second;
    tm_set.tm_isdst = -1;

    time_t ts = mktime(&tm_set);
    if (ts == (time_t)-1 ||
        !manual_time_roundtrip_valid(year, month, day, hour, minute, second, ts)) {
        service_lock_give();
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = set_system_time_from_unix(ts);
    if (ret == ESP_OK) {
        ret = sd3078_set_unix_time(ts);
    }
    if (ret == ESP_OK) {
        refresh_time_cache_from_system();
    }

    service_lock_give();
    return ret;
}

esp_err_t rtc_time_service_hourly_sync_system_from_rtc(void)
{
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!service_lock_take(pdMS_TO_TICKS(200))) {
        return ESP_ERR_TIMEOUT;
    }

    time_t ts = 0;
    esp_err_t ret = sd3078_get_unix_time(&ts);
    if (ret == ESP_OK) {
        ret = set_system_time_from_unix(ts);
    }

    service_lock_give();
    return ret;
}
