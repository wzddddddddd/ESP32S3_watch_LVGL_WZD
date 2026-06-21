/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "events_init.h"
#include "gui_guider.h"
#include "custom.h"
#include "arc_menu.h"

#include <stdio.h>
#include "lvgl.h"
#include "lvgl_display.h"
#include "SD_card.h"
#include "esp_log.h"
#include "esp_err.h"
#include "novel_progress.h"
#include "img_display.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "onenet_ota.h"
#include "local_ota.h"
#include "st7789_driver.h"
#include "rtc_time_service.h"
#include "get_weather.h"
#include "../game/2048/lv_100ask_2048.h"
#include "../game/memory_game/lv_100ask_memory_game.h"
#include "../game/snake/lv_100ask_snake.h"

#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "freemaster_client.h"
#endif

#define TAG     "EVENT_INIT"

#define MOUNT_POINT "/sdcard"   //?????.????????????????i???????????

// ????????????????????OTA?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
static volatile bool g_is_ota_running = false;

extern lv_ui guider_ui;

extern FILE *fp;
extern long g_file_offset;        // ??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
char full_path[512] = {0};          //???????????????????????????
char img_full_path[512] = {0};         //??????????????????g??????????????
extern char display_buf[1200];      //???????????????????????????????????????????????n??????????????????????????????????
extern SemaphoreHandle_t sntp_trigger_sem;

static lv_obj_t *s_screen_quick = NULL;
static lv_obj_t *s_quick_wifi_btn = NULL;
static lv_obj_t *s_quick_wifi_btn_label = NULL;
static lv_obj_t *s_quick_time_sync_btn = NULL;
static lv_obj_t *s_quick_time_sync_btn_label = NULL;
static lv_obj_t *s_quick_wifi_status_label = NULL;
static lv_obj_t *s_quick_time_sync_status_label = NULL;
static lv_obj_t *s_quick_weather_btn = NULL;
static lv_obj_t *s_quick_weather_label = NULL;
static lv_obj_t *s_quick_weather_status_label = NULL;
static lv_obj_t *s_quick_bri_slider = NULL;
static lv_obj_t *s_quick_bri_label = NULL;
static bool s_quick_wifi_enabled = false;
static bool s_quick_wifi_connected = false;
static bool s_quick_wifi_busy = false;
static uint32_t s_quick_wifi_busy_tick = 0;
static uint32_t s_quick_wifi_busy_timeout_ms = 2500;
static lv_timer_t *s_quick_wifi_watchdog_timer = NULL;
static QueueHandle_t s_quick_wifi_cmd_queue = NULL;
static TaskHandle_t s_quick_wifi_worker_task_handle = NULL;

typedef struct {
    bool enable;
} quick_wifi_cmd_t;

static void quick_update_brightness_label(int value)
{
    if (s_quick_bri_label != NULL && lv_obj_is_valid(s_quick_bri_label)) {
        lv_label_set_text_fmt(s_quick_bri_label, "%d%%", value);
    }
}

static void quick_refresh_wifi_ui(void)
{
    if (s_quick_wifi_btn != NULL && lv_obj_is_valid(s_quick_wifi_btn)) {
        uint32_t color = s_quick_wifi_enabled ? 0x2F80ED : 0x8E8E8E;
        uint32_t pressed_color = s_quick_wifi_enabled ? 0x2469C8 : 0x7A7A7A;
        lv_obj_set_style_bg_color(s_quick_wifi_btn, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(s_quick_wifi_btn, lv_color_hex(pressed_color), LV_PART_MAIN | LV_STATE_PRESSED);
        if (s_quick_wifi_busy) lv_obj_add_state(s_quick_wifi_btn, LV_STATE_DISABLED);
        else lv_obj_clear_state(s_quick_wifi_btn, LV_STATE_DISABLED);
    }
    if (s_quick_wifi_btn_label != NULL && lv_obj_is_valid(s_quick_wifi_btn_label)) {
        lv_label_set_text(s_quick_wifi_btn_label, "wifi");
        lv_obj_set_style_text_color(s_quick_wifi_btn_label, lv_color_hex(0xFFFFFF), 0);
    }
    if (s_quick_wifi_status_label != NULL && lv_obj_is_valid(s_quick_wifi_status_label)) {
        if (s_quick_wifi_busy) {
            lv_label_set_text(s_quick_wifi_status_label, "Switching...");
            lv_obj_set_style_text_color(s_quick_wifi_status_label, lv_color_hex(0xC8CDD6), 0);
        } else if (!s_quick_wifi_enabled) {
            lv_label_set_text(s_quick_wifi_status_label, "Off");
            lv_obj_set_style_text_color(s_quick_wifi_status_label, lv_color_hex(0x9A9A9A), 0);
        } else if (s_quick_wifi_connected) {
            lv_label_set_text(s_quick_wifi_status_label, "Connected");
            lv_obj_set_style_text_color(s_quick_wifi_status_label, lv_color_hex(0x33CC66), 0);
        } else {
            lv_label_set_text(s_quick_wifi_status_label, "Connecting...");
            lv_obj_set_style_text_color(s_quick_wifi_status_label, lv_color_hex(0xF0B429), 0);
        }
    }
}

static void quick_set_wifi_busy(bool busy, uint32_t timeout_ms)
{
    s_quick_wifi_busy = busy;
    if (busy) {
        s_quick_wifi_busy_tick = lv_tick_get();
        s_quick_wifi_busy_timeout_ms = (timeout_ms > 0U) ? timeout_ms : 2500U;
    }
    quick_refresh_wifi_ui();
}

static void quick_wifi_worker_task(void *param)
{
    LV_UNUSED(param);
    quick_wifi_cmd_t cmd = {0};
    while (1) {
        if (xQueueReceive(s_quick_wifi_cmd_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            if (cmd.enable) wifi_manager_start();
            else wifi_manager_stop();
        }
    }
}

static bool quick_wifi_worker_init_once(void)
{
    if (s_quick_wifi_cmd_queue == NULL) {
        s_quick_wifi_cmd_queue = xQueueCreate(4, sizeof(quick_wifi_cmd_t));
        if (s_quick_wifi_cmd_queue == NULL) {
            ESP_LOGE(TAG, "quick wifi queue create failed");
            return false;
        }
    }
    if (s_quick_wifi_worker_task_handle == NULL) {
        if (xTaskCreate(quick_wifi_worker_task, "quick_wifi_task", 3072, NULL, 4,
                        &s_quick_wifi_worker_task_handle) != pdPASS) {
            ESP_LOGE(TAG, "quick wifi worker create failed");
            s_quick_wifi_worker_task_handle = NULL;
            return false;
        }
    }
    return true;
}

static bool quick_wifi_post_set(bool enable)
{
    quick_wifi_cmd_t cmd = {
        .enable = enable
    };
    if (!quick_wifi_worker_init_once()) return false;
    if (xQueueSend(s_quick_wifi_cmd_queue, &cmd, 0) != pdTRUE) {
        ESP_LOGW(TAG, "quick wifi queue full");
        return false;
    }
    return true;
}

static void quick_wifi_watchdog_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    if (!s_quick_wifi_busy) return;
    if (lv_tick_elaps(s_quick_wifi_busy_tick) >= s_quick_wifi_busy_timeout_ms) {
        quick_set_wifi_busy(false, 0);
    }
}

void wifi_quick_set_state(bool connected)
{
    s_quick_wifi_connected = connected;
    if (connected) {
        s_quick_wifi_enabled = true;
    }
    quick_set_wifi_busy(false, 0);
}

void quick_time_sync_set_status(const char *text, uint32_t color_hex)
{
    if (s_quick_time_sync_status_label == NULL || !lv_obj_is_valid(s_quick_time_sync_status_label)) {
        return;
    }
    lv_label_set_text(s_quick_time_sync_status_label, (text != NULL) ? text : "");
    lv_obj_set_style_text_color(s_quick_time_sync_status_label, lv_color_hex(color_hex), 0);
}

void weather_ui_set_status(const char *text, uint32_t color_hex)
{
    if (s_quick_weather_status_label != NULL && lv_obj_is_valid(s_quick_weather_status_label)) {
        lv_label_set_text(s_quick_weather_status_label, (text != NULL) ? text : "");
        lv_obj_set_style_text_color(s_quick_weather_status_label, lv_color_hex(color_hex), 0);
    }

    if (guider_ui.screen_weather_label_status != NULL && lv_obj_is_valid(guider_ui.screen_weather_label_status)) {
        lv_label_set_text(guider_ui.screen_weather_label_status, (text != NULL) ? text : "");
        lv_obj_set_style_text_color(guider_ui.screen_weather_label_status, lv_color_hex(color_hex), 0);
    }
}

static void weather_format_hhmm(char *buf, size_t cap, time_t t)
{
    if (buf == NULL || cap == 0) return;
    struct tm tm_info;
    localtime_r(&t, &tm_info);
    snprintf(buf, cap, "%02d:%02d", tm_info.tm_hour, tm_info.tm_min);
}

void weather_ui_refresh_from_snapshot(void)
{
    weather_snapshot_t snap;
    bool ok = weather_get_snapshot(&snap);

    const char *condition = (ok && snap.valid && snap.condition[0] != '\0') ? snap.condition : "--";
    const char *city = (ok && snap.valid && snap.city[0] != '\0') ? snap.city : "--";

    if (s_quick_weather_label != NULL && lv_obj_is_valid(s_quick_weather_label)) {
        lv_label_set_text(s_quick_weather_label, condition);
    }

    if (guider_ui.screen_weather_label_city != NULL && lv_obj_is_valid(guider_ui.screen_weather_label_city)) {
        lv_label_set_text_fmt(guider_ui.screen_weather_label_city, "City: %s", city);
    }
    if (guider_ui.screen_weather_label_condition != NULL && lv_obj_is_valid(guider_ui.screen_weather_label_condition)) {
        lv_label_set_text_fmt(guider_ui.screen_weather_label_condition, "Weather: %s", condition);
    }

    if (guider_ui.screen_weather_label_temp != NULL && lv_obj_is_valid(guider_ui.screen_weather_label_temp)) {
        if (ok && snap.valid) {
            lv_label_set_text_fmt(guider_ui.screen_weather_label_temp, "Temp: %d~%d C", (int)snap.low, (int)snap.high);
        } else {
            lv_label_set_text(guider_ui.screen_weather_label_temp, "Temp: --");
        }
    }
    if (guider_ui.screen_weather_label_humidity != NULL && lv_obj_is_valid(guider_ui.screen_weather_label_humidity)) {
        if (ok && snap.valid) {
            lv_label_set_text_fmt(guider_ui.screen_weather_label_humidity, "Humidity: %u%%", (unsigned)snap.humidity);
        } else {
            lv_label_set_text(guider_ui.screen_weather_label_humidity, "Humidity: --");
        }
    }
    if (guider_ui.screen_weather_label_update != NULL && lv_obj_is_valid(guider_ui.screen_weather_label_update)) {
        if (ok && snap.valid && snap.last_update != 0) {
            char hhmm[16];
            weather_format_hhmm(hhmm, sizeof(hhmm), snap.last_update);
            lv_label_set_text_fmt(guider_ui.screen_weather_label_update, "Updated: %s", hhmm);
        } else {
            lv_label_set_text(guider_ui.screen_weather_label_update, "Updated: --");
        }
    }

    if (!ok) {
        weather_ui_set_status("No data", 0x9A9A9A);
        return;
    }

    switch (snap.status) {
        case WEATHER_SYNCING:
            weather_ui_set_status("Syncing...", 0xF0B429);
            break;
        case WEATHER_NO_WIFI:
            weather_ui_set_status("No WiFi", 0x9A9A9A);
            break;
        case WEATHER_HTTP_FAIL:
            weather_ui_set_status("HTTP fail", 0xFF3333);
            break;
        case WEATHER_PARSE_FAIL:
            weather_ui_set_status("Parse fail", 0xFF3333);
            break;
        case WEATHER_OK:
        default:
            if (snap.valid && snap.last_update != 0) {
                char hhmm[16];
                weather_format_hhmm(hhmm, sizeof(hhmm), snap.last_update);
                char status[32];
                snprintf(status, sizeof(status), "OK %s", hhmm);
                weather_ui_set_status(status, 0x33CC66);
            } else {
                weather_ui_set_status("OK", 0x33CC66);
            }
            break;
    }
}

static void quick_screen_load_clock(void)
{
    if (guider_ui.clock_screen == NULL || !lv_obj_is_valid(guider_ui.clock_screen)) {
        setup_scr_clock_screen(&guider_ui);
    }
    lv_scr_load_anim(guider_ui.clock_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

static void quick_screen_open_wifi_setting(void)
{
    if (guider_ui.screen_wifi_set == NULL || !lv_obj_is_valid(guider_ui.screen_wifi_set)) {
        setup_scr_screen_wifi_set(&guider_ui);
    }
    lv_scr_load_anim(guider_ui.screen_wifi_set, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

static void quick_screen_open_time_setting(void)
{
    if (guider_ui.screen_time_set == NULL || !lv_obj_is_valid(guider_ui.screen_time_set)) {
        setup_scr_screen_time_set(&guider_ui);
    }
    lv_scr_load_anim(guider_ui.screen_time_set, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

static void quick_screen_open_weather_screen(void)
{
    if (guider_ui.screen_weather == NULL || !lv_obj_is_valid(guider_ui.screen_weather)) {
        setup_scr_screen_weather(&guider_ui);
    }
    weather_ui_refresh_from_snapshot();
    lv_scr_load_anim(guider_ui.screen_weather, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

static void quick_screen_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_GESTURE) return;
    lv_indev_t *indev = lv_event_get_indev(e);
    if (indev == NULL) indev = lv_indev_get_act();
    if (indev != NULL && lv_indev_get_gesture_dir(indev) == LV_DIR_TOP) {
        quick_screen_load_clock();
    }
}

static void quick_brightness_slider_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    int val = lv_slider_get_value(lv_event_get_target(e));
    if (val < 10) val = 10;
    if (val > 100) val = 100;
    st7789_lcd_set_brightness((uint8_t)val);
    quick_update_brightness_label(val);
}

static void quick_wifi_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_SHORT_CLICKED) {
        if (s_quick_wifi_busy) return;

        bool prev_enabled = s_quick_wifi_enabled;
        bool prev_connected = s_quick_wifi_connected;
        bool next_enabled = !s_quick_wifi_enabled;

        s_quick_wifi_enabled = next_enabled;
        if (!next_enabled) {
            s_quick_wifi_connected = false;
        }
        quick_set_wifi_busy(true, 2500);

        if (!quick_wifi_post_set(next_enabled)) {
            s_quick_wifi_enabled = prev_enabled;
            s_quick_wifi_connected = prev_connected;
            quick_set_wifi_busy(false, 0);
        }
    } else if (code == LV_EVENT_LONG_PRESSED) {
        quick_screen_open_wifi_setting();
    }
}

static void quick_time_sync_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_SHORT_CLICKED) {
        quick_time_sync_set_status("Syncing...", 0xF0B429);
        if (sntp_trigger_sem != NULL) {
            xSemaphoreGive(sntp_trigger_sem);
            ESP_LOGI(TAG, "quick panel ntp sync trigger sent");
        } else {
            quick_time_sync_set_status("NTP unavailable", 0xFF3333);
            ESP_LOGW(TAG, "quick panel ntp trigger unavailable");
        }
    } else if (code == LV_EVENT_LONG_PRESSED) {
        quick_screen_open_time_setting();
    }
}

static void quick_weather_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_SHORT_CLICKED) {
        weather_ui_set_status("Syncing...", 0xF0B429);
        weather_request_sync();
    } else if (code == LV_EVENT_LONG_PRESSED) {
        quick_screen_open_weather_screen();
    }
}

