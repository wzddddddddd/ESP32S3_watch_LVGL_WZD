/*
 * setup_scr_screen_wifi_set.c
 * WiFi设置界面 —— 遵循 gui_guider 框架
 */

#include "lvgl.h"
#include "gui_guider.h"
#include "events_init.h"

// ==================== 自定义键盘布局（与 lvgl_keyboard.c 一致） ====================

static const char *wifi_kb_map_lower[] = {
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", "\n",
    "ABC", "z", "x", "c", "v", "b", "n", "m", LV_SYMBOL_BACKSPACE, "\n",
    "1#", " ", LV_SYMBOL_OK, ""
};
static const lv_btnmatrix_ctrl_t wifi_kb_ctrl_lower[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 1, 1, 1, 2,
    4, 2, 3
};

static const char *wifi_kb_map_upper[] = {
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "\n",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", "\n",
    "abc", "Z", "X", "C", "V", "B", "N", "M", LV_SYMBOL_BACKSPACE, "\n",
    "1#", " ", LV_SYMBOL_OK, ""
};
static const lv_btnmatrix_ctrl_t wifi_kb_ctrl_upper[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 1, 1, 1, 2,
    4, 2, 3
};

static const char *wifi_kb_map_special[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "+", "-", "*", "/", "=", "%", "@", "#", "!", "\n",
    "_", "<", ">", "[", "]", "(", ")", LV_SYMBOL_BACKSPACE, "\n",
    "abc", " ", LV_SYMBOL_OK, ""
};
static const lv_btnmatrix_ctrl_t wifi_kb_ctrl_special[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 2,
    4, 2, 3
};

// ==================== 隐藏光标辅助 ====================

static void wifi_hide_cursor(lv_obj_t *ta)
{
    lv_obj_set_style_width(ta, 0, LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(ta, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_set_style_anim_time(ta, 0, LV_PART_CURSOR);
}

// ==================== 界面创建 ====================

void setup_scr_screen_wifi_set(lv_ui *ui)
{
    // -------- 创建屏幕 --------
    ui->screen_wifi_set = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_wifi_set, 240, 284);
    lv_obj_set_scrollbar_mode(ui->screen_wifi_set, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(ui->screen_wifi_set, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(ui->screen_wifi_set, 255, 0);

    // -------- 标题 --------
    lv_obj_t *title = lv_label_create(ui->screen_wifi_set);
    lv_label_set_text(title, "WiFi Settings");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(title, &songti_font_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    // -------- SSID 标签 --------
    lv_obj_t *lbl_ssid = lv_label_create(ui->screen_wifi_set);
    lv_label_set_text(lbl_ssid, "WiFi SSID:");
    lv_obj_set_style_text_color(lbl_ssid, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_text_font(lbl_ssid, &songti_font_16, 0);
    lv_obj_align(lbl_ssid, LV_ALIGN_TOP_LEFT, 5, 35);

    // -------- SSID 输入框 --------
    ui->screen_wifi_set_ta_ssid = lv_textarea_create(ui->screen_wifi_set);
    lv_obj_set_size(ui->screen_wifi_set_ta_ssid, 230, 35);
    lv_obj_align(ui->screen_wifi_set_ta_ssid, LV_ALIGN_TOP_MID, 0, 52);
    lv_textarea_set_one_line(ui->screen_wifi_set_ta_ssid, true);
    lv_textarea_set_placeholder_text(ui->screen_wifi_set_ta_ssid, "Enter WiFi name");
    wifi_hide_cursor(ui->screen_wifi_set_ta_ssid);

    // -------- 密码标签 --------
    lv_obj_t *lbl_pwd = lv_label_create(ui->screen_wifi_set);
    lv_label_set_text(lbl_pwd, "Password:");
    lv_obj_set_style_text_color(lbl_pwd, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_text_font(lbl_pwd, &songti_font_16, 0);
    lv_obj_align(lbl_pwd, LV_ALIGN_TOP_LEFT, 5, 92);

    // -------- 密码输入框 --------
    ui->screen_wifi_set_ta_pwd = lv_textarea_create(ui->screen_wifi_set);
    lv_obj_set_size(ui->screen_wifi_set_ta_pwd, 230, 35);
    lv_obj_align(ui->screen_wifi_set_ta_pwd, LV_ALIGN_TOP_MID, 0, 109);
    lv_textarea_set_one_line(ui->screen_wifi_set_ta_pwd, true);
    lv_textarea_set_placeholder_text(ui->screen_wifi_set_ta_pwd, "Enter password");
    wifi_hide_cursor(ui->screen_wifi_set_ta_pwd);

    // -------- 连接按钮 --------
    ui->screen_wifi_set_btn_connect = lv_btn_create(ui->screen_wifi_set);
    lv_obj_set_size(ui->screen_wifi_set_btn_connect, 230, 38);
    lv_obj_align(ui->screen_wifi_set_btn_connect, LV_ALIGN_TOP_MID, 0, 155);
    lv_obj_set_style_bg_color(ui->screen_wifi_set_btn_connect,
                              lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_bg_color(ui->screen_wifi_set_btn_connect,
                              lv_color_hex(0x1565C0), LV_STATE_PRESSED);
    lv_obj_set_style_radius(ui->screen_wifi_set_btn_connect, 5, 0);

    lv_obj_t *btn_lbl = lv_label_create(ui->screen_wifi_set_btn_connect);
    lv_label_set_text(btn_lbl, "Connect");
    lv_obj_set_style_text_color(btn_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(btn_lbl, &songti_font_16, 0);
    lv_obj_center(btn_lbl);

    // -------- 状态标签 --------
    ui->screen_wifi_set_label_status = lv_label_create(ui->screen_wifi_set);
    lv_label_set_text(ui->screen_wifi_set_label_status, "Not connected");
    lv_obj_set_style_text_color(ui->screen_wifi_set_label_status,
                                lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(ui->screen_wifi_set_label_status,
                               &songti_font_16, 0);
    lv_obj_set_width(ui->screen_wifi_set_label_status, 230);
    lv_obj_set_style_text_align(ui->screen_wifi_set_label_status,
                                LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ui->screen_wifi_set_label_status, LV_ALIGN_TOP_MID, 0, 200);

    // -------- 键盘（初始隐藏） --------
    ui->screen_wifi_set_kb = lv_keyboard_create(ui->screen_wifi_set);
    lv_obj_set_size(ui->screen_wifi_set_kb, 240, 150);
    lv_obj_align(ui->screen_wifi_set_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(ui->screen_wifi_set_kb, LV_OBJ_FLAG_HIDDEN);

    // ★★★ 应用三套自定义键盘布局 ★★★
    lv_keyboard_set_map(ui->screen_wifi_set_kb, LV_KEYBOARD_MODE_TEXT_LOWER,
                        wifi_kb_map_lower, wifi_kb_ctrl_lower);
    lv_keyboard_set_map(ui->screen_wifi_set_kb, LV_KEYBOARD_MODE_TEXT_UPPER,
                        wifi_kb_map_upper, wifi_kb_ctrl_upper);
    lv_keyboard_set_map(ui->screen_wifi_set_kb, LV_KEYBOARD_MODE_SPECIAL,
                        wifi_kb_map_special, wifi_kb_ctrl_special);

    // -------- 绑定事件 --------
    events_init_screen_wifi_set(ui);

    ui->screen_wifi_set_del = false;
}
