/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "video_player.h"


#define  _LIST_NUMBER      40
typedef struct
{
  
	lv_obj_t *clock_screen;
	bool clock_screen_del;
	lv_obj_t *clock_screen_cont_2;
	lv_obj_t *clock_screen_label_time;    // 时间 Label（14:20）
	lv_obj_t *clock_screen_label_date;    // 日期 Label（周六 8月5日）


	lv_obj_t *menu_screen;
	bool menu_screen_del;
	lv_obj_t *menu_screen_list_1;
	lv_obj_t *menu_screen_list_1_item0;
	lv_obj_t *menu_screen_list_1_item1;
	lv_obj_t *menu_screen_list_1_item2;
	lv_obj_t *menu_screen_list_1_item3;  // 视频菜单项
	lv_obj_t *menu_screen_list_1_item4;  // 游戏菜单项

	lv_obj_t *novel_display;
	bool novel_display_del;
	lv_obj_t *novel_display_label_1;
	lv_obj_t *novel_display_spangroup_1;
	lv_span_t *novel_display_spangroup_1_span;

	lv_obj_t *novel_list;
	bool novel_list_del;
	lv_obj_t *novel_list_list_1;
	lv_obj_t *novel_list_list_1_item[_LIST_NUMBER];

	lv_obj_t *setting_screen;
	bool setting_screen_del;
	lv_obj_t *setting_screen_list_1;
	lv_obj_t *setting_screen_list_1_item0;
	lv_obj_t *setting_screen_list_1_item1;
	lv_obj_t *setting_screen_list_1_item2;

	lv_obj_t *screen_game;
	bool screen_game_del;
	lv_obj_t *screen_game_list_1;
	lv_obj_t *screen_game_list_1_item[_LIST_NUMBER];

	 // ★★★ 新增：图片列表界面 ★★★
    lv_obj_t *screen_img_list;
    bool screen_img_list_del;
    lv_obj_t *screen_img_list_list_1;
    lv_obj_t *screen_img_list_list_1_item[_LIST_NUMBER];

    // ★★★ 新增：图片显示界面 ★★★
    lv_obj_t *screen_img_display;
    bool screen_img_display_del;
    lv_obj_t *screen_img_display_img;      // 图片对象

	 // ★★★ 新增：WiFi设置界面 ★★★
    lv_obj_t *screen_wifi_set;
    bool screen_wifi_set_del;
    lv_obj_t *screen_wifi_set_ta_ssid;       // SSID输入框
    lv_obj_t *screen_wifi_set_ta_pwd;        // 密码输入框
    lv_obj_t *screen_wifi_set_kb;            // 键盘
    lv_obj_t *screen_wifi_set_btn_connect;   // 连接按钮
    lv_obj_t *screen_wifi_set_label_status;  // 状态标签


	 // ★★★ 新增：时间设置界面 ★★★
    lv_obj_t *screen_time_set;
    bool screen_time_set_del;
    lv_obj_t *screen_time_set_ta_year;       // 年输入框
    lv_obj_t *screen_time_set_ta_month;      // 月输入框
    lv_obj_t *screen_time_set_ta_day;        // 日输入框
    lv_obj_t *screen_time_set_ta_hour;       // 时输入框
    lv_obj_t *screen_time_set_ta_min;        // 分输入框
    lv_obj_t *screen_time_set_ta_sec;        // 秒输入框
    lv_obj_t *screen_time_set_kb;            // 数字键盘
    lv_obj_t *screen_time_set_btn_confirm;   // 手动确认按钮
    lv_obj_t *screen_time_set_btn_ntp;       // 网络更新按钮
    lv_obj_t *screen_time_set_label_status;  // 状态标签

     // ★★★ 新增：天气界面 ★★★
    lv_obj_t *screen_weather;
    bool screen_weather_del;
    lv_obj_t *screen_weather_label_city;
    lv_obj_t *screen_weather_label_condition;
    lv_obj_t *screen_weather_label_temp;
    lv_obj_t *screen_weather_label_humidity;
    lv_obj_t *screen_weather_label_update;
    lv_obj_t *screen_weather_label_status;

	 // ★★★ OTA主界面 ★★★
    lv_obj_t *screen_ota;
    bool screen_ota_del;
    lv_obj_t *screen_ota_list_1;
    lv_obj_t *screen_ota_list_1_item0;   // OneNET平台升级
    lv_obj_t *screen_ota_list_1_item1;   // 本地升级
    lv_obj_t *screen_ota_list_1_item2;   // 切换分区

	 // ★★★ OneNET平台升级界面 ★★★
    lv_obj_t *screen_ota_onenet;
    bool screen_ota_onenet_del;
    lv_obj_t *screen_ota_onenet_label_version;
    lv_obj_t *screen_ota_onenet_label_status;
    lv_obj_t *screen_ota_onenet_btn_start;
    lv_obj_t *screen_ota_onenet_btn_jump;   // 跳转按钮（OTA下载成功后显示）

	 // ★★★ 本地升级界面 ★★★
    lv_obj_t *screen_ota_local;
    bool screen_ota_local_del;
    lv_obj_t *screen_ota_local_list_1;
    lv_obj_t *screen_ota_local_list_1_item[20];

	 // ★★★ 切换分区界面 ★★★
    lv_obj_t *screen_ota_switch;
    bool screen_ota_switch_del;
    lv_obj_t *screen_ota_switch_label_curr_part;
    lv_obj_t *screen_ota_switch_label_standby_part;
    lv_obj_t *screen_ota_switch_label_version;
    lv_obj_t *screen_ota_switch_btn_switch;

	 // ★★★ 视频列表界面 ★★★
    lv_obj_t *video_list;
    bool video_list_del;
    lv_obj_t *video_list_list;
    lv_obj_t *video_list_list_item[_LIST_NUMBER];

	 // ★★★ 视频播放界面 ★★★
    lv_obj_t *video_player;
    bool video_player_del;
    lv_obj_t *video_player_img;
    lv_obj_t *video_player_btn_touch;
    lv_obj_t *video_player_btn_close;
    video_format_t video_player_format;

}lv_ui;