static void open_clock_quick_screen(void)
{
    int cur_bri = (int)st7789_lcd_get_brightness();
    if (cur_bri < 10 || cur_bri > 100) cur_bri = 30;

    if (s_screen_quick == NULL || !lv_obj_is_valid(s_screen_quick)) {
        const int quick_btn_w = 96;
        const int quick_btn_h = 38;
        s_screen_quick = lv_obj_create(NULL);
        lv_obj_set_size(s_screen_quick, 240, 284);
        lv_obj_clear_flag(s_screen_quick, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(s_screen_quick, lv_color_hex(0x0B1220), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(s_screen_quick, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(s_screen_quick, quick_screen_event_handler, LV_EVENT_GESTURE, NULL);

        s_quick_wifi_btn = lv_btn_create(s_screen_quick);
        lv_obj_set_size(s_quick_wifi_btn, quick_btn_w, quick_btn_h);
        lv_obj_align(s_quick_wifi_btn, LV_ALIGN_TOP_MID, -60, 14);
        lv_obj_set_style_radius(s_quick_wifi_btn, 12, 0);
        lv_obj_set_style_border_width(s_quick_wifi_btn, 0, 0);
        lv_obj_add_event_cb(s_quick_wifi_btn, quick_wifi_btn_event_cb, LV_EVENT_ALL, NULL);

        s_quick_wifi_btn_label = lv_label_create(s_quick_wifi_btn);
        lv_obj_set_style_text_font(s_quick_wifi_btn_label, &songti_font_16, 0);
        lv_obj_center(s_quick_wifi_btn_label);

        s_quick_time_sync_btn = lv_btn_create(s_screen_quick);
        lv_obj_set_size(s_quick_time_sync_btn, quick_btn_w, quick_btn_h);
        lv_obj_align(s_quick_time_sync_btn, LV_ALIGN_TOP_MID, 60, 14);
        lv_obj_set_style_radius(s_quick_time_sync_btn, 12, 0);
        lv_obj_set_style_border_width(s_quick_time_sync_btn, 0, 0);
        lv_obj_set_style_bg_color(s_quick_time_sync_btn, lv_color_hex(0x4CAF50), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(s_quick_time_sync_btn, lv_color_hex(0x3D9142), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_add_event_cb(s_quick_time_sync_btn, quick_time_sync_btn_event_cb, LV_EVENT_ALL, NULL);

        s_quick_time_sync_btn_label = lv_label_create(s_quick_time_sync_btn);
        lv_label_set_text(s_quick_time_sync_btn_label, "sync");
        lv_obj_set_style_text_font(s_quick_time_sync_btn_label, &songti_font_16, 0);
        lv_obj_set_style_text_color(s_quick_time_sync_btn_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(s_quick_time_sync_btn_label);

        s_quick_wifi_status_label = lv_label_create(s_screen_quick);
        lv_obj_set_width(s_quick_wifi_status_label, quick_btn_w);
        lv_obj_align_to(s_quick_wifi_status_label, s_quick_wifi_btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
        lv_obj_set_style_text_font(s_quick_wifi_status_label, &songti_font_16, 0);
        lv_obj_set_style_text_align(s_quick_wifi_status_label, LV_TEXT_ALIGN_CENTER, 0);

        s_quick_time_sync_status_label = lv_label_create(s_screen_quick);
        lv_obj_set_width(s_quick_time_sync_status_label, quick_btn_w);
        lv_obj_align_to(s_quick_time_sync_status_label, s_quick_time_sync_btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
        lv_obj_set_style_text_font(s_quick_time_sync_status_label, &songti_font_16, 0);
        lv_obj_set_style_text_align(s_quick_time_sync_status_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(s_quick_time_sync_status_label, "");
        lv_obj_set_style_text_color(s_quick_time_sync_status_label, lv_color_hex(0xC8CDD6), 0);

        s_quick_bri_slider = lv_slider_create(s_screen_quick);
        lv_slider_set_range(s_quick_bri_slider, 10, 100);
        lv_obj_set_size(s_quick_bri_slider, 168, 12);
        lv_obj_align(s_quick_bri_slider, LV_ALIGN_TOP_MID, 0, 76);
        lv_obj_add_event_cb(s_quick_bri_slider, quick_brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

        s_quick_bri_label = lv_label_create(s_screen_quick);
        lv_obj_set_style_text_font(s_quick_bri_label, &songti_font_16, 0);
        lv_obj_align_to(s_quick_bri_label, s_quick_bri_slider, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

        s_quick_weather_btn = lv_btn_create(s_screen_quick);
        lv_obj_set_size(s_quick_weather_btn, 200, 44);
        lv_obj_align_to(s_quick_weather_btn, s_quick_bri_slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 18);
        lv_obj_set_style_radius(s_quick_weather_btn, 10, 0);
        lv_obj_set_style_border_width(s_quick_weather_btn, 0, 0);
        lv_obj_set_style_bg_color(s_quick_weather_btn, lv_color_hex(0x2D3748), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(s_quick_weather_btn, lv_color_hex(0x1F2937), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_add_event_cb(s_quick_weather_btn, quick_weather_btn_event_cb, LV_EVENT_ALL, NULL);

        s_quick_weather_label = lv_label_create(s_quick_weather_btn);
        lv_label_set_text(s_quick_weather_label, "--");
        lv_obj_set_style_text_font(s_quick_weather_label, &songti_font_16, 0);
        lv_obj_set_style_text_color(s_quick_weather_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(s_quick_weather_label);

        s_quick_weather_status_label = lv_label_create(s_screen_quick);
        lv_obj_set_width(s_quick_weather_status_label, 200);
        lv_obj_align_to(s_quick_weather_status_label, s_quick_weather_btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
        lv_obj_set_style_text_font(s_quick_weather_status_label, &songti_font_16, 0);
        lv_obj_set_style_text_align(s_quick_weather_status_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(s_quick_weather_status_label, "");
        lv_obj_set_style_text_color(s_quick_weather_status_label, lv_color_hex(0xC8CDD6), 0);
    }

    if (s_quick_wifi_watchdog_timer == NULL) {
        s_quick_wifi_watchdog_timer = lv_timer_create(quick_wifi_watchdog_cb, 200, NULL);
    }

    if (s_quick_bri_slider != NULL && lv_obj_is_valid(s_quick_bri_slider)) {
        lv_slider_set_value(s_quick_bri_slider, cur_bri, LV_ANIM_OFF);
    }
    (void)quick_wifi_worker_init_once();
    quick_update_brightness_label(cur_bri);
    quick_refresh_wifi_ui();
    weather_ui_refresh_from_snapshot();
    lv_scr_load_anim(s_screen_quick, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

static void clock_screen_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        switch(dir) {
        case LV_DIR_TOP:
        {
            if (arc_menu_is_open()) {
                break; /* menu open: do not switch page on upward gesture */
            }
            /* non-blocking: skip wait_release to avoid wakeup deadlock */
            ui_load_scr_animation(&guider_ui, &guider_ui.menu_screen, guider_ui.menu_screen_del, &guider_ui.clock_screen_del, setup_scr_menu_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false);
            break;
        }
        case LV_DIR_BOTTOM:
        {
            if (arc_menu_is_open()) {
                break; /* menu open: let it handle vertical drag */
            }
            open_clock_quick_screen();
            break;
        }
        case LV_DIR_RIGHT:
        {
            // ??????????????????????????????????????????????????????????i?????????.????????
            arc_menu_open();
            break;
        }
        case LV_DIR_LEFT:
        {
            if (arc_menu_is_open()) {
                arc_menu_close();
            }
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

static void clock_screen_cont_2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            arc_menu_open();
        } else if (dir == LV_DIR_LEFT) {
            if (arc_menu_is_open()) {
                arc_menu_close();
            }
        } else if (dir == LV_DIR_TOP) {
            if (arc_menu_is_open()) {
                break;
            }
            /* non-blocking: skip wait_release to avoid wakeup deadlock */
            ui_load_scr_animation(&guider_ui, &guider_ui.menu_screen, guider_ui.menu_screen_del, &guider_ui.clock_screen_del, setup_scr_menu_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false);
        } else if (dir == LV_DIR_BOTTOM) {
            if (arc_menu_is_open()) {
                break; /* menu open: let it handle vertical drag */
            }
            open_clock_quick_screen();
        }
        break;
    }
    default:
        break;
    }
}

// ========== ??????????????????????e??????????????????????????bel????$?????????????????????????==========
void clock_screen_color_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    if (arc_menu_is_open()) return;
    if (lv_event_get_target(e) != guider_ui.clock_screen_label_time) return;

    // ???????????????????????????????????????????????????????????????????????????????????????????????????.????????????????????????????????????????????????????????????????????????????????????????????
    static const uint32_t colors[] = {
        0xF5F0E1,  // ???????????????
        0xFF6B6B,  // ????????????????????
        0x4ECDC4,  // ???????????????
        0xFFE66D,  // ??????????????
        0x95E1D3,  // ?????????????e?????????
        0x000000,  // ???????????????
    };
    static uint8_t color_idx = 0;

    color_idx = (color_idx + 1) % (sizeof(colors) / sizeof(colors[0]));

    lv_obj_set_style_text_color(guider_ui.clock_screen_label_time, lv_color_hex(colors[color_idx]), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(guider_ui.clock_screen_label_date, lv_color_hex(colors[color_idx]), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void events_init_clock_screen (lv_ui *ui)
{
    lv_obj_clear_flag(ui->clock_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ui->clock_screen_cont_2, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_flag(ui->clock_screen_label_time, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui->clock_screen_label_date, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_flag(ui->clock_screen_cont_2, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(ui->clock_screen_label_time, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(ui->clock_screen_label_date, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_obj_add_event_cb(ui->clock_screen, clock_screen_event_handler, LV_EVENT_GESTURE, ui);
    lv_obj_add_event_cb(ui->clock_screen_cont_2, clock_screen_cont_2_event_handler, LV_EVENT_GESTURE, ui);
}

static void menu_screen_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        switch(dir) {
        case LV_DIR_BOTTOM:
        {
            // ????????????????????????????????????????????????????????????????
            /* non-blocking: skip wait_release to avoid wakeup deadlock */
            ui_load_scr_animation(&guider_ui, &guider_ui.clock_screen, guider_ui.clock_screen_del, &guider_ui.menu_screen_del, setup_scr_clock_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

void menu_screen_list_1_item0_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.novel_list, guider_ui.novel_list_del, &guider_ui.menu_screen_del, setup_scr_novel_list, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
        break;
    }
    default:
        break;
    }
}

void menu_screen_list_1_item2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        // ?????????????????????????????????????????????????????????????????????????
        if (lv_scr_act() != guider_ui.menu_screen) return;
    ui_load_scr_animation(&guider_ui, &guider_ui.setting_screen, guider_ui.setting_screen_del, &guider_ui.menu_screen_del, setup_scr_setting_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
        break;
    }
    default:
        break;
    }
}

// ???events_init.c ?????????????????????
void menu_screen_list_1_item1_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui,
            &guider_ui.screen_img_list, guider_ui.screen_img_list_del,
            &guider_ui.menu_screen_del, setup_scr_screen_img_list,
            LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
    }
}

// ???????????????????????????????n?????????????e????????????????????????????????????????????????????
void menu_screen_list_1_item3_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui,
            &guider_ui.video_list, guider_ui.video_list_del,
            &guider_ui.menu_screen_del, setup_scr_video_list,
            LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
    }
}


void menu_screen_list_1_item4_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui,
            &guider_ui.screen_game, guider_ui.screen_game_del,
            &guider_ui.menu_screen_del, setup_scr_screen_game,
            LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
    }
}

void events_init_menu_screen (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->menu_screen, menu_screen_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->menu_screen_list_1_item0, menu_screen_list_1_item0_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->menu_screen_list_1_item2, menu_screen_list_1_item2_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->menu_screen_list_1_item1, menu_screen_list_1_item1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->menu_screen_list_1_item3, menu_screen_list_1_item3_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->menu_screen_list_1_item4, menu_screen_list_1_item4_event_handler, LV_EVENT_ALL, ui);

    // ????????????????????????$???????????????????????????????????????????????????????n?????????????????????????????????????????????????????????????????
    create_swipeable_menu(ui);
}

static void screen_game_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            /* non-blocking: skip wait_release to avoid wakeup deadlock */
            ui_load_scr_animation(&guider_ui,
                &guider_ui.menu_screen, guider_ui.menu_screen_del,
                &guider_ui.screen_game_del, setup_scr_menu_screen,
                LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
        }
    }
}

static lv_obj_t *s_screen_2048 = NULL;
static lv_obj_t *s_obj_2048 = NULL;
static lv_obj_t *s_label_2048_score = NULL;

static void screen_2048_update_score_text(void)
{
    if (s_label_2048_score == NULL || s_obj_2048 == NULL) {
        return;
    }

    if (lv_100ask_2048_get_best_tile(s_obj_2048) >= 2048) {
        lv_label_set_text(s_label_2048_score, "YOU WIN!");
        return;
    }

    if (lv_100ask_2048_get_status(s_obj_2048)) {
        lv_label_set_text(s_label_2048_score, "GAME OVER");
        return;
    }

    lv_label_set_text_fmt(s_label_2048_score, "SCORE: %d", lv_100ask_2048_get_score(s_obj_2048));
}

static void screen_2048_value_changed_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    screen_2048_update_score_text();
}

static void screen_2048_new_game_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED || s_obj_2048 == NULL) {
        return;
    }
    lv_100ask_2048_set_new_game(s_obj_2048);
    screen_2048_update_score_text();
}

static void screen_2048_back_to_game_list(void)
{
    lv_indev_t *indev = lv_indev_get_act();
    if (indev != NULL) {
        /* non-blocking: skip wait_release to avoid wakeup deadlock */
    }

    if (guider_ui.screen_game != NULL) {
        lv_scr_load_anim(guider_ui.screen_game, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
    }
}

static void screen_2048_back_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    screen_2048_back_to_game_list();
}

static void open_game_2048_screen(void)
{
    s_screen_2048 = lv_obj_create(NULL);
    lv_obj_clear_flag(s_screen_2048, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_screen_2048, lv_color_hex(0xF6F0E8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_screen_2048, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *btn_back = lv_btn_create(s_screen_2048);
    lv_obj_set_size(btn_back, 62, 36);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 16, 8);
    lv_obj_add_event_cb(btn_back, screen_2048_back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "Back");
    lv_obj_center(label_back);

    lv_obj_t *btn_new_game = lv_btn_create(s_screen_2048);
    lv_obj_set_size(btn_new_game, 78, 30);
    lv_obj_align(btn_new_game, LV_ALIGN_TOP_RIGHT, -8, 8);
    lv_obj_add_event_cb(btn_new_game, screen_2048_new_game_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label_new = lv_label_create(btn_new_game);
    lv_label_set_text(label_new, "New");
    lv_obj_center(label_new);

    s_label_2048_score = lv_label_create(s_screen_2048);
    lv_obj_align(s_label_2048_score, LV_ALIGN_TOP_MID, 0, 16);

    s_obj_2048 = lv_100ask_2048_create(s_screen_2048);
    lv_obj_set_size(s_obj_2048, 220, 220);
    lv_obj_align(s_obj_2048, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_event_cb(s_obj_2048, screen_2048_value_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    screen_2048_update_score_text();
    lv_scr_load_anim(s_screen_2048, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

static lv_obj_t *s_screen_memory = NULL;
static lv_obj_t *s_obj_memory = NULL;
static lv_obj_t *s_screen_snake = NULL;
static lv_obj_t *s_obj_snake = NULL;
static lv_obj_t *s_label_snake_score = NULL;
static lv_obj_t *s_flappy_screen = NULL;
static lv_obj_t *s_flappy_back_btn = NULL;
static lv_obj_t *s_flappy_bird = NULL;
static lv_obj_t *s_flappy_pipe_top = NULL;
static lv_obj_t *s_flappy_pipe_bottom = NULL;
static lv_obj_t *s_flappy_score_label = NULL;
static lv_timer_t *s_flappy_timer = NULL;
static int16_t s_flappy_bird_y = 120;
static int16_t s_flappy_bird_vy = 0;
static int16_t s_flappy_pipe_x = 240;
static int16_t s_flappy_gap_y = 90;
static uint16_t s_flappy_score = 0;
static bool s_flappy_alive = true;
static bool s_flappy_pipe_passed = false;
static lv_point_t s_snake_touch_anchor = {0, 0};
static bool s_snake_touch_active = false;

static void screen_memory_back_to_game_list(void)
{
    lv_indev_t *indev = lv_indev_get_act();
    if (indev != NULL) {
        /* non-blocking: skip wait_release to avoid wakeup deadlock */
    }

    if (guider_ui.screen_game != NULL) {
        lv_scr_load_anim(guider_ui.screen_game, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
    }
}

static void screen_memory_back_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    screen_memory_back_to_game_list();
}

static void screen_memory_new_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED || s_obj_memory == NULL) {
        return;
    }
    lv_100ask_memory_game_set_map(s_obj_memory, 4, 4);
}

static void open_game_memory_screen(void)
{
    s_screen_memory = lv_obj_create(NULL);
    lv_obj_clear_flag(s_screen_memory, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_screen_memory, lv_color_hex(0xF1F6FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_screen_memory, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *btn_back = lv_btn_create(s_screen_memory);
    lv_obj_set_size(btn_back, 62, 36);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 16, 8);
    lv_obj_add_event_cb(btn_back, screen_memory_back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "Back");
    lv_obj_center(label_back);

    lv_obj_t *btn_new = lv_btn_create(s_screen_memory);
    lv_obj_set_size(btn_new, 78, 30);
    lv_obj_align(btn_new, LV_ALIGN_TOP_RIGHT, -8, 8);
    lv_obj_add_event_cb(btn_new, screen_memory_new_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label_new = lv_label_create(btn_new);
    lv_label_set_text(label_new, "New");
    lv_obj_center(label_new);

    lv_obj_t *title = lv_label_create(s_screen_memory);
    lv_label_set_text(title, "Memory Game");
    lv_obj_set_style_text_font(title, &songti_font_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    s_obj_memory = lv_100ask_memory_game_create(s_screen_memory);
    lv_obj_set_size(s_obj_memory, 220, 220);
    lv_obj_align(s_obj_memory, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_100ask_memory_game_set_map(s_obj_memory, 4, 4);

    lv_scr_load_anim(s_screen_memory, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

static void screen_snake_update_score_text(void)
{
    if (s_label_snake_score == NULL || s_obj_snake == NULL) {
        return;
    }

    if (lv_100ask_snake_is_over(s_obj_snake)) {
        lv_label_set_text_fmt(s_label_snake_score, "GAME OVER  SCORE:%d", lv_100ask_snake_get_score(s_obj_snake));
        return;
    }

    lv_label_set_text_fmt(s_label_snake_score, "SCORE: %d", lv_100ask_snake_get_score(s_obj_snake));
}

static void screen_snake_back_to_game_list(void)
{
    if (guider_ui.screen_game != NULL) {
        lv_scr_load_anim(guider_ui.screen_game, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
    }
}

static void screen_snake_back_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    screen_snake_back_to_game_list();
}

static void screen_snake_new_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED || s_obj_snake == NULL) {
        return;
    }
    lv_100ask_snake_new_game(s_obj_snake);
    screen_snake_update_score_text();
}

static void screen_snake_value_changed_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    screen_snake_update_score_text();
}

static void screen_snake_gesture_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev;

    if (s_obj_snake == NULL) {
        return;
    }

    indev = lv_event_get_indev(e);
    if (indev == NULL) {
        indev = lv_indev_get_act();
    }
    if (indev == NULL) {
        return;
    }

    if (code == LV_EVENT_PRESSED) {
        lv_indev_get_point(indev, &s_snake_touch_anchor);
        s_snake_touch_active = true;
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        s_snake_touch_active = false;
        return;
    }

    if (code == LV_EVENT_PRESSING && s_snake_touch_active) {
        lv_point_t vect;
        lv_point_t p;
        lv_indev_get_vect(indev, &vect);
        lv_indev_get_point(indev, &p);

        if (LV_ABS(vect.x) >= 2 || LV_ABS(vect.y) >= 2) {
            if (LV_ABS(vect.x) >= LV_ABS(vect.y)) {
                lv_100ask_snake_set_dir(s_obj_snake, (vect.x > 0) ? LV_DIR_RIGHT : LV_DIR_LEFT);
            } else {
                lv_100ask_snake_set_dir(s_obj_snake, (vect.y > 0) ? LV_DIR_BOTTOM : LV_DIR_TOP);
            }
            s_snake_touch_anchor = p;
            return;
        }

        {
            int dx = p.x - s_snake_touch_anchor.x;
            int dy = p.y - s_snake_touch_anchor.y;
            if (LV_ABS(dx) >= 4 || LV_ABS(dy) >= 4) {
                if (LV_ABS(dx) >= LV_ABS(dy)) {
                    lv_100ask_snake_set_dir(s_obj_snake, (dx > 0) ? LV_DIR_RIGHT : LV_DIR_LEFT);
                } else {
                    lv_100ask_snake_set_dir(s_obj_snake, (dy > 0) ? LV_DIR_BOTTOM : LV_DIR_TOP);
                }
                s_snake_touch_anchor = p;
            }
        }
        return;
    }

    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(indev);
        if (dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT || dir == LV_DIR_TOP || dir == LV_DIR_BOTTOM) {
            lv_100ask_snake_set_dir(s_obj_snake, dir);
        }
    }
}

static void open_game_snake_screen(void)
{
    s_screen_snake = lv_obj_create(NULL);
    lv_obj_add_flag(s_screen_snake, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_screen_snake, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(s_screen_snake, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_clear_flag(s_screen_snake, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_screen_snake, lv_color_hex(0xEEF7E7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_screen_snake, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *btn_back = lv_btn_create(s_screen_snake);
    lv_obj_set_size(btn_back, 62, 36);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 16, 8);
    lv_obj_add_event_cb(btn_back, screen_snake_back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "Back");
    lv_obj_center(label_back);

    lv_obj_t *btn_new = lv_btn_create(s_screen_snake);
    lv_obj_set_size(btn_new, 78, 30);
    lv_obj_align(btn_new, LV_ALIGN_TOP_RIGHT, -8, 8);
    lv_obj_add_event_cb(btn_new, screen_snake_new_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label_new = lv_label_create(btn_new);
    lv_label_set_text(label_new, "New");
    lv_obj_center(label_new);

    lv_obj_t *title = lv_label_create(s_screen_snake);
    lv_label_set_text(title, "\xE8\xB4\xAA\xE5\x90\x83\xE8\x9B\x87");
    lv_obj_set_style_text_font(title, &songti_font_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    s_label_snake_score = lv_label_create(s_screen_snake);
    lv_obj_align(s_label_snake_score, LV_ALIGN_TOP_MID, 0, 40);

    s_obj_snake = lv_100ask_snake_create(s_screen_snake);
    lv_obj_add_flag(s_obj_snake, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_obj_snake, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(s_obj_snake, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(s_obj_snake, 224, 224);
    lv_obj_align(s_obj_snake, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_add_event_cb(s_obj_snake, screen_snake_value_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(s_obj_snake, screen_snake_gesture_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(s_obj_snake, screen_snake_gesture_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_obj_snake, screen_snake_gesture_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(s_obj_snake, screen_snake_gesture_cb, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_add_event_cb(s_obj_snake, screen_snake_gesture_cb, LV_EVENT_PRESSING, NULL);

    lv_obj_add_event_cb(s_screen_snake, screen_snake_gesture_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(s_screen_snake, screen_snake_gesture_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_screen_snake, screen_snake_gesture_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(s_screen_snake, screen_snake_gesture_cb, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_add_event_cb(s_screen_snake, screen_snake_gesture_cb, LV_EVENT_PRESSING, NULL);

    screen_snake_update_score_text();
    lv_scr_load_anim(s_screen_snake, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

static void flappy_refresh_ui(void)
{
    if (s_flappy_bird != NULL) {
        lv_obj_set_pos(s_flappy_bird, 52, s_flappy_bird_y);
    }

    if (s_flappy_pipe_top != NULL) {
        lv_obj_set_pos(s_flappy_pipe_top, s_flappy_pipe_x, 0);
        lv_obj_set_size(s_flappy_pipe_top, 28, s_flappy_gap_y);
    }

    if (s_flappy_pipe_bottom != NULL) {
        lv_coord_t bottom_y = s_flappy_gap_y + 90;
        lv_coord_t bottom_h = 284 - bottom_y;
        if (bottom_h < 1) bottom_h = 1;
        lv_obj_set_pos(s_flappy_pipe_bottom, s_flappy_pipe_x, bottom_y);
        lv_obj_set_size(s_flappy_pipe_bottom, 28, bottom_h);
    }

    if (s_flappy_score_label != NULL) {
        if (s_flappy_alive) {
            lv_label_set_text_fmt(s_flappy_score_label, "Score: %u", s_flappy_score);
        } else {
            lv_label_set_text_fmt(s_flappy_score_label, "Game Over  Score: %u", s_flappy_score);
        }
    }
}

static void flappy_reset_game(void)
{
    s_flappy_bird_y = 120;
    s_flappy_bird_vy = 0;
    s_flappy_pipe_x = 240;
    s_flappy_gap_y = 70 + (rand() % 90);
    s_flappy_score = 0;
    s_flappy_alive = true;
    s_flappy_pipe_passed = false;
    flappy_refresh_ui();
}

static void flappy_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);

    if (!s_flappy_alive) {
        return;
    }

    s_flappy_bird_vy += 1;
    if (s_flappy_bird_vy > 5) s_flappy_bird_vy = 5;
    s_flappy_bird_y += s_flappy_bird_vy;

    s_flappy_pipe_x -= 2;
    if (s_flappy_pipe_x < -30) {
        s_flappy_pipe_x = 240;
        s_flappy_gap_y = 70 + (rand() % 90);
        s_flappy_pipe_passed = false;
    }

    if (!s_flappy_pipe_passed && (52 > (s_flappy_pipe_x + 28))) {
        s_flappy_pipe_passed = true;
        s_flappy_score++;
    }

    if (s_flappy_bird_y < 0 || (s_flappy_bird_y + 16) > 284) {
        s_flappy_alive = false;
    }

    if ((52 + 16) > s_flappy_pipe_x && 52 < (s_flappy_pipe_x + 28)) {
        lv_coord_t gap_bottom = s_flappy_gap_y + 90;
        if (s_flappy_bird_y < s_flappy_gap_y || (s_flappy_bird_y + 16) > gap_bottom) {
            s_flappy_alive = false;
        }
    }

    flappy_refresh_ui();
}

static void screen_flappy_input_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    if (s_flappy_alive) {
        s_flappy_bird_vy = -11;
    } else {
        flappy_reset_game();
    }
}

static void screen_flappy_back_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    if (s_flappy_timer != NULL) {
        lv_timer_del(s_flappy_timer);
        s_flappy_timer = NULL;
    }

    if (guider_ui.screen_game != NULL) {
        lv_scr_load_anim(guider_ui.screen_game, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }

    if (s_flappy_screen != NULL && lv_obj_is_valid(s_flappy_screen)) {
        lv_obj_del_async(s_flappy_screen);
    }
    s_flappy_screen = NULL;
    s_flappy_back_btn = NULL;
    s_flappy_bird = NULL;
    s_flappy_pipe_top = NULL;
    s_flappy_pipe_bottom = NULL;
    s_flappy_score_label = NULL;
}

static void open_game_flappy_screen(void)
{
    if (s_flappy_timer != NULL) {
        lv_timer_del(s_flappy_timer);
        s_flappy_timer = NULL;
    }
    if (s_flappy_screen != NULL && lv_obj_is_valid(s_flappy_screen)) {
        lv_obj_del(s_flappy_screen);
        s_flappy_screen = NULL;
    }

    s_flappy_screen = lv_obj_create(NULL);
    lv_obj_clear_flag(s_flappy_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_flappy_screen, lv_color_hex(0x6EC6FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_flappy_screen, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(s_flappy_screen, screen_flappy_input_cb, LV_EVENT_CLICKED, NULL);

    s_flappy_pipe_top = lv_obj_create(s_flappy_screen);
    lv_obj_set_style_bg_color(s_flappy_pipe_top, lv_color_hex(0x3EA63E), 0);
    lv_obj_set_style_border_width(s_flappy_pipe_top, 0, 0);
    lv_obj_set_style_radius(s_flappy_pipe_top, 2, 0);

    s_flappy_pipe_bottom = lv_obj_create(s_flappy_screen);
    lv_obj_set_style_bg_color(s_flappy_pipe_bottom, lv_color_hex(0x3EA63E), 0);
    lv_obj_set_style_border_width(s_flappy_pipe_bottom, 0, 0);
    lv_obj_set_style_radius(s_flappy_pipe_bottom, 2, 0);

    s_flappy_bird = lv_obj_create(s_flappy_screen);
    lv_obj_set_size(s_flappy_bird, 16, 16);
    lv_obj_set_style_radius(s_flappy_bird, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_flappy_bird, lv_color_hex(0xFFD24A), 0);
    lv_obj_set_style_border_width(s_flappy_bird, 0, 0);

    s_flappy_back_btn = lv_btn_create(s_flappy_screen);
    lv_obj_set_size(s_flappy_back_btn, 76, 40);
    lv_obj_align(s_flappy_back_btn, LV_ALIGN_TOP_LEFT, 12, 8);
    lv_obj_add_event_cb(s_flappy_back_btn, screen_flappy_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label_back = lv_label_create(s_flappy_back_btn);
    lv_label_set_text(label_back, "Back");
    lv_obj_center(label_back);

    s_flappy_score_label = lv_label_create(s_flappy_screen);
    lv_obj_align(s_flappy_score_label, LV_ALIGN_TOP_RIGHT, -10, 16);
    lv_obj_set_style_text_color(s_flappy_score_label, lv_color_hex(0xFFFFFF), 0);

    flappy_reset_game();
    s_flappy_timer = lv_timer_create(flappy_timer_cb, 35, NULL);
    lv_scr_load_anim(s_flappy_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

static void screen_game_item_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    lv_obj_t *btn = lv_event_get_current_target(e);
    const char *game_name = lv_list_get_btn_text(guider_ui.screen_game_list_1, btn);
    if (game_name == NULL) return;
    if (btn == guider_ui.screen_game_list_1_item[0]) {
        ESP_LOGI(TAG, "open 2048 from game list item 0");
        open_game_2048_screen();
        return;
    }

    if (btn == guider_ui.screen_game_list_1_item[1]) {
        ESP_LOGI(TAG, "open memory game from game list item 1");
        open_game_memory_screen();
        return;
    }

    if (btn == guider_ui.screen_game_list_1_item[2]) {
        ESP_LOGI(TAG, "open snake game from game list item 2");
        open_game_snake_screen();
        return;
    }

    if (btn == guider_ui.screen_game_list_1_item[3]) {
        ESP_LOGI(TAG, "open flappy bird from game list item 3");
        open_game_flappy_screen();
        return;
    }

    ESP_LOGI(TAG, "game item clicked: %s (not configured)", game_name);
}

void events_init_screen_game(lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_game, screen_game_event_handler, LV_EVENT_ALL, ui);

    for (int i = 0; i < _LIST_NUMBER; i++) {
        if (ui->screen_game_list_1_item[i] == NULL) continue;
        lv_obj_add_event_cb(ui->screen_game_list_1_item[i], screen_game_item_handler, LV_EVENT_CLICKED, ui);
    }
}

static void novel_display_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        switch(dir) {
        case LV_DIR_RIGHT:
        {
            lvgl_mark_novel_swipe_back_guard(450);
            lvgl_msg_send(LVGL_MSG_NOVEL_CLOSE_REQ, 0, NULL);
            /* non-blocking: skip wait_release to avoid wakeup deadlock */
            ui_load_scr_animation(&guider_ui,
                            &guider_ui.novel_list,            // ????????????h?????e?????????????????????????????????????????????????????????????
                            guider_ui.novel_list_del,          // ??????????
                            &guider_ui.novel_display_del,      // ??????????????????????????????????????????????????????????????????????????????????
                            setup_scr_novel_list,              // ????????????h?????e?????????????????????????????????????????????
                            LV_SCR_LOAD_ANIM_NONE, 0, 0,
                            false, true);         
                               break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

void events_init_novel_display (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->novel_display, novel_display_event_handler, LV_EVENT_ALL, ui);
}


static uint32_t s_novel_list_last_right_gesture_tick = 0;

static void novel_list_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        switch(dir) {
        case LV_DIR_RIGHT:
        {
            lvgl_mark_novel_swipe_back_guard(450);
            s_novel_list_last_right_gesture_tick = lv_tick_get();
            /* non-blocking: skip wait_release to avoid wakeup deadlock */
            ui_load_scr_animation(&guider_ui, &guider_ui.menu_screen, guider_ui.menu_screen_del, &guider_ui.novel_list_del, setup_scr_menu_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

static void novel_list_item_common_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    if (lvgl_should_block_novel_click()) {
        ESP_LOGI(TAG, "ignore novel click by shared swipe guard");
        return;
    }
    if (lv_tick_elaps(s_novel_list_last_right_gesture_tick) < 450) {
        ESP_LOGI(TAG, "ignore novel click after right-swipe");
        return;
    }

    lv_obj_t *btn = lv_event_get_current_target(e);
    const char *file_name = lv_list_get_btn_text(guider_ui.novel_list_list_1, btn);
    if (file_name == NULL || strlen(file_name) == 0) {
        ESP_LOGE(TAG, "invalid novel file name");
        return;
    }

    ESP_LOGI(TAG, "novel item clicked: %s", file_name);
    lvgl_msg_send(LVGL_MSG_NOVEL_OPEN_REQ, 0, file_name);
}
void events_init_novel_list (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->novel_list, novel_list_event_handler, LV_EVENT_ALL, ui);

    

 for (int i = 0; i < _LIST_NUMBER; i++) {
        // ???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????.??????????????????????????0???????????????????????????
        if (ui->novel_list_list_1_item[i] == NULL) continue;
        
        // ??????????????????????????????????????????????????????????????????b???????????????????????CLICKED????????????????????????????????LL???????????????????????????????
        lv_obj_add_event_cb(
            ui->novel_list_list_1_item[i],  // ???????????????????????.??????????????????
            novel_list_item_common_handler, // ????????????????g?????????????????????????????????????????????????i?????????????????
            LV_EVENT_CLICKED,               // ????????????????????????????????????????????????????????????????
            ui                              // ?????????????????????????????????????????????????????????
        );
    }

}

static void setting_screen_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        switch(dir) {
        case LV_DIR_RIGHT:
        {
            /* non-blocking: skip wait_release to avoid wakeup deadlock */
            ui_load_scr_animation(&guider_ui, &guider_ui.menu_screen, guider_ui.menu_screen_del, &guider_ui.setting_screen_del, setup_scr_menu_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}


// ?????????????????????????????????????????????????????????????????????????????????????????????????????????Fi???????????????
static void setting_screen_list_1_item0_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui,
            &guider_ui.screen_wifi_set, guider_ui.screen_wifi_set_del,
            &guider_ui.setting_screen_del, setup_scr_screen_wifi_set,
            LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
    }
}


// ????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
static void setting_screen_list_1_item0_click_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui,
            &guider_ui.screen_time_set,
            guider_ui.screen_time_set_del,
            &guider_ui.setting_screen_del,
            setup_scr_screen_time_set,
            LV_SCR_LOAD_ANIM_NONE, 0, 0,
            false, true);
    }
}


// ??????????????????????????????????????????????????????????????????????OTA?????????????????????????????????????????????????????????A?????????????????????????????????????????
static void setting_screen_list_1_item2_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui,
            &guider_ui.screen_ota,
            guider_ui.screen_ota_del,
            &guider_ui.setting_screen_del,
            setup_scr_screen_ota,
            LV_SCR_LOAD_ANIM_NONE, 0, 0,
            false, true);
    }
}


void events_init_setting_screen (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->setting_screen, setting_screen_event_handler, LV_EVENT_ALL, ui);

     lv_obj_add_event_cb(ui->setting_screen_list_1_item0,
        setting_screen_list_1_item0_click_handler, LV_EVENT_ALL, ui);

    lv_obj_add_event_cb(ui->setting_screen_list_1_item1,
        setting_screen_list_1_item0_event_handler, LV_EVENT_ALL, ui);

    lv_obj_add_event_cb(ui->setting_screen_list_1_item2,
        setting_screen_list_1_item2_handler, LV_EVENT_ALL, ui);
}



void events_init(lv_ui *ui)
{

}







// ??????????????????????????????????????????????
static void screen_img_list_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            /* non-blocking: skip wait_release to avoid wakeup deadlock */
            ui_load_scr_animation(&guider_ui,
                &guider_ui.menu_screen, guider_ui.menu_screen_del,
                &guider_ui.screen_img_list_del, setup_scr_menu_screen,
                LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
        }
    }
}

// ??????????????????????????????????????????????????
static void screen_img_list_item_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    lv_obj_t *btn = lv_event_get_current_target(e);
    const char *file_name = lv_list_get_btn_text(guider_ui.screen_img_list_list_1, btn);

    if (file_name == NULL || strlen(file_name) == 0) return;

    // ?????????????????????????????????????????????????????????????????????????????????????????????????$???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
    memset(img_full_path, 0, sizeof(img_full_path));
    for (int i = 0; i < s_file_list.count; i++) {
        if (strcmp(s_file_list.files[i].name, file_name) == 0) {
            snprintf(img_full_path, sizeof(img_full_path), "%s", s_file_list.files[i].full_path);
            break;
        }
    }

    if (strlen(img_full_path) > 0) {
        ESP_LOGI(TAG, "??????????????????????????????? %s", img_full_path);
    }

    // ???????????????????????????????????????????????????????????????
    ui_load_scr_animation(&guider_ui,
        &guider_ui.screen_img_display, guider_ui.screen_img_display_del,
        &guider_ui.screen_img_list_del, setup_scr_screen_img_display,
        LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
}

void events_init_screen_img_list(lv_ui *ui)
{
    // ???????????????????????????????????????????
    lv_obj_add_event_cb(ui->screen_img_list, screen_img_list_event_handler, LV_EVENT_ALL, ui);

    // ????$??????????????????????????????
    for (int i = 0; i < s_file_list.count; i++) {
        if (ui->screen_img_list_list_1_item[i] == NULL) continue;
        lv_obj_add_event_cb(
            ui->screen_img_list_list_1_item[i],
            screen_img_list_item_handler,
            LV_EVENT_CLICKED,
            ui
        );
    }
}

// ==================== ?????????????????????????????????????????.??????????????????====================

static void screen_img_display_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            // ???????????????????????????????????????????????????f?????????????????????????????????????????????????????????_del ?????????????????????????????????????????????????????????????
            img_display_free();

            /* non-blocking: skip wait_release to avoid wakeup deadlock */
            ui_load_scr_animation(&guider_ui,
                            &guider_ui.screen_img_list,          // ????????????h?????e??????????????????????????????????????????????????????????????
                            guider_ui.screen_img_list_del,        // ??????????
                            &guider_ui.screen_img_display_del,    // ??????????????????????????????????????????????????????????????????????????????????
                            setup_scr_screen_img_list,            // ????????????h?????e????????????????????????????????????????????
                            LV_SCR_LOAD_ANIM_NONE, 0, 0,
                            false, true);       
                         }
    }
}

void events_init_screen_img_display(lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_img_display, screen_img_display_event_handler, LV_EVENT_ALL, ui);
}






// ==================== WiFi??????????????????????????????????????????????====================

static lv_keyboard_mode_t wifi_last_kb_mode = LV_KEYBOARD_MODE_TEXT_LOWER;

// ??????????????????????????????????????/???????????????????????lvgl_keyboard.c ???????????????????????0??????
static void screen_wifi_set_kb_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_keyboard_mode_t cur_mode = lv_keyboard_get_mode(kb);
        if (cur_mode != wifi_last_kb_mode) {
            wifi_last_kb_mode = cur_mode;
            lv_indev_t *indev = lv_indev_get_act();
            if (indev != NULL) {
                /* non-blocking: skip wait_release to avoid wakeup deadlock */
            }
        }
    }
    else if (code == LV_EVENT_READY) {
        lv_obj_t *cur_ta = lv_keyboard_get_textarea(kb);

        if (cur_ta == guider_ui.screen_wifi_set_ta_ssid) {
            // SSID???????????????????????????????????????????????????????????????????????????
            lv_keyboard_set_textarea(guider_ui.screen_wifi_set_kb,
                                     guider_ui.screen_wifi_set_ta_pwd);
            lv_obj_clear_state(guider_ui.screen_wifi_set_ta_ssid, LV_STATE_FOCUSED);
            lv_obj_add_state(guider_ui.screen_wifi_set_ta_pwd, LV_STATE_FOCUSED);
            ESP_LOGI(TAG, "SSID input done, switch to password input");
        }
        else if (cur_ta == guider_ui.screen_wifi_set_ta_pwd) {
            // ????e?????????????????????????????????????????????????????????????????????
            lv_obj_clear_state(guider_ui.screen_wifi_set_ta_pwd, LV_STATE_FOCUSED);
            lv_obj_add_flag(guider_ui.screen_wifi_set_kb, LV_OBJ_FLAG_HIDDEN);
            ESP_LOGI(TAG, "password input done, keyboard hidden");
        }
    }
    else if (code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(guider_ui.screen_wifi_set_kb, LV_OBJ_FLAG_HIDDEN);
    }
}

// ?????????????????????ID?????????????????????
static void screen_wifi_set_ta_ssid_click_cb(lv_event_t *e)
{
    lv_keyboard_set_textarea(guider_ui.screen_wifi_set_kb,
                             guider_ui.screen_wifi_set_ta_ssid);
    lv_keyboard_set_mode(guider_ui.screen_wifi_set_kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    wifi_last_kb_mode = LV_KEYBOARD_MODE_TEXT_LOWER;
    lv_obj_clear_flag(guider_ui.screen_wifi_set_kb, LV_OBJ_FLAG_HIDDEN);
}

// ??????????????????????????????????????????????????????
static void screen_wifi_set_ta_pwd_click_cb(lv_event_t *e)
{
    lv_keyboard_set_textarea(guider_ui.screen_wifi_set_kb,
                             guider_ui.screen_wifi_set_ta_pwd);
    lv_keyboard_set_mode(guider_ui.screen_wifi_set_kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    wifi_last_kb_mode = LV_KEYBOARD_MODE_TEXT_LOWER;
    lv_obj_clear_flag(guider_ui.screen_wifi_set_kb, LV_OBJ_FLAG_HIDDEN);
}

// ???????????????????????????????????????????????
static void screen_wifi_set_btn_connect_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;// ?????????????????????????????
    lv_obj_add_flag(guider_ui.screen_wifi_set_kb, LV_OBJ_FLAG_HIDDEN);

    const char *ssid = lv_textarea_get_text(guider_ui.screen_wifi_set_ta_ssid);
    const char *pwd  = lv_textarea_get_text(guider_ui.screen_wifi_set_ta_pwd);

    if (ssid == NULL || strlen(ssid) == 0) {
        lv_label_set_text(guider_ui.screen_wifi_set_label_status,
                          "Please enter SSID!");
        lv_obj_set_style_text_color(guider_ui.screen_wifi_set_label_status,
                                    lv_color_hex(0xff0000), 0);
        return;
    }

    ESP_LOGI(TAG, "WiFi connecting: SSID=%s PWD=%s", ssid, pwd);

    // ?????????????????????????????
    lv_label_set_text(guider_ui.screen_wifi_set_label_status, "Connecting...");
    lv_obj_set_style_text_color(guider_ui.screen_wifi_set_label_status,
                                lv_color_hex(0xffff00), 0);

    // ??????????????????Fi???????????????
    wifi_manager_connect(ssid, pwd);
}

// ?????????????????????????????????????i?????????????????????????????
static void screen_wifi_set_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            /* non-blocking: skip wait_release to avoid wakeup deadlock */
            ui_load_scr_animation(&guider_ui,
                &guider_ui.setting_screen, guider_ui.setting_screen_del,
                &guider_ui.screen_wifi_set_del, setup_scr_setting_screen,
                LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
        }
    }
}

// ?????????????????????????????????????
void events_init_screen_wifi_set(lv_ui *ui)
{
    // ????????????????f?????????????????????????????????????????????????????????
    lv_obj_add_event_cb(ui->screen_wifi_set,
        screen_wifi_set_event_handler, LV_EVENT_ALL, ui);

    // SSID???????????????????????????????????
    lv_obj_add_event_cb(ui->screen_wifi_set_ta_ssid,
        screen_wifi_set_ta_ssid_click_cb, LV_EVENT_CLICKED, NULL);

    // ????e??????????????????????????????????????????????
    lv_obj_add_event_cb(ui->screen_wifi_set_ta_pwd,
        screen_wifi_set_ta_pwd_click_cb, LV_EVENT_CLICKED, NULL);

    // ??????????????????????????????
    lv_obj_add_event_cb(ui->screen_wifi_set_kb,
        screen_wifi_set_kb_event_cb, LV_EVENT_ALL, NULL);

    // ?????????????????????????????
    lv_obj_add_event_cb(ui->screen_wifi_set_btn_connect,
        screen_wifi_set_btn_connect_cb, LV_EVENT_ALL, NULL);
}





// ==================================================================
// ????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
// ==================================================================

/* ????????????????????????????????????????????????*/
extern struct tm timeinfo;
extern time_t now;

/* ??????????????????????????????????????????????????????????????????????????????????????i???????????????????????????????????????????????*/
static lv_obj_t *s_time_active_ta = NULL;

/* ---------- ????????????????????????????????????????????????????????????????????????????????---------- */
static void screen_time_set_ta_click_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    lv_obj_t *ta = lv_event_get_target(e);
    s_time_active_ta = ta;

    /* ????????????????????????????i???????????????????????????????????????????????????????????????*/
    lv_obj_clear_state(guider_ui.screen_time_set_ta_year,  LV_STATE_FOCUSED);
    lv_obj_clear_state(guider_ui.screen_time_set_ta_month, LV_STATE_FOCUSED);
    lv_obj_clear_state(guider_ui.screen_time_set_ta_day,   LV_STATE_FOCUSED);
    lv_obj_clear_state(guider_ui.screen_time_set_ta_hour,  LV_STATE_FOCUSED);
    lv_obj_clear_state(guider_ui.screen_time_set_ta_min,   LV_STATE_FOCUSED);
    lv_obj_clear_state(guider_ui.screen_time_set_ta_sec,   LV_STATE_FOCUSED);

    /* ??????????????????????????????????????????????????????????????????????????*/
    lv_obj_add_state(ta, LV_STATE_FOCUSED);

    /* ????????????????????????????????????????$???????????????????????????????????????????????????????????*/
    lv_keyboard_set_textarea(guider_ui.screen_time_set_kb, ta);
    lv_obj_clear_flag(guider_ui.screen_time_set_kb, LV_OBJ_FLAG_HIDDEN);
}

/* ---------- ???????????????????????????????????/??????---------- */
static void screen_time_set_kb_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        /* ?????.??????????????????????????????????????????????????????????????????????????????????????????*/
        lv_obj_add_flag(guider_ui.screen_time_set_kb, LV_OBJ_FLAG_HIDDEN);

        if (s_time_active_ta != NULL) {
            lv_obj_clear_state(s_time_active_ta, LV_STATE_FOCUSED);
            s_time_active_ta = NULL;
        }
    }
}


/* ---------- ? ????????????????f??????????????????????????????????????????????????????????????????P???????????????---------- */
static void screen_time_set_btn_confirm_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    lv_obj_add_flag(guider_ui.screen_time_set_kb, LV_OBJ_FLAG_HIDDEN);

    int year  = atoi(lv_textarea_get_text(guider_ui.screen_time_set_ta_year));
    int month = atoi(lv_textarea_get_text(guider_ui.screen_time_set_ta_month));
    int day   = atoi(lv_textarea_get_text(guider_ui.screen_time_set_ta_day));
    int hour  = atoi(lv_textarea_get_text(guider_ui.screen_time_set_ta_hour));
    int min   = atoi(lv_textarea_get_text(guider_ui.screen_time_set_ta_min));
    int sec   = atoi(lv_textarea_get_text(guider_ui.screen_time_set_ta_sec));

    if (year < 2000 || year > 2099 ||
        month < 1   || month > 12  ||
        day < 1     || day > 31    ||
        hour < 0    || hour > 23   ||
        min < 0     || min > 59    ||
        sec < 0     || sec > 59)
    {
        lv_label_set_text(guider_ui.screen_time_set_label_status,
                          "input out of range");
        lv_obj_set_style_text_color(guider_ui.screen_time_set_label_status,
                                    lv_color_hex(0xFF0000), 0);
        return;
    }

    esp_err_t rtc_ret = rtc_time_service_set_manual_and_sync(year, month, day, hour, min, sec);
    if (rtc_ret != ESP_OK) {
        lv_label_set_text(guider_ui.screen_time_set_label_status,
                          "set rtc/system failed");
        lv_obj_set_style_text_color(guider_ui.screen_time_set_label_status,
                                    lv_color_hex(0xFF0000), 0);
        ESP_LOGW(TAG, "manual sync failed: %s", esp_err_to_name(rtc_ret));
        return;
    }

    lv_label_set_text(guider_ui.screen_time_set_label_status,
                      "time set success");
    lv_obj_set_style_text_color(guider_ui.screen_time_set_label_status,
                                lv_color_hex(0x00FF00), 0);

    ESP_LOGI(TAG, "manual set and sync ok: %04d-%02d-%02d %02d:%02d:%02d",
             year, month, day, hour, min, sec);
}

static void screen_time_set_btn_ntp_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;/* ?????????????????????????????*/
    lv_obj_add_flag(guider_ui.screen_time_set_kb, LV_OBJ_FLAG_HIDDEN);

    /* ?????????????????????????????????????????????????????????????i???????????*/
    if (sntp_trigger_sem == NULL) {
        lv_label_set_text(guider_ui.screen_time_set_label_status,
                          "NTP trigger unavailable");
        lv_obj_set_style_text_color(guider_ui.screen_time_set_label_status,
                                    lv_color_hex(0xFF0000), 0);
        return;
    }

    /* ? ???????????????????????????????????????????????????????????sntp_interval_task ??????????????TP???????????????*/
    xSemaphoreGive(sntp_trigger_sem);

    /* ?????????????????????????????*/
    lv_label_set_text(guider_ui.screen_time_set_label_status,
                      "璇锋眰鍚屾涓?..");
    lv_obj_set_style_text_color(guider_ui.screen_time_set_label_status,
                                lv_color_hex(0xFFFF00), 0);

    ESP_LOGI(TAG, "NTP sync trigger sent");
}

/* ---------- ??????????????????????????????????i?????????????????????????????---------- */
static void screen_time_set_gesture_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            /* ?????????????????????????????*/
            lv_obj_add_flag(guider_ui.screen_time_set_kb, LV_OBJ_FLAG_HIDDEN);

            /* non-blocking: skip wait_release to avoid wakeup deadlock */
            ui_load_scr_animation(&guider_ui,
                &guider_ui.setting_screen,
                guider_ui.setting_screen_del,
                &guider_ui.screen_time_set_del,
                setup_scr_setting_screen,
                LV_SCR_LOAD_ANIM_NONE, 0, 0,
                false, true);
        }
    }
}

/* ---------- ?????????????????????????????????????????????????????---------- */
void events_init_screen_time_set(lv_ui *ui)
{
    /* ????????????????f??????????????????????????????????????????????????????????*/
    lv_obj_add_event_cb(ui->screen_time_set,
        screen_time_set_gesture_cb, LV_EVENT_ALL, ui);

    /* 6?????????????????????????????????????????????????????????????????????????????*/
    lv_obj_add_event_cb(ui->screen_time_set_ta_year,
        screen_time_set_ta_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->screen_time_set_ta_month,
        screen_time_set_ta_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->screen_time_set_ta_day,
        screen_time_set_ta_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->screen_time_set_ta_hour,
        screen_time_set_ta_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->screen_time_set_ta_min,
        screen_time_set_ta_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->screen_time_set_ta_sec,
        screen_time_set_ta_click_cb, LV_EVENT_CLICKED, NULL);

    /* ??????????????????????????????*/
    lv_obj_add_event_cb(ui->screen_time_set_kb,
        screen_time_set_kb_event_cb, LV_EVENT_ALL, NULL);

    /* ????????????????????????????????????????????*/
    lv_obj_add_event_cb(ui->screen_time_set_btn_confirm,
        screen_time_set_btn_confirm_cb, LV_EVENT_ALL, NULL);

    /* ????????????????f????????????????????????????*/
    lv_obj_add_event_cb(ui->screen_time_set_btn_ntp,
        screen_time_set_btn_ntp_cb, LV_EVENT_ALL, NULL);
}

// ==================================================================
// Weather screen
// ==================================================================

static void screen_weather_gesture_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_GESTURE) return;

    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_RIGHT) {
        open_clock_quick_screen();
    }
}

void events_init_screen_weather(lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_weather, screen_weather_gesture_cb, LV_EVENT_ALL, ui);
}

// ==================================================================
// OTA???????????????????????????????????
// ==================================================================

static void screen_ota_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            /* non-blocking: skip wait_release to avoid wakeup deadlock */
            ui_load_scr_animation(&guider_ui,
                &guider_ui.setting_screen, guider_ui.setting_screen_del,
                &guider_ui.screen_ota_del, setup_scr_setting_screen,
                LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
        }
    }
}

static void screen_ota_list_1_item0_handler(lv_event_t *e)  // OneNET??????????????????????????????
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui,
            &guider_ui.screen_ota_onenet, guider_ui.screen_ota_onenet_del,
            &guider_ui.screen_ota_del, setup_scr_screen_ota_onenet,
            LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
    }
}

static void screen_ota_list_1_item1_handler(lv_event_t *e)  // ???????????????????????????????
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui,
            &guider_ui.screen_ota_local, guider_ui.screen_ota_local_del,
            &guider_ui.screen_ota_del, setup_scr_screen_ota_local,
            LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
    }
}

