/****************************************************************
 * ★★★ 新建文件 ★★★
 * setup_scr_screen_time_set.c — 时间设置界面
 * 功能：手动输入时间 + 网络NTP同步
 ****************************************************************/

#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include "gui_guider.h"
#include "events_init.h"
#include <time.h>
/* 需要获取当前时间来预填输入框 */
extern struct tm timeinfo;

/* 创建一个通用样式的 textarea（数字输入框）*/
static lv_obj_t* create_num_ta(lv_obj_t *parent,
                                lv_coord_t x, lv_coord_t y,
                                lv_coord_t w, lv_coord_t h,
                                uint8_t max_len,
                                const char *init_text)
{
    lv_obj_t *ta = lv_textarea_create(parent);
    lv_obj_set_pos(ta, x, y);
    lv_obj_set_size(ta, w, h);
    lv_textarea_set_max_length(ta, max_len);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_accepted_chars(ta, "0123456789");
    lv_textarea_set_text(ta, init_text);

    /* 样式：黑底白字，居中 */
    lv_obj_set_style_bg_color(ta, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_text_color(ta, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(ta, &songti_font_16, LV_PART_MAIN);
    lv_obj_set_style_text_align(ta, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_border_color(ta, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_set_style_border_width(ta, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(ta, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_top(ta, 4, LV_PART_MAIN);

    return ta;
}

/* 创建一个标签 */
static lv_obj_t* create_unit_label(lv_obj_t *parent,
                                    lv_coord_t x, lv_coord_t y,
                                    const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_pos(label, x, y);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &songti_font_16, LV_PART_MAIN);
    return label;
}


void setup_scr_screen_time_set(lv_ui *ui)
{
    char buf[16];

    /* ==================== 创建屏幕 ==================== */
    ui->screen_time_set = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_time_set, 240, 284);
    lv_obj_set_scrollbar_mode(ui->screen_time_set, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(ui->screen_time_set, lv_color_hex(0x1A1A2E), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui->screen_time_set, 255, LV_PART_MAIN);

    /* ==================== 标题 ==================== */
    lv_obj_t *title = lv_label_create(ui->screen_time_set);
    lv_label_set_text(title, "时间设置");
    lv_obj_set_pos(title, 0, 4);
    lv_obj_set_width(title, 240);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &songti_font_16, LV_PART_MAIN);

    /* ==================== 第1行：年 月 日 ==================== */
    /*  布局: [YYYY](55px) 年(18px) [MM](35px) 月(18px) [DD](35px) 日(18px)  */
    /*  起始x=8, 总宽度约 8+55+3+18+5+35+3+18+5+35+3+18 = 206               */

    lv_coord_t row1_y = 28;
    lv_coord_t ta_h   = 32;

    snprintf(buf, sizeof(buf), "%d", timeinfo.tm_year + 1900);
    ui->screen_time_set_ta_year = create_num_ta(
        ui->screen_time_set, 8, row1_y, 55, ta_h, 4, buf);

    create_unit_label(ui->screen_time_set, 65, row1_y + 7, "年");

    snprintf(buf, sizeof(buf), "%d", timeinfo.tm_mon + 1);
    ui->screen_time_set_ta_month = create_num_ta(
        ui->screen_time_set, 88, row1_y, 35, ta_h, 2, buf);

    create_unit_label(ui->screen_time_set, 125, row1_y + 7, "月");

    snprintf(buf, sizeof(buf), "%d", timeinfo.tm_mday);
    ui->screen_time_set_ta_day = create_num_ta(
        ui->screen_time_set, 148, row1_y, 35, ta_h, 2, buf);

    create_unit_label(ui->screen_time_set, 185, row1_y + 7, "日");

    /* ==================== 第2行：时 分 秒 ==================== */
    lv_coord_t row2_y = row1_y + ta_h + 8;  /* = 68 */

    snprintf(buf, sizeof(buf), "%d", timeinfo.tm_hour);
    ui->screen_time_set_ta_hour = create_num_ta(
        ui->screen_time_set, 18, row2_y, 35, ta_h, 2, buf);

    create_unit_label(ui->screen_time_set, 55, row2_y + 7, "时");

    snprintf(buf, sizeof(buf), "%d", timeinfo.tm_min);
    ui->screen_time_set_ta_min = create_num_ta(
        ui->screen_time_set, 83, row2_y, 35, ta_h, 2, buf);

    create_unit_label(ui->screen_time_set, 120, row2_y + 7, "分");

    snprintf(buf, sizeof(buf), "%d", timeinfo.tm_sec);
    ui->screen_time_set_ta_sec = create_num_ta(
        ui->screen_time_set, 148, row2_y, 35, ta_h, 2, buf);

    create_unit_label(ui->screen_time_set, 185, row2_y + 7, "秒");

    /* ==================== 手动确认按钮 ==================== */
    lv_coord_t btn_y = row2_y + ta_h + 10;  /* = 110 */

    ui->screen_time_set_btn_confirm = lv_btn_create(ui->screen_time_set);
    lv_obj_set_pos(ui->screen_time_set_btn_confirm, 30, btn_y);
    lv_obj_set_size(ui->screen_time_set_btn_confirm, 180, 36);
    lv_obj_set_style_bg_color(ui->screen_time_set_btn_confirm,
                               lv_color_hex(0x2196F3), LV_PART_MAIN);
    lv_obj_set_style_radius(ui->screen_time_set_btn_confirm, 8, LV_PART_MAIN);

    lv_obj_t *btn1_label = lv_label_create(ui->screen_time_set_btn_confirm);
    lv_label_set_text(btn1_label, "手动确认");
    lv_obj_center(btn1_label);
    lv_obj_set_style_text_font(btn1_label, &songti_font_16, LV_PART_MAIN);

    /* ==================== 网络更新按钮 ==================== */
    lv_coord_t btn2_y = btn_y + 42;  /* = 152 */

    ui->screen_time_set_btn_ntp = lv_btn_create(ui->screen_time_set);
    lv_obj_set_pos(ui->screen_time_set_btn_ntp, 30, btn2_y);
    lv_obj_set_size(ui->screen_time_set_btn_ntp, 180, 36);
    lv_obj_set_style_bg_color(ui->screen_time_set_btn_ntp,
                               lv_color_hex(0x4CAF50), LV_PART_MAIN);
    lv_obj_set_style_radius(ui->screen_time_set_btn_ntp, 8, LV_PART_MAIN);

    lv_obj_t *btn2_label = lv_label_create(ui->screen_time_set_btn_ntp);
    lv_label_set_text(btn2_label, "网络更新");
    lv_obj_center(btn2_label);
    lv_obj_set_style_text_font(btn2_label, &songti_font_16, LV_PART_MAIN);

    /* ==================== 状态标签 ==================== */
    ui->screen_time_set_label_status = lv_label_create(ui->screen_time_set);
    lv_obj_set_pos(ui->screen_time_set_label_status, 0, btn2_y + 40);
    lv_obj_set_width(ui->screen_time_set_label_status, 240);
    lv_label_set_text(ui->screen_time_set_label_status, "");
    lv_obj_set_style_text_align(ui->screen_time_set_label_status,
                                 LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui->screen_time_set_label_status,
                                 lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_set_style_text_font(ui->screen_time_set_label_status,
                                &songti_font_16, LV_PART_MAIN);

    /* ==================== 数字键盘（默认隐藏）==================== */
    ui->screen_time_set_kb = lv_keyboard_create(ui->screen_time_set);
    lv_keyboard_set_mode(ui->screen_time_set_kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_set_size(ui->screen_time_set_kb, 240, 140);
    lv_obj_align(ui->screen_time_set_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(ui->screen_time_set_kb, LV_OBJ_FLAG_HIDDEN);

    /* ==================== 更新布局 & 注册事件 ==================== */
    lv_obj_update_layout(ui->screen_time_set);
    events_init_screen_time_set(ui);
}