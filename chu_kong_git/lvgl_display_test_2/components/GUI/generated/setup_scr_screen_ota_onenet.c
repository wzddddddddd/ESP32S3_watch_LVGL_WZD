/*
 * setup_scr_screen_ota_onenet.c
 * OneNET平台升级界面
 */

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"

void setup_scr_screen_ota_onenet(lv_ui *ui)
{
    // 创建界面
    ui->screen_ota_onenet = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_ota_onenet, 240, 284);
    lv_obj_set_scrollbar_mode(ui->screen_ota_onenet, LV_SCROLLBAR_MODE_OFF);

    // 背景样式
    lv_obj_set_style_bg_opa(ui->screen_ota_onenet, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_ota_onenet, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);

    // 标题
    lv_obj_t *title = lv_label_create(ui->screen_ota_onenet);
    lv_label_set_text(title, "OneNET平台升级");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(title, &songti_font_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // 版本标签
    lv_obj_t *label_ver_title = lv_label_create(ui->screen_ota_onenet);
    lv_label_set_text(label_ver_title, "当前版本:");
    lv_obj_set_style_text_color(label_ver_title, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(label_ver_title, &songti_font_16, 0);
    lv_obj_align(label_ver_title, LV_ALIGN_TOP_LEFT, 15, 50);

    ui->screen_ota_onenet_label_version = lv_label_create(ui->screen_ota_onenet);
    lv_label_set_text(ui->screen_ota_onenet_label_version, "V1.0.0");
    lv_obj_set_style_text_color(ui->screen_ota_onenet_label_version, lv_color_hex(0x00ff00), 0);
    lv_obj_set_style_text_font(ui->screen_ota_onenet_label_version, &songti_font_16, 0);
    lv_obj_align(ui->screen_ota_onenet_label_version, LV_ALIGN_TOP_LEFT, 95, 50);

    // 状态标签
    lv_obj_t *label_status_title = lv_label_create(ui->screen_ota_onenet);
    lv_label_set_text(label_status_title, "状    态:");
    lv_obj_set_style_text_color(label_status_title, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(label_status_title, &songti_font_16, 0);
    lv_obj_align(label_status_title, LV_ALIGN_TOP_LEFT, 15, 85);

    ui->screen_ota_onenet_label_status = lv_label_create(ui->screen_ota_onenet);
    lv_label_set_text(ui->screen_ota_onenet_label_status, "等待操作");
    lv_obj_set_style_text_color(ui->screen_ota_onenet_label_status, lv_color_hex(0x00ff00), 0);
    lv_obj_set_style_text_font(ui->screen_ota_onenet_label_status, &songti_font_16, 0);
    lv_obj_align(ui->screen_ota_onenet_label_status, LV_ALIGN_TOP_LEFT, 95, 85);

    // 开始升级按钮
    ui->screen_ota_onenet_btn_start = lv_btn_create(ui->screen_ota_onenet);
    lv_obj_set_size(ui->screen_ota_onenet_btn_start, 200, 40);
    lv_obj_align(ui->screen_ota_onenet_btn_start, LV_ALIGN_TOP_MID, 0, 130);
    lv_obj_set_style_bg_color(ui->screen_ota_onenet_btn_start, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_bg_color(ui->screen_ota_onenet_btn_start, lv_color_hex(0x1565C0), LV_STATE_PRESSED);
    lv_obj_set_style_radius(ui->screen_ota_onenet_btn_start, 5, 0);

    lv_obj_t *btn_lbl = lv_label_create(ui->screen_ota_onenet_btn_start);
    lv_label_set_text(btn_lbl, "检查并升级");
    lv_obj_set_style_text_color(btn_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(btn_lbl, &songti_font_16, 0);
    lv_obj_center(btn_lbl);

    // 跳转按钮（OTA下载成功后显示）
    ui->screen_ota_onenet_btn_jump = lv_btn_create(ui->screen_ota_onenet);
    lv_obj_set_size(ui->screen_ota_onenet_btn_jump, 200, 40);
    lv_obj_align(ui->screen_ota_onenet_btn_jump, LV_ALIGN_TOP_MID, 0, 185);
    lv_obj_set_style_bg_color(ui->screen_ota_onenet_btn_jump, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_bg_color(ui->screen_ota_onenet_btn_jump, lv_color_hex(0x388E3C), LV_STATE_PRESSED);
    lv_obj_set_style_radius(ui->screen_ota_onenet_btn_jump, 5, 0);
    lv_obj_add_flag(ui->screen_ota_onenet_btn_jump, LV_OBJ_FLAG_HIDDEN);  // 默认隐藏

    lv_obj_t *jump_lbl = lv_label_create(ui->screen_ota_onenet_btn_jump);
    lv_label_set_text(jump_lbl, "跳转到新固件");
    lv_obj_set_style_text_color(jump_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(jump_lbl, &songti_font_16, 0);
    lv_obj_center(jump_lbl);

    // 提示标签
    lv_obj_t *label_tip = lv_label_create(ui->screen_ota_onenet);
    lv_label_set_text(label_tip, "设备将从OneNET云平台\n自动检测并下载新版本");
    lv_obj_set_style_text_color(label_tip, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(label_tip, &songti_font_16, 0);
    lv_obj_set_style_text_align(label_tip, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label_tip, LV_ALIGN_TOP_MID, 0, 240);
    lv_obj_set_width(label_tip, 220);

    lv_obj_update_layout(ui->screen_ota_onenet);

    // 事件初始化
    events_init_screen_ota_onenet(ui);
}