static void screen_ota_list_1_item2_handler(lv_event_t *e)  // ????$??????????????????????????
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        ui_load_scr_animation(&guider_ui,
            &guider_ui.screen_ota_switch, guider_ui.screen_ota_switch_del,
            &guider_ui.screen_ota_del, setup_scr_screen_ota_switch,
            LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
    }
}

void events_init_screen_ota(lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_ota, screen_ota_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_ota_list_1_item0, screen_ota_list_1_item0_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->screen_ota_list_1_item1, screen_ota_list_1_item1_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->screen_ota_list_1_item2, screen_ota_list_1_item2_handler, LV_EVENT_CLICKED, NULL);
}

// ==================================================================
// OneNET??????????????????????????????????????????????????????????????
// ==================================================================

static void screen_ota_onenet_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            /* non-blocking: skip wait_release to avoid wakeup deadlock */
            ui_load_scr_animation(&guider_ui,
                &guider_ui.screen_ota, guider_ui.screen_ota_del,
                &guider_ui.screen_ota_onenet_del, setup_scr_screen_ota,
                LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
        }
    }
}

static void screen_ota_onenet_btn_start_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    lv_obj_t *btn_lbl = lv_obj_get_child(guider_ui.screen_ota_onenet_btn_start, 0);
    if (btn_lbl == NULL) return;

    // ??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
    const char *btn_text = lv_label_get_text(btn_lbl);

    if (strcmp(btn_text, "Upgrade done") == 0) {
        // ??????????????????????????????????????????????????????????f?????????????????????????????????????.?????????????????????????????????????????????????????????h?????????????n????????????????????????????????????????????
        lv_obj_add_flag(guider_ui.screen_ota_onenet_btn_jump, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(btn_lbl, "Check & upgrade");
        lv_label_set_text(guider_ui.screen_ota_onenet_label_status, "Idle");
        lv_obj_set_style_text_color(guider_ui.screen_ota_onenet_label_status, lv_color_hex(0x00ff00), 0);
        ESP_LOGI(TAG, "[OTA] reset from done state");
    } else {
        // ????$???????????????????????????????????e????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
        lv_label_set_text(btn_lbl, "Upgrade done");
        lv_label_set_text(guider_ui.screen_ota_onenet_label_status, "Checking...");
        lv_obj_set_style_text_color(guider_ui.screen_ota_onenet_label_status, lv_color_hex(0xffff00), 0);
        // ???????????????onenet_ota_start() ?????????????????a????????????????
        onenet_ota_start();
        ESP_LOGI(TAG, "OneNET OTA start");
    }
}

