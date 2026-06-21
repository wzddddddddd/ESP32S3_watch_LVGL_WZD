#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "SD_card.h"

void setup_scr_screen_img_list(lv_ui *ui)
{
    // 创建界面
    ui->screen_img_list = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_img_list, 240, 284);
    lv_obj_set_scrollbar_mode(ui->screen_img_list, LV_SCROLLBAR_MODE_OFF);

    // 背景样式
    lv_obj_set_style_bg_opa(ui->screen_img_list, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_img_list, lv_color_hex(0x010101), LV_PART_MAIN|LV_STATE_DEFAULT);

    // 创建列表
    ui->screen_img_list_list_1 = lv_list_create(ui->screen_img_list);
    lv_obj_set_pos(ui->screen_img_list_list_1, 0, 0);
    lv_obj_set_size(ui->screen_img_list_list_1, 240, 284);
    lv_obj_set_scrollbar_mode(ui->screen_img_list_list_1, LV_SCROLLBAR_MODE_OFF);

    // 先清空所有 item
    for (int i = 0; i < _LIST_NUMBER; i++) {
        ui->screen_img_list_list_1_item[i] = NULL;
    }

    // ★ 定向扫描图片目录
    sd_scan_target_dir("/sdcard/images", ".png");

    // ★★★ 添加列表项 ★★★
    int item_idx = 0;
    for (int i = 0; i < s_file_list.count; i++)
    {
        if (item_idx >= _LIST_NUMBER) break;  // ★ 绝对护盾：装满40个强制停手

        ui->screen_img_list_list_1_item[item_idx] = lv_list_add_btn(
            ui->screen_img_list_list_1,
            &_image_alpha_30x30,
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
    lv_obj_add_style(ui->screen_img_list_list_1, &style_list_main, LV_PART_MAIN|LV_STATE_DEFAULT);

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

    for (int i = 0; i < _LIST_NUMBER; i++)
    {
        if (ui->screen_img_list_list_1_item[i] == NULL) continue;
        lv_obj_add_style(ui->screen_img_list_list_1_item[i], &style_list_btn, LV_PART_MAIN|LV_STATE_DEFAULT);
    }

    lv_obj_update_layout(ui->screen_img_list);
    events_init_screen_img_list(ui);
}