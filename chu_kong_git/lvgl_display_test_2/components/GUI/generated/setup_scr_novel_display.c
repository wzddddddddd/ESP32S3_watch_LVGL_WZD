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
#include "lvgl_display.h"

void setup_scr_novel_display(lv_ui *ui)
{
    //Write codes novel_display
    ui->novel_display = lv_obj_create(NULL);
    lv_obj_set_size(ui->novel_display, 240, 284);
    lv_obj_set_scrollbar_mode(ui->novel_display, LV_SCROLLBAR_MODE_OFF);

    //Write style for novel_display, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->novel_display, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->novel_display, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->novel_display, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes novel_display_label_1
    ui->novel_display_label_1 = lv_label_create(ui->novel_display);
    lv_label_set_text(ui->novel_display_label_1, "Loading...");
    lv_label_set_long_mode(ui->novel_display_label_1, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->novel_display_label_1, 0, 0);
    lv_obj_set_size(ui->novel_display_label_1, 240, 265);
    lv_obj_add_flag(ui->novel_display_label_1, LV_OBJ_FLAG_SCROLLABLE);

    //Write style for novel_display_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->novel_display_label_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->novel_display_label_1, &songti_font_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->novel_display_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->novel_display_label_1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes novel_display_spangroup_1
    ui->novel_display_spangroup_1 = lv_spangroup_create(ui->novel_display);
    lv_spangroup_set_align(ui->novel_display_spangroup_1, LV_TEXT_ALIGN_LEFT);
    lv_spangroup_set_overflow(ui->novel_display_spangroup_1, LV_SPAN_OVERFLOW_CLIP);
    lv_spangroup_set_mode(ui->novel_display_spangroup_1, LV_SPAN_MODE_BREAK);
    //create span
    ui->novel_display_spangroup_1_span = lv_spangroup_new_span(ui->novel_display_spangroup_1);
    lv_span_set_text(ui->novel_display_spangroup_1_span, "19");
    lv_style_set_text_color(&ui->novel_display_spangroup_1_span->style, lv_color_hex(0xffffff));
    lv_style_set_text_decor(&ui->novel_display_spangroup_1_span->style, LV_TEXT_DECOR_NONE);
    lv_style_set_text_font(&ui->novel_display_spangroup_1_span->style, &lv_font_Amiko_Regular_12);
    ui->novel_display_spangroup_1_span = lv_spangroup_new_span(ui->novel_display_spangroup_1);
    lv_span_set_text(ui->novel_display_spangroup_1_span, ":");
    lv_style_set_text_color(&ui->novel_display_spangroup_1_span->style, lv_color_hex(0xfdfdfd));
    lv_style_set_text_decor(&ui->novel_display_spangroup_1_span->style, LV_TEXT_DECOR_NONE);
    lv_style_set_text_font(&ui->novel_display_spangroup_1_span->style, &songti_font_16);
    ui->novel_display_spangroup_1_span = lv_spangroup_new_span(ui->novel_display_spangroup_1);
    lv_span_set_text(ui->novel_display_spangroup_1_span, "45");
    lv_style_set_text_color(&ui->novel_display_spangroup_1_span->style, lv_color_hex(0xffffff));
    lv_style_set_text_decor(&ui->novel_display_spangroup_1_span->style, LV_TEXT_DECOR_NONE);
    lv_style_set_text_font(&ui->novel_display_spangroup_1_span->style, &lv_font_Amiko_Regular_12);
    lv_obj_set_pos(ui->novel_display_spangroup_1, 198, 269);
    lv_obj_set_size(ui->novel_display_spangroup_1, 39, 8);

    //Write style state: LV_STATE_DEFAULT for &style_novel_display_spangroup_1_main_main_default
    static lv_style_t style_novel_display_spangroup_1_main_main_default;
    ui_init_style(&style_novel_display_spangroup_1_main_main_default);

    lv_style_set_border_width(&style_novel_display_spangroup_1_main_main_default, 0);
    lv_style_set_radius(&style_novel_display_spangroup_1_main_main_default, 0);
    lv_style_set_bg_opa(&style_novel_display_spangroup_1_main_main_default, 0);
    lv_style_set_pad_top(&style_novel_display_spangroup_1_main_main_default, 0);
    lv_style_set_pad_right(&style_novel_display_spangroup_1_main_main_default, 0);
    lv_style_set_pad_bottom(&style_novel_display_spangroup_1_main_main_default, 0);
    lv_style_set_pad_left(&style_novel_display_spangroup_1_main_main_default, 0);
    lv_style_set_shadow_width(&style_novel_display_spangroup_1_main_main_default, 0);
    lv_obj_add_style(ui->novel_display_spangroup_1, &style_novel_display_spangroup_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_spangroup_refr_mode(ui->novel_display_spangroup_1);

    //The custom code of novel_display.


    //Update current screen layout.
    lv_obj_update_layout(ui->novel_display);

    //Init events for screen.
    events_init_novel_display(ui);

    lvgl_msg_send_nonblocking(LVGL_MSG_NOVEL_PAGE_SYNC_REQ, 0, NULL);
}