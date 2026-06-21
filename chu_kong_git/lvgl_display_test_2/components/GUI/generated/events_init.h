/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/


#ifndef EVENTS_INIT_H_
#define EVENTS_INIT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "gui_guider.h"
#include "wifi_manager.h"
void events_init(lv_ui *ui);

void events_init_clock_screen(lv_ui *ui);
void events_init_menu_screen(lv_ui *ui);
void events_init_novel_display(lv_ui *ui);
void events_init_novel_list(lv_ui *ui);
void events_init_setting_screen(lv_ui *ui);
void events_init_screen_game(lv_ui *ui);

void events_init_screen_img_list(lv_ui *ui);
void events_init_screen_img_display(lv_ui *ui);
void events_init_screen_wifi_set(lv_ui *ui);
void events_init_screen_time_set(lv_ui *ui);
void events_init_screen_weather(lv_ui *ui);

void events_init_screen_ota(lv_ui *ui);
void events_init_screen_ota_onenet(lv_ui *ui);
void events_init_screen_ota_local(lv_ui *ui);
void events_init_screen_ota_switch(lv_ui *ui);

// ★★★ OTA运行状态控制（供lvgl_display.c调用）★★★
void local_ota_set_running(bool running);
void local_ota_handle_complete_result(int result_code);

// ★★★ 时钟界面Label点击切换颜色 ★★★
void clock_screen_color_event_cb(lv_event_t *e); 
void clock_screen_set_weather_brief(const char *text);
void clock_screen_set_weather_detail(const char *city, const char *condition, int high, int low, int humidity);
void wifi_quick_set_state(bool connected);
void quick_time_sync_set_status(const char *text, uint32_t color_hex);
void weather_ui_set_status(const char *text, uint32_t color_hex);
void weather_ui_refresh_from_snapshot(void);

#ifdef __cplusplus
}
#endif
#endif /* EVENT_CB_H_ */
