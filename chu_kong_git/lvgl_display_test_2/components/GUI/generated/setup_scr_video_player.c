#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "setup_scr_video_player.h"
#include "video_player.h"
#include "events_init_video_player.h"

#define VIDEO_WIDTH   240
#define VIDEO_HEIGHT  284

// 外部全局变量（由events_init_video_list.c设置）
extern char g_video_filepath[512];
extern video_format_t g_video_format;

void setup_scr_video_player(lv_ui *ui)
{
    // 创建全屏黑色背景
    ui->video_player = lv_obj_create(NULL);
    lv_obj_set_size(ui->video_player, 240, 284);
    lv_obj_set_style_bg_color(ui->video_player, lv_color_black(), 0);

    // 全屏图片显示（居中）
    ui->video_player_img = lv_img_create(ui->video_player);
    lv_obj_set_size(ui->video_player_img, VIDEO_WIDTH, VIDEO_HEIGHT);
    lv_obj_align(ui->video_player_img, LV_ALIGN_CENTER, 0, 0);

    // 全屏透明触摸按钮（用于点击暂停）
    ui->video_player_btn_touch = lv_btn_create(ui->video_player);
    lv_obj_set_size(ui->video_player_btn_touch, 240, 284);
    lv_obj_set_style_bg_opa(ui->video_player_btn_touch, 0, 0);
    lv_obj_set_style_border_width(ui->video_player_btn_touch, 0, 0);
    lv_obj_align(ui->video_player_btn_touch, LV_ALIGN_CENTER, 0, 0);

    // 右上角×关闭按钮
    ui->video_player_btn_close = lv_btn_create(ui->video_player);
    lv_obj_set_size(ui->video_player_btn_close, 35, 35);
    lv_obj_align(ui->video_player_btn_close, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_set_style_bg_color(ui->video_player_btn_close, lv_color_hex(0x505050), 0);
    lv_obj_set_style_radius(ui->video_player_btn_close, 17, 0);

    lv_obj_t *label_close = lv_label_create(ui->video_player_btn_close);
    lv_label_set_text(label_close, "X");
    lv_obj_set_style_text_color(label_close, lv_color_white(), 0);
    lv_obj_center(label_close);

    // 事件绑定
    events_init_video_player(ui);

    // 保存格式
    ui->video_player_format = g_video_format;

    // 启动播放（从全局变量读取文件路径）
    video_player_start(g_video_filepath, g_video_format);
}