static void screen_ota_onenet_btn_jump_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    lv_label_set_text(guider_ui.screen_ota_onenet_label_status, "??????????????????????????g??..");
    lv_obj_set_style_text_color(guider_ui.screen_ota_onenet_label_status, lv_color_hex(0xff0000), 0);

    // ??????????????????????????g???????????????????????????????????????????????????????????????????
    onenet_ota_jump_and_restart();
}

void events_init_screen_ota_onenet(lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_ota_onenet, screen_ota_onenet_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_ota_onenet_btn_start, screen_ota_onenet_btn_start_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui->screen_ota_onenet_btn_jump, screen_ota_onenet_btn_jump_handler, LV_EVENT_ALL, NULL);
}

// ==================================================================
// ???????????????????????????????????????????????????????????????
// ==================================================================

static void screen_ota_local_event_handler(lv_event_t *e)
{
    static int16_t s_last_x = 0;
    static int16_t s_last_y = 0;
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        lv_indev_t *indev = lv_indev_get_act();
        if (indev != NULL) {
            lv_point_t p;
            lv_indev_get_point(indev, &p);
            s_last_x = p.x;
            s_last_y = p.y;
        }
        return;
    }

    if (code == LV_EVENT_PRESSING) {
        lv_indev_t *indev = lv_indev_get_act();
        if (indev != NULL) {
            lv_point_t p;
            lv_indev_get_point(indev, &p);
            int diff_x = p.x - s_last_x;
            int diff_y = p.y - s_last_y;

            // ???????????????????????????????????????????????????????????????????????????URE????????????????????????????f???????????????????????????????????????????????????????????????
            if (diff_x > 35 && LV_ABS(diff_x) > (LV_ABS(diff_y) + 10)) {
                /* non-blocking: skip wait_release to avoid wakeup deadlock */
                ui_load_scr_animation(&guider_ui,
                    &guider_ui.screen_ota, guider_ui.screen_ota_del,
                    &guider_ui.screen_ota_local_del, setup_scr_screen_ota,
                    LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
                return;
            }
        }
    }

    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            // ????????????????????OTA????????????????????????????g???????????????????????????????????????????????????????????????????
            // TODO: ?????????????????????????????????????????????????????????????
            // if (g_is_ota_running) {
            //     ESP_LOGW(TAG, "OTA????????????????????????????g???????????????????????????????????????????);
            //     return;
            // }
            /* non-blocking: skip wait_release to avoid wakeup deadlock */
            ui_load_scr_animation(&guider_ui,
                &guider_ui.screen_ota, guider_ui.screen_ota_del,
                &guider_ui.screen_ota_local_del, setup_scr_screen_ota,
                LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
        }
    }
}

