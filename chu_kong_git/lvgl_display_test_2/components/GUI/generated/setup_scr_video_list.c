#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include "gui_guider.h"
#include "setup_scr_video_list.h"
#include "lvgl_display.h"
#include "events_init_video_list.h"

void setup_scr_video_list(lv_ui *ui)
{
    // 鍒涘缓鐣岄潰
    ui->video_list = lv_obj_create(NULL);
    lv_obj_set_size(ui->video_list, 240, 284);
    lv_obj_set_scrollbar_mode(ui->video_list, LV_SCROLLBAR_MODE_OFF);

    // 鑳屾櫙鏍峰紡
    lv_obj_set_style_bg_opa(ui->video_list, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->video_list, lv_color_hex(0x010101), LV_PART_MAIN|LV_STATE_DEFAULT);

    // 鍒涘缓鍒楄〃
    ui->video_list_list = lv_list_create(ui->video_list);
    lv_obj_set_pos(ui->video_list_list, 0, 0);
    lv_obj_set_size(ui->video_list_list, 240, 284);
    lv_obj_set_scrollbar_mode(ui->video_list_list, LV_SCROLLBAR_MODE_OFF);

    // 娓呯┖鎵€鏈?item
    for (int i = 0; i < _LIST_NUMBER; i++) {
        ui->video_list_list_item[i] = NULL;
    }

    // 鈽?瀹氬悜鎵弿瑙嗛鐩綍
    lv_list_add_text(ui->video_list_list, "Loading...");

    // 鍒楄〃鑳屾櫙鏍峰紡
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
    lv_obj_add_style(ui->video_list_list, &style_list_main, LV_PART_MAIN|LV_STATE_DEFAULT);

    // 鍒楄〃鎸夐挳鏍峰紡
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
        if (ui->video_list_list_item[i] == NULL) continue;
        lv_obj_add_style(ui->video_list_list_item[i], &style_list_btn, LV_PART_MAIN|LV_STATE_DEFAULT);
    }

    lv_obj_update_layout(ui->video_list);
    events_init_video_list(ui);

    lvgl_msg_send_nonblocking(LVGL_MSG_VIDEO_LIST_REFRESH_REQ, 0, NULL);
}
