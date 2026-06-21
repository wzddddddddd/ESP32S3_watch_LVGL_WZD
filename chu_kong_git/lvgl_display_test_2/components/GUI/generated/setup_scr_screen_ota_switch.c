/*
 * setup_scr_screen_ota_switch.c
 * 切换分区界面
 */

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"

void setup_scr_screen_ota_switch(lv_ui *ui)
{
    // 创建界面
    ui->screen_ota_switch = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_ota_switch, 240, 284);
    lv_obj_set_scrollbar_mode(ui->screen_ota_switch, LV_SCROLLBAR_MODE_OFF);

    // 背景样式
    lv_obj_set_style_bg_opa(ui->screen_ota_switch, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_ota_switch, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);

    // 标题
    lv_obj_t *title = lv_label_create(ui->screen_ota_switch);
    lv_label_set_text(title, "切换分区");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(title, &songti_font_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // 当前分区标签
    lv_obj_t *label_curr_title = lv_label_create(ui->screen_ota_switch);
    lv_label_set_text(label_curr_title, "当前分区:");
    lv_obj_set_style_text_color(label_curr_title, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(label_curr_title, &songti_font_16, 0);
    lv_obj_align(label_curr_title, LV_ALIGN_TOP_LEFT, 15, 50);

    ui->screen_ota_switch_label_curr_part = lv_label_create(ui->screen_ota_switch);
    lv_label_set_text(ui->screen_ota_switch_label_curr_part, "app_ota_0");
    lv_obj_set_style_text_color(ui->screen_ota_switch_label_curr_part, lv_color_hex(0x00ff00), 0);
    lv_obj_set_style_text_font(ui->screen_ota_switch_label_curr_part, &songti_font_16, 0);
    lv_obj_align(ui->screen_ota_switch_label_curr_part, LV_ALIGN_TOP_LEFT, 100, 50);

    // 备用分区标签
    lv_obj_t *label_standby_title = lv_label_create(ui->screen_ota_switch);
    lv_label_set_text(label_standby_title, "备用分区:");
    lv_obj_set_style_text_color(label_standby_title, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(label_standby_title, &songti_font_16, 0);
    lv_obj_align(label_standby_title, LV_ALIGN_TOP_LEFT, 15, 80);

    ui->screen_ota_switch_label_standby_part = lv_label_create(ui->screen_ota_switch);
    lv_label_set_text(ui->screen_ota_switch_label_standby_part, "app_ota_1");
    lv_obj_set_style_text_color(ui->screen_ota_switch_label_standby_part, lv_color_hex(0xffff00), 0);
    lv_obj_set_style_text_font(ui->screen_ota_switch_label_standby_part, &songti_font_16, 0);
    lv_obj_align(ui->screen_ota_switch_label_standby_part, LV_ALIGN_TOP_LEFT, 100, 80);

    // 当前版本标签
    lv_obj_t *label_ver_title = lv_label_create(ui->screen_ota_switch);
    lv_label_set_text(label_ver_title, "当前版本:");
    lv_obj_set_style_text_color(label_ver_title, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(label_ver_title, &songti_font_16, 0);
    lv_obj_align(label_ver_title, LV_ALIGN_TOP_LEFT, 15, 110);

    ui->screen_ota_switch_label_version = lv_label_create(ui->screen_ota_switch);
    lv_label_set_text(ui->screen_ota_switch_label_version, "V1.0.0");
    lv_obj_set_style_text_color(ui->screen_ota_switch_label_version, lv_color_hex(0x00ff00), 0);
    lv_obj_set_style_text_font(ui->screen_ota_switch_label_version, &songti_font_16, 0);
    lv_obj_align(ui->screen_ota_switch_label_version, LV_ALIGN_TOP_LEFT, 100, 110);

    // 切换按钮
    ui->screen_ota_switch_btn_switch = lv_btn_create(ui->screen_ota_switch);
    lv_obj_set_size(ui->screen_ota_switch_btn_switch, 200, 40);
    lv_obj_align(ui->screen_ota_switch_btn_switch, LV_ALIGN_TOP_MID, 0, 150);
    lv_obj_set_style_bg_color(ui->screen_ota_switch_btn_switch, lv_color_hex(0xFF9800), 0);
    lv_obj_set_style_bg_color(ui->screen_ota_switch_btn_switch, lv_color_hex(0xE65100), LV_STATE_PRESSED);
    lv_obj_set_style_radius(ui->screen_ota_switch_btn_switch, 5, 0);

    lv_obj_t *btn_lbl = lv_label_create(ui->screen_ota_switch_btn_switch);
    lv_label_set_text(btn_lbl, "切换并重启");
    lv_obj_set_style_text_color(btn_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(btn_lbl, &songti_font_16, 0);
    lv_obj_center(btn_lbl);

    // 提示标签
    lv_obj_t *label_tip = lv_label_create(ui->screen_ota_switch);
    lv_label_set_text(label_tip, "新固件需通过\"本地升级\"\n提前写入备用分区");
    lv_obj_set_style_text_color(label_tip, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(label_tip, &songti_font_16, 0);
    lv_obj_set_style_text_align(label_tip, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label_tip, LV_ALIGN_TOP_MID, 0, 205);
    lv_obj_set_width(label_tip, 220);

    lv_obj_update_layout(ui->screen_ota_switch);

    // 事件初始化
    events_init_screen_ota_switch(ui);
}