static char ota_local_full_path[512];

// ??????????????????A???????????????????????????????????lvgl_process_msg_queue ?????????????????????????????????
static lv_obj_t *s_ota_local_msgbox = NULL;
static lv_obj_t *s_ota_local_result_msgbox = NULL;

// ???????????????????????????????????A MsgBox ???????????????
void local_ota_update_msgbox(const char *text)
{
    if (s_ota_local_msgbox == NULL) return;
    lv_obj_t *ta = lv_msgbox_get_text(s_ota_local_msgbox);
    if (ta == NULL) return;
    lv_label_set_text(ta, text);
}

void local_ota_close_msgbox(void)
{
    if (s_ota_local_msgbox != NULL) {
        lv_msgbox_close(s_ota_local_msgbox);
        s_ota_local_msgbox = NULL;
    }
}

static void ota_local_result_panel_close(void)
{
    if (s_ota_local_result_msgbox != NULL && lv_obj_is_valid(s_ota_local_result_msgbox)) {
        lv_obj_t *obj = s_ota_local_result_msgbox;
        s_ota_local_result_msgbox = NULL;
        lv_obj_del_async(obj);
    } else {
        s_ota_local_result_msgbox = NULL;
    }
}

static void ota_local_result_exit_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ota_local_result_panel_close();

    if (guider_ui.screen_ota == NULL || !lv_obj_is_valid(guider_ui.screen_ota)) {
        setup_scr_screen_ota(&guider_ui);
    }
    lv_scr_load_anim(guider_ui.screen_ota, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

static void ota_local_result_jump_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ota_local_result_panel_close();
    local_ota_switch_partition();
}

