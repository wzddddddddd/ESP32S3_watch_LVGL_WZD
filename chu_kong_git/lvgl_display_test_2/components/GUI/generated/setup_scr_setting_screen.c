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

void setup_scr_setting_screen(lv_ui *ui)
{
    //Write codes setting_screen
    ui->setting_screen = lv_obj_create(NULL);
    lv_obj_set_size(ui->setting_screen, 240, 284);
    lv_obj_set_scrollbar_mode(ui->setting_screen, LV_SCROLLBAR_MODE_OFF);

    //Write style for setting_screen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->setting_screen, 251, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->setting_screen, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->setting_screen, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes setting_screen_list_1
    ui->setting_screen_list_1 = lv_list_create(ui->setting_screen);
    ui->setting_screen_list_1_item0 = lv_list_add_btn(ui->setting_screen_list_1, &_clock_alpha_30x30, "时钟设置");
    ui->setting_screen_list_1_item1 = lv_list_add_btn(ui->setting_screen_list_1, &_WIFI_alpha_30x30, "  WIFI");
    ui->setting_screen_list_1_item2 = lv_list_add_btn(ui->setting_screen_list_1, &_OTA_alpha_30x30, " OTA升级");
    lv_obj_set_pos(ui->setting_screen_list_1, 0, 0);
    lv_obj_set_size(ui->setting_screen_list_1, 240, 284);
    lv_obj_set_scrollbar_mode(ui->setting_screen_list_1, LV_SCROLLBAR_MODE_OFF);

    //Write style state: LV_STATE_DEFAULT for &style_setting_screen_list_1_main_main_default
    static lv_style_t style_setting_screen_list_1_main_main_default;
    ui_init_style(&style_setting_screen_list_1_main_main_default);

    lv_style_set_pad_top(&style_setting_screen_list_1_main_main_default, 5);
    lv_style_set_pad_left(&style_setting_screen_list_1_main_main_default, 5);
    lv_style_set_pad_right(&style_setting_screen_list_1_main_main_default, 5);
    lv_style_set_pad_bottom(&style_setting_screen_list_1_main_main_default, 5);
    lv_style_set_bg_opa(&style_setting_screen_list_1_main_main_default, 255);
    lv_style_set_bg_color(&style_setting_screen_list_1_main_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_setting_screen_list_1_main_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&style_setting_screen_list_1_main_main_default, 1);
    lv_style_set_border_opa(&style_setting_screen_list_1_main_main_default, 255);
    lv_style_set_border_color(&style_setting_screen_list_1_main_main_default, lv_color_hex(0xe1e6ee));
    lv_style_set_border_side(&style_setting_screen_list_1_main_main_default, LV_BORDER_SIDE_FULL);
    lv_style_set_radius(&style_setting_screen_list_1_main_main_default, 3);
    lv_style_set_shadow_width(&style_setting_screen_list_1_main_main_default, 0);
    lv_obj_add_style(ui->setting_screen_list_1, &style_setting_screen_list_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_setting_screen_list_1_main_scrollbar_default
    static lv_style_t style_setting_screen_list_1_main_scrollbar_default;
    ui_init_style(&style_setting_screen_list_1_main_scrollbar_default);

    lv_style_set_radius(&style_setting_screen_list_1_main_scrollbar_default, 3);
    lv_style_set_bg_opa(&style_setting_screen_list_1_main_scrollbar_default, 255);
    lv_style_set_bg_color(&style_setting_screen_list_1_main_scrollbar_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_setting_screen_list_1_main_scrollbar_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->setting_screen_list_1, &style_setting_screen_list_1_main_scrollbar_default, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_setting_screen_list_1_extra_btns_main_default
    static lv_style_t style_setting_screen_list_1_extra_btns_main_default;
    ui_init_style(&style_setting_screen_list_1_extra_btns_main_default);

    lv_style_set_pad_top(&style_setting_screen_list_1_extra_btns_main_default, 5);
    lv_style_set_pad_left(&style_setting_screen_list_1_extra_btns_main_default, 5);
    lv_style_set_pad_right(&style_setting_screen_list_1_extra_btns_main_default, 5);
    lv_style_set_pad_bottom(&style_setting_screen_list_1_extra_btns_main_default, 5);
    lv_style_set_border_width(&style_setting_screen_list_1_extra_btns_main_default, 0);
    lv_style_set_text_color(&style_setting_screen_list_1_extra_btns_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_setting_screen_list_1_extra_btns_main_default, &songti_font_16);
    lv_style_set_text_opa(&style_setting_screen_list_1_extra_btns_main_default, 255);
    lv_style_set_radius(&style_setting_screen_list_1_extra_btns_main_default, 3);
    lv_style_set_bg_opa(&style_setting_screen_list_1_extra_btns_main_default, 255);
    lv_style_set_bg_color(&style_setting_screen_list_1_extra_btns_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_setting_screen_list_1_extra_btns_main_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->setting_screen_list_1_item1, &style_setting_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->setting_screen_list_1_item0, &style_setting_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->setting_screen_list_1_item2, &style_setting_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_setting_screen_list_1_extra_texts_main_default
    static lv_style_t style_setting_screen_list_1_extra_texts_main_default;
    ui_init_style(&style_setting_screen_list_1_extra_texts_main_default);

    lv_style_set_pad_top(&style_setting_screen_list_1_extra_texts_main_default, 5);
    lv_style_set_pad_left(&style_setting_screen_list_1_extra_texts_main_default, 5);
    lv_style_set_pad_right(&style_setting_screen_list_1_extra_texts_main_default, 5);
    lv_style_set_pad_bottom(&style_setting_screen_list_1_extra_texts_main_default, 5);
    lv_style_set_border_width(&style_setting_screen_list_1_extra_texts_main_default, 0);
    lv_style_set_text_color(&style_setting_screen_list_1_extra_texts_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_setting_screen_list_1_extra_texts_main_default, &songti_font_16);
    lv_style_set_text_opa(&style_setting_screen_list_1_extra_texts_main_default, 255);
    lv_style_set_radius(&style_setting_screen_list_1_extra_texts_main_default, 3);
    lv_style_set_transform_width(&style_setting_screen_list_1_extra_texts_main_default, 0);
    lv_style_set_bg_opa(&style_setting_screen_list_1_extra_texts_main_default, 255);
    lv_style_set_bg_color(&style_setting_screen_list_1_extra_texts_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_setting_screen_list_1_extra_texts_main_default, LV_GRAD_DIR_NONE);

    //The custom code of setting_screen.


    //Update current screen layout.
    lv_obj_update_layout(ui->setting_screen);

    //Init events for screen.
    events_init_setting_screen(ui);
}
