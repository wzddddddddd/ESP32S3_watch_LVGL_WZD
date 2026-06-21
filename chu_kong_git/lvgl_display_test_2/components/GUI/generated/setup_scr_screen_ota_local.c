/*
 * setup_scr_screen_ota_local.c
 * 本地升级界面 —— SD卡.bin文件列表
 */

#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include "gui_guider.h"
#include "events_init.h"
#include "SD_card.h"

void setup_scr_screen_ota_local(lv_ui *ui)
{
    // 创建界面
    ui->screen_ota_local = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_ota_local, 240, 284);
    lv_obj_set_scrollbar_mode(ui->screen_ota_local, LV_SCROLLBAR_MODE_OFF);

    // 背景样式
    lv_obj_set_style_bg_opa(ui->screen_ota_local, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_ota_local, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);

    // 标题
    lv_obj_t *title = lv_label_create(ui->screen_ota_local);
    lv_label_set_text(title, "本地升级");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(title, &songti_font_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // 创建列表
    ui->screen_ota_local_list_1 = lv_list_create(ui->screen_ota_local);
    lv_obj_set_pos(ui->screen_ota_local_list_1, 0, 40);
    lv_obj_set_size(ui->screen_ota_local_list_1, 240, 200);
    lv_obj_set_scrollbar_mode(ui->screen_ota_local_list_1, LV_SCROLLBAR_MODE_OFF);

    // 先清空所有item
    for (int i = 0; i < 20; i++) {
        ui->screen_ota_local_list_1_item[i] = NULL;
    }

    // ★ 定向扫描固件目录
    sd_scan_target_dir("/sdcard/firmware", ".bin");

    // ★★★ 添加.bin文件到列表 ★★★
    int item_idx = 0;
    for (int i = 0; i < s_file_list.count; i++)
    {
        if (item_idx >= 20) break;

        ui->screen_ota_local_list_1_item[item_idx] = lv_list_add_btn(
            ui->screen_ota_local_list_1,
            NULL,
            s_file_list.files[i].name
        );
        item_idx++;
    }

    // 列表背景样式
    static lv_style_t style_list_main;
    ui_init_style(&style_list_main);
    lv_style_set_pad_top(&style_list_main, 0);
    lv_style_set_pad_left(&style_list_main, 0);
    lv_style_set_pad_right(&style_list_main, 5);
    lv_style_set_pad_bottom(&style_list_main, 5);
    lv_style_set_bg_opa(&style_list_main, 255);
    lv_style_set_bg_color(&style_list_main, lv_color_hex(0xffffff));
    lv_style_set_border_width(&style_list_main, 0);
    lv_style_set_radius(&style_list_main, 3);
    lv_style_set_shadow_width(&style_list_main, 0);
    lv_obj_add_style(ui->screen_ota_local_list_1, &style_list_main, LV_PART_MAIN|LV_STATE_DEFAULT);

    // 列表按钮样式
    static lv_style_t style_list_btn;
    ui_init_style(&style_list_btn);
    lv_style_set_pad_top(&style_list_btn, 5);
    lv_style_set_pad_left(&style_list_btn, 5);
    lv_style_set_pad_right(&style_list_btn, 5);
    lv_style_set_pad_bottom(&style_list_btn, 5);
    lv_style_set_border_width(&style_list_btn, 0);
    lv_style_set_text_color(&style_list_btn, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_list_btn, &songti_font_16);
    lv_style_set_text_opa(&style_list_btn, 255);
    lv_style_set_radius(&style_list_btn, 3);
    lv_style_set_bg_opa(&style_list_btn, 255);
    lv_style_set_bg_color(&style_list_btn, lv_color_hex(0xffffff));

    for (int i = 0; i < 20; i++)
    {
        if (ui->screen_ota_local_list_1_item[i] == NULL) continue;
        lv_obj_add_style(ui->screen_ota_local_list_1_item[i], &style_list_btn, LV_PART_MAIN|LV_STATE_DEFAULT);
    }

    // 提示标签
    lv_obj_t *label_tip = lv_label_create(ui->screen_ota_local);
    lv_label_set_text(label_tip, "请将固件(.bin)放入SD卡firmware目录");
    lv_obj_set_style_text_color(label_tip, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(label_tip, &songti_font_16, 0);
    lv_obj_align(label_tip, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_obj_update_layout(ui->screen_ota_local);

    // 事件初始化
    events_init_screen_ota_local(ui);
}
