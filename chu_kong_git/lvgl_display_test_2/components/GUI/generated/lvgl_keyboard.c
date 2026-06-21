#include "lvgl_keyboard.h"
#include "esp_log.h"

#define TAG "LVGL_KB"

// ==================== 自定义键盘布局 ====================

// �?小写字母键盘
static const char *kb_map_lower[] = {
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", "\n",
    "ABC", "z", "x", "c", "v", "b", "n", "m", LV_SYMBOL_BACKSPACE, "\n",
    "1#", " ", LV_SYMBOL_OK, ""
};
static const lv_btnmatrix_ctrl_t kb_ctrl_lower[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 1, 1, 1, 2,
    4, 2, 3
};

// �?大写字母键盘
static const char *kb_map_upper[] = {
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "\n",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", "\n",
    "abc", "Z", "X", "C", "V", "B", "N", "M", LV_SYMBOL_BACKSPACE, "\n",
    "1#", " ", LV_SYMBOL_OK, ""
};
static const lv_btnmatrix_ctrl_t kb_ctrl_upper[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 1, 1, 1, 1, 1, 1, 1, 2,
    4, 2, 3
};

// �?数字符号键盘
static const char *kb_map_special[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "+", "-", "*", "/", "=", "%", "@", "#", "!", "\n",
    "_", "<", ">", "[", "]", "(", ")", LV_SYMBOL_BACKSPACE, "\n",
    "abc", " ", LV_SYMBOL_OK, ""
};
static const lv_btnmatrix_ctrl_t kb_ctrl_special[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 2,
    4, 2, 3
};

// ==================== 模块内部全局变量 ====================

static lv_obj_t *g_kb         = NULL;
static lv_obj_t *g_ta_account = NULL;
static lv_obj_t *g_ta_pwd     = NULL;

static lv_keyboard_mode_t g_last_kb_mode = LV_KEYBOARD_MODE_TEXT_LOWER;

static lvgl_kb_submit_cb_t g_submit_cb = NULL;   // 用户注册的提交回�?

// ==================== 内部回调函数 ====================

/**
 * @brief 键盘事件回调
 */
static void kb_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_keyboard_mode_t cur_mode = lv_keyboard_get_mode(kb);
        if (cur_mode != g_last_kb_mode) {
            g_last_kb_mode = cur_mode;
            lv_indev_t *indev = lv_indev_get_act();
            if (indev != NULL) {
                /* non-blocking: skip wait_release to avoid wakeup deadlock */
            }
        }
    }
    else if (code == LV_EVENT_READY) {
        lv_obj_t *cur_ta = lv_keyboard_get_textarea(kb);

        if (cur_ta == g_ta_account) {
            /* 账号框按OK �?跳转到密码框 */
            lv_keyboard_set_textarea(g_kb, g_ta_pwd);
            lv_obj_clear_state(g_ta_account, LV_STATE_FOCUSED);
            lv_obj_add_state(g_ta_pwd, LV_STATE_FOCUSED);
            ESP_LOGI(TAG, "账号输入完毕，跳转到密码输入");
        }
        else if (cur_ta == g_ta_pwd) {
            /* 密码框按OK �?提交 */
            const char *account = lv_textarea_get_text(g_ta_account);
            const char *pwd     = lv_textarea_get_text(g_ta_pwd);
            ESP_LOGI(TAG, "账号: %s, 密码: %s", account, pwd);

            /* 调用用户注册的回�?*/
            if (g_submit_cb) {
                g_submit_cb(account, pwd);
            }

            /* 清空输入框并隐藏键盘 */
            lv_textarea_set_text(g_ta_account, "");
            lv_textarea_set_text(g_ta_pwd, "");
            lv_obj_clear_state(g_ta_pwd, LV_STATE_FOCUSED);
            lv_obj_add_flag(g_kb, LV_OBJ_FLAG_HIDDEN);
        }
    }
    else if (code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(g_kb, LV_OBJ_FLAG_HIDDEN);
    }
}

/**
 * @brief 点击账号输入框回�?
 */
