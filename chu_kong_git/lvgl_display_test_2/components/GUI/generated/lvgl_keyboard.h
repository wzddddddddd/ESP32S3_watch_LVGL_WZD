#ifndef LVGL_KEYBOARD_H
#define LVGL_KEYBOARD_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化自定义键盘及账号/密码输入框
 * @param parent 父对象（通常传 lv_scr_act()）
 */
void lvgl_keyboard_init(lv_obj_t *parent);

/**
 * @brief 获取账号输入框中的文本
 * @return 账号字符串指针
 */
const char *lvgl_keyboard_get_account(void);

/**
 * @brief 获取密码输入框中的文本
 * @return 密码字符串指针
 */
const char *lvgl_keyboard_get_password(void);

/**
 * @brief 获取键盘对象指针
 * @return 键盘 lv_obj_t 指针
 */
lv_obj_t *lvgl_keyboard_get_kb(void);

/**
 * @brief 获取账号输入框对象指针
 */
lv_obj_t *lvgl_keyboard_get_ta_account(void);

/**
 * @brief 获取密码输入框对象指针
 */
lv_obj_t *lvgl_keyboard_get_ta_pwd(void);

/**
 * @brief 用户可注册的"确认提交"回调类型
 *        当密码框按下OK时触发
 */
typedef void (*lvgl_kb_submit_cb_t)(const char *account, const char *password);

/**
 * @brief 注册提交回调（密码框按OK后调用）
 */
void lvgl_keyboard_set_submit_cb(lvgl_kb_submit_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif /* LVGL_KEYBOARD_H */