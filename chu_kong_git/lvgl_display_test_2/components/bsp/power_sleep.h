#ifndef _POWER_SLEEP_H_
#define _POWER_SLEEP_H_

#include <stdbool.h>

/**
 * @brief 初始化自动睡眠模块
 * @note 应在 app_main 中调用
 */
void power_sleep_boot_init(void);
void power_sleep_init(void);

/**
 * @brief 重置 2 分钟空闲计时器
 * @note 按键或触摸时调用
 */
void power_sleep_reset_timer(void);
void power_sleep_request_sleep(void);
void power_sleep_request_deep_sleep(void);
bool power_sleep_wake_key_guard_active(void);
void power_sleep_clear_wake_key_guard(void);

#endif