static void ta_account_click_cb(lv_event_t *e)
{
    lv_keyboard_set_textarea(g_kb, g_ta_account);
    lv_keyboard_set_mode(g_kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    g_last_kb_mode = LV_KEYBOARD_MODE_TEXT_LOWER;
    lv_obj_clear_flag(g_kb, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief 点击密码输入框回�?
 */
static void ta_pwd_click_cb(lv_event_t *e)
{
    lv_keyboard_set_textarea(g_kb, g_ta_pwd);
    lv_keyboard_set_mode(g_kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    g_last_kb_mode = LV_KEYBOARD_MODE_TEXT_LOWER;
    lv_obj_clear_flag(g_kb, LV_OBJ_FLAG_HIDDEN);
}

// ==================== 隐藏光标的辅助函�?====================

static void hide_cursor(lv_obj_t *ta)
{
    lv_obj_set_style_width(ta, 0, LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(ta, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_set_style_anim_time(ta, 0, LV_PART_CURSOR);
}

// ==================== 对外接口 ====================

void lvgl_keyboard_init(lv_obj_t *parent)
{
    /* -------- 账号标签 -------- */
    lv_obj_t *label_acc = lv_label_create(parent);
    lv_label_set_text(label_acc, "Account:");
    lv_obj_align(label_acc, LV_ALIGN_TOP_LEFT, 5, 10);

    /* -------- 账号输入�?-------- */
    g_ta_account = lv_textarea_create(parent);
    lv_obj_set_size(g_ta_account, 230, 35);
    lv_obj_align(g_ta_account, LV_ALIGN_TOP_MID, 0, 30);
    lv_textarea_set_one_line(g_ta_account, true);
    lv_textarea_set_placeholder_text(g_ta_account, "Enter account");
    lv_obj_add_event_cb(g_ta_account, ta_account_click_cb, LV_EVENT_CLICKED, NULL);
    hide_cursor(g_ta_account);

    /* -------- 密码标签 -------- */
    lv_obj_t *label_pwd = lv_label_create(parent);
    lv_label_set_text(label_pwd, "Password:");
    lv_obj_align(label_pwd, LV_ALIGN_TOP_LEFT, 5, 70);

    /* -------- 密码输入�?-------- */
    g_ta_pwd = lv_textarea_create(parent);
    lv_obj_set_size(g_ta_pwd, 230, 35);
    lv_obj_align(g_ta_pwd, LV_ALIGN_TOP_MID, 0, 90);
    lv_textarea_set_one_line(g_ta_pwd, true);
    lv_textarea_set_placeholder_text(g_ta_pwd, "Enter password");
    lv_obj_add_event_cb(g_ta_pwd, ta_pwd_click_cb, LV_EVENT_CLICKED, NULL);
    hide_cursor(g_ta_pwd);

    /* -------- 键盘 -------- */
    g_kb = lv_keyboard_create(parent);
    lv_obj_set_size(g_kb, 240, 150);
    lv_obj_align(g_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(g_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(g_kb, kb_event_cb, LV_EVENT_ALL, NULL);

    /* 应用三套自定义布局 */
    lv_keyboard_set_map(g_kb, LV_KEYBOARD_MODE_TEXT_LOWER,
                        kb_map_lower, kb_ctrl_lower);
    lv_keyboard_set_map(g_kb, LV_KEYBOARD_MODE_TEXT_UPPER,
                        kb_map_upper, kb_ctrl_upper);
    lv_keyboard_set_map(g_kb, LV_KEYBOARD_MODE_SPECIAL,
                        kb_map_special, kb_ctrl_special);

    ESP_LOGI(TAG, "LVGL keyboard initialized");
}

const char *lvgl_keyboard_get_account(void)
{
    if (g_ta_account) return lv_textarea_get_text(g_ta_account);
    return "";
}

const char *lvgl_keyboard_get_password(void)
{
    if (g_ta_pwd) return lv_textarea_get_text(g_ta_pwd);
    return "";
}

lv_obj_t *lvgl_keyboard_get_kb(void)
{
    return g_kb;
}

lv_obj_t *lvgl_keyboard_get_ta_account(void)
{
    return g_ta_account;
}

lv_obj_t *lvgl_keyboard_get_ta_pwd(void)
{
    return g_ta_pwd;
}

void lvgl_keyboard_set_submit_cb(lvgl_kb_submit_cb_t cb)
{
    g_submit_cb = cb;
}
