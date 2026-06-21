/*
 * setup_scr_screen_ota.c
 * OTA主界面 —— 3个升级选项
 */

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"

void setup_scr_screen_ota(lv_ui *ui)
{
    // 创建界面
    ui->screen_ota = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_ota, 240, 284);
    lv_obj_set_scrollbar_mode(ui->screen_ota, LV_SCROLLBAR_MODE_OFF);

    // 背景样式
    lv_obj_set_style_bg_opa(ui->screen_ota, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_ota, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);

    // 标题
    lv_obj_t *title = lv_label_create(ui->screen_ota);
    lv_label_set_text(title, "OTA升级");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(title, &songti_font_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // 创建列表
    ui->screen_ota_list_1 = lv_list_create(ui->screen_ota);
    lv_obj_set_pos(ui->screen_ota_list_1, 0, 40);
    lv_obj_set_size(ui->screen_ota_list_1, 240, 244);
    lv_obj_set_scrollbar_mode(ui->screen_ota_list_1, LV_SCROLLBAR_MODE_OFF);

    // 列表背景样式
    static lv_style_t style_list_main;
    ui_init_style(&style_list_main);
    lv_style_set_pad_top(&style_list_main, 5);
    lv_style_set_pad_left(&style_list_main, 5);
    lv_style_set_pad_right(&style_list_main, 5);
    lv_style_set_pad_bottom(&style_list_main, 5);
    lv_style_set_bg_opa(&style_list_main, 255);
    lv_style_set_bg_color(&style_list_main, lv_color_hex(0xffffff));
    lv_style_set_border_width(&style_list_main, 0);
    lv_style_set_radius(&style_list_main, 3);
    lv_obj_add_style(ui->screen_ota_list_1, &style_list_main, LV_PART_MAIN|LV_STATE_DEFAULT);

    // 添加3个列表项（无图标）
    ui->screen_ota_list_1_item0 = lv_list_add_btn(ui->screen_ota_list_1, NULL, "OneNET平台升级");
    ui->screen_ota_list_1_item1 = lv_list_add_btn(ui->screen_ota_list_1, NULL, "本地升级");
    ui->screen_ota_list_1_item2 = lv_list_add_btn(ui->screen_ota_list_1, NULL, "切换分区");

    // 列表按钮样式
    static lv_style_t style_list_btn;
    ui_init_style(&style_list_btn);
    lv_style_set_pad_top(&style_list_btn, 8);
    lv_style_set_pad_left(&style_list_btn, 10);
    lv_style_set_pad_right(&style_list_btn, 10);
    lv_style_set_pad_bottom(&style_list_btn, 8);
    lv_style_set_border_width(&style_list_btn, 0);
    lv_style_set_text_color(&style_list_btn, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_list_btn, &songti_font_16);
    lv_style_set_text_opa(&style_list_btn, 255);
    lv_style_set_radius(&style_list_btn, 3);
    lv_style_set_bg_opa(&style_list_btn, 255);
    lv_style_set_bg_color(&style_list_btn, lv_color_hex(0xffffff));
    lv_obj_add_style(ui->screen_ota_list_1_item0, &style_list_btn, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_ota_list_1_item1, &style_list_btn, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->screen_ota_list_1_item2, &style_list_btn, LV_PART_MAIN|LV_STATE_DEFAULT);

    lv_obj_update_layout(ui->screen_ota);

    // 事件初始化
    events_init_screen_ota(ui);
}