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



void setup_scr_menu_screen(lv_ui *ui)
{
    //Write codes menu_screen
    ui->menu_screen = lv_obj_create(NULL);
    lv_obj_set_size(ui->menu_screen, 240, 284);
    lv_obj_set_scrollbar_mode(ui->menu_screen, LV_SCROLLBAR_MODE_OFF);

    //Write style for menu_screen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->menu_screen, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->menu_screen, lv_color_hex(0xfafafa), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->menu_screen, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes menu_screen_list_1
    ui->menu_screen_list_1 = lv_list_create(ui->menu_screen);
    ui->menu_screen_list_1_item0 = lv_list_add_btn(ui->menu_screen_list_1, &_novel_alpha_30x30, "小说");
    ui->menu_screen_list_1_item1 = lv_list_add_btn(ui->menu_screen_list_1, &_image_alpha_30x30, "图片");
    ui->menu_screen_list_1_item2 = lv_list_add_btn(ui->menu_screen_list_1, &_she_zhi_alpha_30x30, "设置");
    lv_obj_set_pos(ui->menu_screen_list_1, 0, 0);
    lv_obj_set_size(ui->menu_screen_list_1, 240, 284);
    lv_obj_set_scrollbar_mode(ui->menu_screen_list_1, LV_SCROLLBAR_MODE_OFF);

    //Write style state: LV_STATE_DEFAULT for &style_menu_screen_list_1_main_main_default
    static lv_style_t style_menu_screen_list_1_main_main_default;
    ui_init_style(&style_menu_screen_list_1_main_main_default);

    lv_style_set_pad_top(&style_menu_screen_list_1_main_main_default, 5);
    lv_style_set_pad_left(&style_menu_screen_list_1_main_main_default, 5);
    lv_style_set_pad_right(&style_menu_screen_list_1_main_main_default, 5);
    lv_style_set_pad_bottom(&style_menu_screen_list_1_main_main_default, 5);
    lv_style_set_bg_opa(&style_menu_screen_list_1_main_main_default, 255);
    lv_style_set_bg_color(&style_menu_screen_list_1_main_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_menu_screen_list_1_main_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&style_menu_screen_list_1_main_main_default, 4);
    lv_style_set_border_opa(&style_menu_screen_list_1_main_main_default, 255);
    lv_style_set_border_color(&style_menu_screen_list_1_main_main_default, lv_color_hex(0xfafafa));
    lv_style_set_border_side(&style_menu_screen_list_1_main_main_default, LV_BORDER_SIDE_FULL);
    lv_style_set_radius(&style_menu_screen_list_1_main_main_default, 3);
    lv_style_set_shadow_width(&style_menu_screen_list_1_main_main_default, 0);
    lv_obj_add_style(ui->menu_screen_list_1, &style_menu_screen_list_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_menu_screen_list_1_main_scrollbar_default
    static lv_style_t style_menu_screen_list_1_main_scrollbar_default;
    ui_init_style(&style_menu_screen_list_1_main_scrollbar_default);

    lv_style_set_radius(&style_menu_screen_list_1_main_scrollbar_default, 3);
    lv_style_set_bg_opa(&style_menu_screen_list_1_main_scrollbar_default, 255);
    lv_style_set_bg_color(&style_menu_screen_list_1_main_scrollbar_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_menu_screen_list_1_main_scrollbar_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->menu_screen_list_1, &style_menu_screen_list_1_main_scrollbar_default, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_menu_screen_list_1_extra_btns_main_default
    static lv_style_t style_menu_screen_list_1_extra_btns_main_default;
    ui_init_style(&style_menu_screen_list_1_extra_btns_main_default);

    lv_style_set_pad_top(&style_menu_screen_list_1_extra_btns_main_default, 5);
    lv_style_set_pad_left(&style_menu_screen_list_1_extra_btns_main_default, 5);
    lv_style_set_pad_right(&style_menu_screen_list_1_extra_btns_main_default, 5);
    lv_style_set_pad_bottom(&style_menu_screen_list_1_extra_btns_main_default, 5);
    lv_style_set_border_width(&style_menu_screen_list_1_extra_btns_main_default, 0);
    lv_style_set_text_color(&style_menu_screen_list_1_extra_btns_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_menu_screen_list_1_extra_btns_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_menu_screen_list_1_extra_btns_main_default, 255);
    lv_style_set_radius(&style_menu_screen_list_1_extra_btns_main_default, 3);
    lv_style_set_bg_opa(&style_menu_screen_list_1_extra_btns_main_default, 255);
    lv_style_set_bg_color(&style_menu_screen_list_1_extra_btns_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_menu_screen_list_1_extra_btns_main_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->menu_screen_list_1_item2, &style_menu_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->menu_screen_list_1_item1, &style_menu_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->menu_screen_list_1_item0, &style_menu_screen_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_menu_screen_list_1_extra_texts_main_default
    static lv_style_t style_menu_screen_list_1_extra_texts_main_default;
    ui_init_style(&style_menu_screen_list_1_extra_texts_main_default);

    lv_style_set_pad_top(&style_menu_screen_list_1_extra_texts_main_default, 5);
    lv_style_set_pad_left(&style_menu_screen_list_1_extra_texts_main_default, 5);
    lv_style_set_pad_right(&style_menu_screen_list_1_extra_texts_main_default, 5);
    lv_style_set_pad_bottom(&style_menu_screen_list_1_extra_texts_main_default, 5);
    lv_style_set_border_width(&style_menu_screen_list_1_extra_texts_main_default, 0);
    lv_style_set_text_color(&style_menu_screen_list_1_extra_texts_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_menu_screen_list_1_extra_texts_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_menu_screen_list_1_extra_texts_main_default, 255);
    lv_style_set_radius(&style_menu_screen_list_1_extra_texts_main_default, 3);
    lv_style_set_transform_width(&style_menu_screen_list_1_extra_texts_main_default, 0);
    lv_style_set_bg_opa(&style_menu_screen_list_1_extra_texts_main_default, 255);
    lv_style_set_bg_color(&style_menu_screen_list_1_extra_texts_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_menu_screen_list_1_extra_texts_main_default, LV_GRAD_DIR_NONE);

    //The custom code of menu_screen.


    //Update current screen layout.
    lv_obj_update_layout(ui->menu_screen);

    //Init events for screen.
    events_init_menu_screen(ui);
}
