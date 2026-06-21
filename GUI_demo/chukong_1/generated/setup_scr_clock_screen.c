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



void setup_scr_clock_screen(lv_ui *ui)
{
    //Write codes clock_screen
    ui->clock_screen = lv_obj_create(NULL);
    lv_obj_set_size(ui->clock_screen, 240, 284);
    lv_obj_set_scrollbar_mode(ui->clock_screen, LV_SCROLLBAR_MODE_OFF);

    //Write style for clock_screen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->clock_screen, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes clock_screen_cont_2
    ui->clock_screen_cont_2 = lv_obj_create(ui->clock_screen);
    lv_obj_set_pos(ui->clock_screen_cont_2, 0, 0);
    lv_obj_set_size(ui->clock_screen_cont_2, 240, 284);
    lv_obj_set_scrollbar_mode(ui->clock_screen_cont_2, LV_SCROLLBAR_MODE_OFF);

    //Write style for clock_screen_cont_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->clock_screen_cont_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->clock_screen_cont_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->clock_screen_cont_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->clock_screen_cont_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->clock_screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->clock_screen_cont_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->clock_screen_cont_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->clock_screen_cont_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->clock_screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->clock_screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->clock_screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->clock_screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->clock_screen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes clock_screen_spangroup_2
    ui->clock_screen_spangroup_2 = lv_spangroup_create(ui->clock_screen_cont_2);
    lv_spangroup_set_align(ui->clock_screen_spangroup_2, LV_TEXT_ALIGN_LEFT);
    lv_spangroup_set_overflow(ui->clock_screen_spangroup_2, LV_SPAN_OVERFLOW_CLIP);
    lv_spangroup_set_mode(ui->clock_screen_spangroup_2, LV_SPAN_MODE_BREAK);
    //create span
    ui->clock_screen_spangroup_2_span = lv_spangroup_new_span(ui->clock_screen_spangroup_2);
    lv_span_set_text(ui->clock_screen_spangroup_2_span, "2025");
    lv_style_set_text_color(&ui->clock_screen_spangroup_2_span->style, lv_color_hex(0xfdfdfd));
    lv_style_set_text_decor(&ui->clock_screen_spangroup_2_span->style, LV_TEXT_DECOR_NONE);
    lv_style_set_text_font(&ui->clock_screen_spangroup_2_span->style, &lv_font_Acme_Regular_12);
    ui->clock_screen_spangroup_2_span = lv_spangroup_new_span(ui->clock_screen_spangroup_2);
    lv_span_set_text(ui->clock_screen_spangroup_2_span, " : ");
    lv_style_set_text_color(&ui->clock_screen_spangroup_2_span->style, lv_color_hex(0xf6f2f2));
    lv_style_set_text_decor(&ui->clock_screen_spangroup_2_span->style, LV_TEXT_DECOR_NONE);
    lv_style_set_text_font(&ui->clock_screen_spangroup_2_span->style, &lv_font_Antonio_Regular_12);
    ui->clock_screen_spangroup_2_span = lv_spangroup_new_span(ui->clock_screen_spangroup_2);
    lv_span_set_text(ui->clock_screen_spangroup_2_span, "3");
    lv_style_set_text_color(&ui->clock_screen_spangroup_2_span->style, lv_color_hex(0xfafafa));
    lv_style_set_text_decor(&ui->clock_screen_spangroup_2_span->style, LV_TEXT_DECOR_NONE);
    lv_style_set_text_font(&ui->clock_screen_spangroup_2_span->style, &lv_font_Acme_Regular_12);
    ui->clock_screen_spangroup_2_span = lv_spangroup_new_span(ui->clock_screen_spangroup_2);
    lv_span_set_text(ui->clock_screen_spangroup_2_span, " : ");
    lv_style_set_text_color(&ui->clock_screen_spangroup_2_span->style, lv_color_hex(0xfdfdfd));
    lv_style_set_text_decor(&ui->clock_screen_spangroup_2_span->style, LV_TEXT_DECOR_NONE);
    lv_style_set_text_font(&ui->clock_screen_spangroup_2_span->style, &lv_font_montserratMedium_12);
    ui->clock_screen_spangroup_2_span = lv_spangroup_new_span(ui->clock_screen_spangroup_2);
    lv_span_set_text(ui->clock_screen_spangroup_2_span, "6");
    lv_style_set_text_color(&ui->clock_screen_spangroup_2_span->style, lv_color_hex(0xf9f4f4));
    lv_style_set_text_decor(&ui->clock_screen_spangroup_2_span->style, LV_TEXT_DECOR_NONE);
    lv_style_set_text_font(&ui->clock_screen_spangroup_2_span->style, &lv_font_Acme_Regular_12);
    lv_obj_set_pos(ui->clock_screen_spangroup_2, 76, 97);
    lv_obj_set_size(ui->clock_screen_spangroup_2, 118, 17);

    //Write style state: LV_STATE_DEFAULT for &style_clock_screen_spangroup_2_main_main_default
    static lv_style_t style_clock_screen_spangroup_2_main_main_default;
    ui_init_style(&style_clock_screen_spangroup_2_main_main_default);

    lv_style_set_border_width(&style_clock_screen_spangroup_2_main_main_default, 0);
    lv_style_set_radius(&style_clock_screen_spangroup_2_main_main_default, 0);
    lv_style_set_bg_opa(&style_clock_screen_spangroup_2_main_main_default, 0);
    lv_style_set_pad_top(&style_clock_screen_spangroup_2_main_main_default, 0);
    lv_style_set_pad_right(&style_clock_screen_spangroup_2_main_main_default, 0);
    lv_style_set_pad_bottom(&style_clock_screen_spangroup_2_main_main_default, 0);
    lv_style_set_pad_left(&style_clock_screen_spangroup_2_main_main_default, 0);
    lv_style_set_shadow_width(&style_clock_screen_spangroup_2_main_main_default, 0);
    lv_obj_add_style(ui->clock_screen_spangroup_2, &style_clock_screen_spangroup_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_spangroup_refr_mode(ui->clock_screen_spangroup_2);

    //Write codes clock_screen_spangroup_1
    ui->clock_screen_spangroup_1 = lv_spangroup_create(ui->clock_screen_cont_2);
    lv_spangroup_set_align(ui->clock_screen_spangroup_1, LV_TEXT_ALIGN_LEFT);
    lv_spangroup_set_overflow(ui->clock_screen_spangroup_1, LV_SPAN_OVERFLOW_CLIP);
    lv_spangroup_set_mode(ui->clock_screen_spangroup_1, LV_SPAN_MODE_BREAK);
    //create span
    ui->clock_screen_spangroup_1_span = lv_spangroup_new_span(ui->clock_screen_spangroup_1);
    lv_span_set_text(ui->clock_screen_spangroup_1_span, "13");
    lv_style_set_text_color(&ui->clock_screen_spangroup_1_span->style, lv_color_hex(0x20db5e));
    lv_style_set_text_decor(&ui->clock_screen_spangroup_1_span->style, LV_TEXT_DECOR_NONE);
    lv_style_set_text_font(&ui->clock_screen_spangroup_1_span->style, &lv_font_Antonio_Regular_50);
    ui->clock_screen_spangroup_1_span = lv_spangroup_new_span(ui->clock_screen_spangroup_1);
    lv_span_set_text(ui->clock_screen_spangroup_1_span, ":");
    lv_style_set_text_color(&ui->clock_screen_spangroup_1_span->style, lv_color_hex(0xf5f5f5));
    lv_style_set_text_decor(&ui->clock_screen_spangroup_1_span->style, LV_TEXT_DECOR_NONE);
    lv_style_set_text_font(&ui->clock_screen_spangroup_1_span->style, &lv_font_Antonio_Regular_50);
    ui->clock_screen_spangroup_1_span = lv_spangroup_new_span(ui->clock_screen_spangroup_1);
    lv_span_set_text(ui->clock_screen_spangroup_1_span, "25");
    lv_style_set_text_color(&ui->clock_screen_spangroup_1_span->style, lv_color_hex(0x27319a));
    lv_style_set_text_decor(&ui->clock_screen_spangroup_1_span->style, LV_TEXT_DECOR_NONE);
    lv_style_set_text_font(&ui->clock_screen_spangroup_1_span->style, &lv_font_Antonio_Regular_50);
    lv_obj_set_pos(ui->clock_screen_spangroup_1, 63, 116);
    lv_obj_set_size(ui->clock_screen_spangroup_1, 117, 47);

    //Write style state: LV_STATE_DEFAULT for &style_clock_screen_spangroup_1_main_main_default
    static lv_style_t style_clock_screen_spangroup_1_main_main_default;
    ui_init_style(&style_clock_screen_spangroup_1_main_main_default);

    lv_style_set_border_width(&style_clock_screen_spangroup_1_main_main_default, 0);
    lv_style_set_radius(&style_clock_screen_spangroup_1_main_main_default, 0);
    lv_style_set_bg_opa(&style_clock_screen_spangroup_1_main_main_default, 0);
    lv_style_set_pad_top(&style_clock_screen_spangroup_1_main_main_default, 0);
    lv_style_set_pad_right(&style_clock_screen_spangroup_1_main_main_default, 0);
    lv_style_set_pad_bottom(&style_clock_screen_spangroup_1_main_main_default, 0);
    lv_style_set_pad_left(&style_clock_screen_spangroup_1_main_main_default, 0);
    lv_style_set_shadow_width(&style_clock_screen_spangroup_1_main_main_default, 0);
    lv_obj_add_style(ui->clock_screen_spangroup_1, &style_clock_screen_spangroup_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_spangroup_refr_mode(ui->clock_screen_spangroup_1);

    //The custom code of clock_screen.



    //Update current screen layout.
    lv_obj_update_layout(ui->clock_screen);

    //Init events for screen.
    events_init_clock_screen(ui);
}
