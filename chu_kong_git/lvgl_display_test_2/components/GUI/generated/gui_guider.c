/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "widgets_init.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "gg_external_data.h"
#endif

void ui_init_style(lv_style_t * style)
{
    if (style->prop_cnt > 1)
        lv_style_reset(style);
    else
        lv_style_init(style);
}

void ui_load_scr_animation(lv_ui *ui, lv_obj_t ** new_scr, bool new_scr_del, bool * old_scr_del, ui_setup_scr_t setup_scr,
                           lv_scr_load_anim_t anim_type, uint32_t time, uint32_t delay, bool is_clean, bool auto_del)
{
    static uint32_t s_last_switch_tick = 0;
    uint32_t now_tick = lv_tick_get();

    if ((now_tick - s_last_switch_tick) < 120U) {
        return;
    }

    if (ui == NULL || new_scr == NULL || old_scr_del == NULL || setup_scr == NULL) {
        return;
    }

    lv_obj_t *act_scr = lv_scr_act();
    if (act_scr == NULL || !lv_obj_is_valid(act_scr)) {
        return;
    }

#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
    if(auto_del) {
        gg_edata_task_clear(act_scr);
    }
#endif
    if (auto_del && is_clean) {
        lv_obj_clean(act_scr);
    }

    if (new_scr_del || *new_scr == NULL || !lv_obj_is_valid(*new_scr)) {
        setup_scr(ui);
    }
    if (*new_scr == NULL || !lv_obj_is_valid(*new_scr)) {
        return;
    }

    if (*new_scr == act_scr) {
        *old_scr_del = false;
        return;
    }

    lv_scr_load_anim(*new_scr, anim_type, time, delay, auto_del);
    *old_scr_del = auto_del;
    s_last_switch_tick = now_tick;
}

void ui_animation(void * var, int32_t duration, int32_t delay, int32_t start_value, int32_t end_value, lv_anim_path_cb_t path_cb,
                  uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                  lv_anim_exec_xcb_t exec_cb, lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb)
{
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, var);
    lv_anim_set_exec_cb(&anim, exec_cb);
    lv_anim_set_values(&anim, start_value, end_value);
    lv_anim_set_time(&anim, duration);
    lv_anim_set_delay(&anim, delay);
    lv_anim_set_path_cb(&anim, path_cb);
    lv_anim_set_repeat_count(&anim, repeat_cnt);
    lv_anim_set_repeat_delay(&anim, repeat_delay);
    lv_anim_set_playback_time(&anim, playback_time);
    lv_anim_set_playback_delay(&anim, playback_delay);
    if (start_cb) {
        lv_anim_set_start_cb(&anim, start_cb);
    }
    if (ready_cb) {
        lv_anim_set_ready_cb(&anim, ready_cb);
    }
    if (deleted_cb) {
        lv_anim_set_deleted_cb(&anim, deleted_cb);
    }
    lv_anim_start(&anim);
}

void init_scr_del_flag(lv_ui *ui)
{

    ui->clock_screen_del = true;
    ui->menu_screen_del = true;
    ui->novel_display_del = true;
    ui->novel_list_del = true;
    ui->setting_screen_del = true;
    ui->screen_game_del = true;

    ui->screen_img_list_del = true;       // �?新增
    ui->screen_img_display_del = true;    // �?新增
    ui->screen_wifi_set_del = true;
    ui->screen_time_set_del = true;

    ui->screen_ota_del = true;           // OTA主界�?    ui->screen_ota_onenet_del = true;    // OneNET升级
    ui->screen_ota_local_del = true;     // 本地升级
    ui->screen_ota_switch_del = true;    // 切换分区

    ui->video_list_del = true;           // 视频列表
    ui->video_player_del = true;          // 视频播放

}

void setup_ui(lv_ui *ui)
{
    init_scr_del_flag(ui);
    init_keyboard(ui);
    setup_scr_clock_screen(ui);
    ui->clock_screen_del = false;
    lv_scr_load(ui->clock_screen);
}

void init_keyboard(lv_ui *ui)
{

}

