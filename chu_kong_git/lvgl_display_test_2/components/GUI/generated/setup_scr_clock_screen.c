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
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"
#include "arc_menu.h"



void setup_scr_clock_screen(lv_ui *ui)
{
    //Write codes clock_screen
    ui->clock_screen = lv_obj_create(NULL);
    lv_obj_set_size(ui->clock_screen, 240, 284);
    lv_obj_set_scrollbar_mode(ui->clock_screen, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->clock_screen, LV_OBJ_FLAG_SCROLLABLE);  // 禁止主屏幕滚动

    //Write style for clock_screen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->clock_screen, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes clock_screen_cont_2
    ui->clock_screen_cont_2 = lv_obj_create(ui->clock_screen);
    lv_obj_set_pos(ui->clock_screen_cont_2, 0, 0);
    lv_obj_set_size(ui->clock_screen_cont_2, 240, 284);
    lv_obj_set_scrollbar_mode(ui->clock_screen_cont_2, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ui->clock_screen_cont_2, LV_OBJ_FLAG_SCROLLABLE);  // 确保容器不可滑动

    //Write style for clock_screen_cont_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->clock_screen_cont_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->clock_screen_cont_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->clock_screen_cont_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->clock_screen_cont_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->clock_screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->clock_screen_cont_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_img_src(ui->clock_screen_cont_2, &IMAGE_1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->clock_screen_cont_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->clock_screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->clock_screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->clock_screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->clock_screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->clock_screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    // ========== 时间 Label（14:20）==========
    ui->clock_screen_label_time = lv_label_create(ui->clock_screen_cont_2);
    lv_label_set_text(ui->clock_screen_label_time, "14:20");
    lv_obj_set_pos(ui->clock_screen_label_time, 0, 50);
    lv_obj_set_size(ui->clock_screen_label_time, 240, 120);
    lv_obj_set_style_text_font(ui->clock_screen_label_time, &zhao_hua, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->clock_screen_label_time, lv_color_hex(0xF5F0E1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->clock_screen_label_time, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(ui->clock_screen_label_time, LV_LABEL_LONG_CLIP);  // 文字超出时裁剪，不滚动

    // ========== 日期 Label（2025年8月5日 周六）==========
    ui->clock_screen_label_date = lv_label_create(ui->clock_screen_cont_2);
    lv_label_set_text(ui->clock_screen_label_date, "2025年8月5日 周六");
    lv_obj_set_pos(ui->clock_screen_label_date, 0, 115);
    lv_obj_set_size(ui->clock_screen_label_date, 240, 25);
    lv_obj_set_style_text_font(ui->clock_screen_label_date, &zhao_hua_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->clock_screen_label_date, lv_color_hex(0xF5F0E1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->clock_screen_label_date, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(ui->clock_screen_label_date, LV_LABEL_LONG_CLIP);  // 文字超出时裁剪，不滚动

    // ========== 点击切换颜色事件 ==========
    lv_obj_add_event_cb(ui->clock_screen_label_time, clock_screen_color_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui->clock_screen_label_date, clock_screen_color_event_cb, LV_EVENT_ALL, NULL);

    // ========== 初始化弧形壁纸菜单 ==========
    arc_menu_init(ui->clock_screen);

    //The custom code of clock_screen.



    //Update current screen layout.
    lv_obj_update_layout(ui->clock_screen);

    //Init events for screen.
    events_init_clock_screen(ui);
}