static void ota_local_result_panel_delete_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_DELETE) {
        s_ota_local_result_msgbox = NULL;
    }
}

// ????????????????????OTA?????????????????????????????????????????????????????????????????????l_display.c???????????????????????????????????????????
void local_ota_set_running(bool running)
{
    g_is_ota_running = running;
}

void local_ota_handle_complete_result(int result_code)
{
    ota_local_result_panel_close();

    if (result_code != (int)LVGL_OTA_RESULT_SUCCESS) {
        return;
    }

    s_ota_local_result_msgbox = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_ota_local_result_msgbox, 240, 284);
    lv_obj_center(s_ota_local_result_msgbox);
    lv_obj_clear_flag(s_ota_local_result_msgbox, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(s_ota_local_result_msgbox, 0, 0);
    lv_obj_set_style_border_width(s_ota_local_result_msgbox, 0, 0);
    lv_obj_set_style_bg_color(s_ota_local_result_msgbox, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_ota_local_result_msgbox, LV_OPA_50, 0);
    lv_obj_add_event_cb(s_ota_local_result_msgbox, ota_local_result_panel_delete_cb, LV_EVENT_DELETE, NULL);

    lv_obj_t *card = lv_obj_create(s_ota_local_result_msgbox);
    lv_obj_set_size(card, 200, 116);
    lv_obj_center(card);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1F2937), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, "鏈湴鍗囩骇");
    lv_obj_set_style_text_font(title, &songti_font_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *text = lv_label_create(card);
    lv_label_set_text(text, "涓嬭浇鎴愬姛");
    lv_obj_set_style_text_font(text, &songti_font_16, 0);
    lv_obj_set_style_text_color(text, lv_color_hex(0xD1D5DB), 0);
    lv_obj_align(text, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t *btn_jump = lv_btn_create(card);
    lv_obj_set_size(btn_jump, 72, 30);
    lv_obj_align(btn_jump, LV_ALIGN_BOTTOM_LEFT, 18, -12);
    lv_obj_set_style_radius(btn_jump, 8, 0);
    lv_obj_set_style_border_width(btn_jump, 0, 0);
    lv_obj_set_style_bg_color(btn_jump, lv_color_hex(0x2F80ED), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn_jump, lv_color_hex(0x2469C8), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_event_cb(btn_jump, ota_local_result_jump_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_jump = lv_label_create(btn_jump);
    lv_label_set_text(lbl_jump, "璺宠浆");
    lv_obj_set_style_text_font(lbl_jump, &songti_font_16, 0);
    lv_obj_set_style_text_color(lbl_jump, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl_jump);

    lv_obj_t *btn_exit = lv_btn_create(card);
    lv_obj_set_size(btn_exit, 72, 30);
    lv_obj_align(btn_exit, LV_ALIGN_BOTTOM_RIGHT, -18, -12);
    lv_obj_set_style_radius(btn_exit, 8, 0);
    lv_obj_set_style_border_width(btn_exit, 0, 0);
    lv_obj_set_style_bg_color(btn_exit, lv_color_hex(0x6B7280), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn_exit, lv_color_hex(0x4B5563), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_event_cb(btn_exit, ota_local_result_exit_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_exit = lv_label_create(btn_exit);
    lv_label_set_text(lbl_exit, "Exit");
    lv_obj_set_style_text_font(lbl_exit, &songti_font_16, 0);
    lv_obj_set_style_text_color(lbl_exit, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl_exit);
}

static void screen_ota_local_item_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;// ??????????????????????????????????????????i??????????????????A?????????????????????????????????
    if (g_is_ota_running) return;
    lv_obj_t *btn = lv_event_get_current_target(e);
    const char *file_name = lv_list_get_btn_text(guider_ui.screen_ota_local_list_1, btn);
    if (file_name == NULL || strlen(file_name) == 0) return;

    // ??????????????????file_list????????????????????????????????????????????????l_path
    ota_local_full_path[0] = '\0';
    for (int i = 0; i < s_file_list.count; i++) {
        if (strcmp(s_file_list.files[i].name, file_name) == 0) {
            snprintf(ota_local_full_path, sizeof(ota_local_full_path),
                     "%s", s_file_list.files[i].full_path);
            break;
        }
    }
    if (ota_local_full_path[0] == '\0') {
        ESP_LOGW(TAG, "local ota file not found: %s", file_name);
        return;
    }

    ESP_LOGI(TAG, "??????????????????????????????? %s", ota_local_full_path);

    // ????????????????????????????f??????????????????????????????????????????????????????????????????????????????????
    if (s_ota_local_msgbox != NULL) {
        lv_msgbox_close(s_ota_local_msgbox);
        s_ota_local_msgbox = NULL;
    }
    s_ota_local_msgbox = lv_msgbox_create(NULL, "Local OTA", "Downloading...", NULL, false);
    lv_obj_set_size(s_ota_local_msgbox, 200, 80);
    lv_obj_center(s_ota_local_msgbox);
    lv_obj_set_style_text_font(lv_msgbox_get_title(s_ota_local_msgbox), &songti_font_16, 0);
    lv_obj_set_style_text_font(lv_msgbox_get_text(s_ota_local_msgbox), &songti_font_16, 0);

    // ????????????????????OTA????????????????????????????????????????????????????????????????????????????????????????????????????????
    g_is_ota_running = true;

    // ????$????????????????????????????????????????????n???????????a?????????????????????????????????GL?????????????????????
    if (local_ota_start(ota_local_full_path) != ESP_OK) {
        g_is_ota_running = false;
        local_ota_update_msgbox("Start failed");
    }
}

void events_init_screen_ota_local(lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_ota_local, screen_ota_local_event_handler, LV_EVENT_ALL, ui);
    // ?????????????????????????????????????????????i?????????????????i????????????????????????????????????????st????????????????????????????????????????????
    lv_obj_add_event_cb(ui->screen_ota_local_list_1, screen_ota_local_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_flag(ui->screen_ota_local_list_1, LV_OBJ_FLAG_GESTURE_BUBBLE);

    for (int i = 0; i < 20; i++) {
        if (ui->screen_ota_local_list_1_item[i] == NULL) continue;
        lv_obj_add_flag(ui->screen_ota_local_list_1_item[i], LV_OBJ_FLAG_GESTURE_BUBBLE);
        lv_obj_add_event_cb(
            ui->screen_ota_local_list_1_item[i],
            screen_ota_local_event_handler,
            LV_EVENT_ALL,
            ui
        );
        lv_obj_add_event_cb(
            ui->screen_ota_local_list_1_item[i],
            screen_ota_local_item_handler,
            LV_EVENT_CLICKED,
            NULL
        );
    }
}

// ==================================================================
// ????$????????????????????????????????????????????????e????????
// ==================================================================

static void screen_ota_switch_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            /* non-blocking: skip wait_release to avoid wakeup deadlock */
            ui_load_scr_animation(&guider_ui,
                &guider_ui.screen_ota, guider_ui.screen_ota_del,
                &guider_ui.screen_ota_switch_del, setup_scr_screen_ota,
                LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
        }
    }
}

static void screen_ota_switch_btn_switch_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;// ?????????????????????????????????????????????????????????????
    local_ota_switch_partition();
    ESP_LOGI(TAG, "switch partition and restart");
}

void events_init_screen_ota_switch(lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_ota_switch, screen_ota_switch_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_ota_switch_btn_switch, screen_ota_switch_btn_switch_handler, LV_EVENT_ALL, NULL);
}















