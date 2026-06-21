#ifndef _WIFI_MANAGER_H_
#define _WIFI_MANAGER_H_
#include <stdbool.h>
#include "esp_err.h"
#include "esp_wifi.h"




typedef enum
{
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTED,
}WIFI_STATE;

//wifi状态变化回调函数
typedef void(*p_wifi_state_callback)(WIFI_STATE state);

/** 初始化wifi，默认进入STA模式
 * @param f wifi状态变化回调函数
 * @return 无 
*/
void wifi_manager_init(p_wifi_state_callback f);

/** 连接wifi
 * @param ssid
 * @param password
 * @return 成功/失败
*/
esp_err_t wifi_manager_connect(const char* ssid,const char* password);

/** 停止并反初始化WiFi（跳转固件前调用）
 * @return 无
*/
void wifi_manager_stop(void);

/** 重新启动 WiFi（从 Light Sleep 唤醒后调用）
 * @return 无
*/
void wifi_manager_start(void);

/** Returns whether WiFi is logically enabled (requested on) by the application/user. */
bool wifi_manager_is_enabled(void);

#endif