typedef void (*ui_setup_scr_t)(lv_ui * ui);

void ui_init_style(lv_style_t * style);

void ui_load_scr_animation(lv_ui *ui, lv_obj_t ** new_scr, bool new_scr_del, bool * old_scr_del, ui_setup_scr_t setup_scr,
                           lv_scr_load_anim_t anim_type, uint32_t time, uint32_t delay, bool is_clean, bool auto_del);

void ui_animation(void * var, int32_t duration, int32_t delay, int32_t start_value, int32_t end_value, lv_anim_path_cb_t path_cb,
                       uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                       lv_anim_exec_xcb_t exec_cb, lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);


void init_scr_del_flag(lv_ui *ui);

void setup_ui(lv_ui *ui);

void init_keyboard(lv_ui *ui);

extern lv_ui guider_ui;


void setup_scr_clock_screen(lv_ui *ui);
void setup_scr_menu_screen(lv_ui *ui);
void setup_scr_novel_display(lv_ui *ui);
void setup_scr_novel_list(lv_ui *ui);
void setup_scr_setting_screen(lv_ui *ui);
void setup_scr_screen_game(lv_ui *ui);

void setup_scr_screen_img_list(lv_ui *ui);        // ★ 新增
void setup_scr_screen_img_display(lv_ui *ui);      // ★ 新增
void setup_scr_screen_wifi_set(lv_ui *ui);
void setup_scr_screen_time_set(lv_ui *ui);
void setup_scr_screen_weather(lv_ui *ui);

void setup_scr_screen_ota(lv_ui *ui);             // OTA主界面
void setup_scr_screen_ota_onenet(lv_ui *ui);       // OneNET平台升级
void setup_scr_screen_ota_local(lv_ui *ui);        // 本地升级
void setup_scr_screen_ota_switch(lv_ui *ui);       // 切换分区

void setup_scr_video_list(lv_ui *ui);              // 视频列表
void setup_scr_video_player(lv_ui *ui);            // 视频播放

// 视频播放全局变量（供events_init_video_list.c使用）
extern char g_video_filepath[512];
extern video_format_t g_video_format;

LV_IMG_DECLARE(_novel_alpha_30x30);
LV_IMG_DECLARE(_image_alpha_30x30);
LV_IMG_DECLARE(_she_zhi_alpha_30x30);
LV_IMG_DECLARE(_TXT_alpha_30x30);
LV_IMG_DECLARE(_TXT_alpha_30x30);
LV_IMG_DECLARE(_TXT_alpha_30x30);
LV_IMG_DECLARE(_clock_alpha_30x30);
LV_IMG_DECLARE(_WIFI_alpha_30x30);
LV_IMG_DECLARE(_OTA_alpha_30x30);
LV_IMG_DECLARE(game);

LV_FONT_DECLARE(lv_font_Acme_Regular_12)
LV_FONT_DECLARE(lv_font_Antonio_Regular_12)
LV_FONT_DECLARE(lv_font_montserratMedium_12)
LV_FONT_DECLARE(lv_font_Antonio_Regular_50)
LV_FONT_DECLARE(lv_font_montserratMedium_16)
LV_FONT_DECLARE(lv_font_Amiko_Regular_12)
LV_FONT_DECLARE(lv_font_montserratMedium_20)
LV_FONT_DECLARE(lv_font_montserratMedium_14)
LV_FONT_DECLARE(lv_font_montserratMedium_25)
LV_FONT_DECLARE(songti_font_16)
LV_FONT_DECLARE(lv_font_montserrat_48)
LV_FONT_DECLARE(zhao_hua)
LV_IMG_DECLARE(IMAGE_1)
LV_FONT_DECLARE(zhao_hua_16)


#ifdef __cplusplus
}
#endif
#endif
