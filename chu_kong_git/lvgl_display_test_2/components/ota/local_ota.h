#ifndef _LOCAL_OTA_H_
#define _LOCAL_OTA_H_
#include "esp_err.h"
#include "lvgl.h"

/**
 * 开始本地OTA升级（创建后台任务，不阻塞）
 * @param file_path SD卡上的.bin文件完整路径
 * @return ESP_OK成功，其他失败
 */
esp_err_t local_ota_start(const char *file_path);

/**
 * 切换到备用分区并重启
 * @return ESP_OK成功，其他失败
 */
esp_err_t local_ota_switch_partition(void);

/**
 * 获取当前运行分区信息
 * @param running_label 输出：当前分区标签名
 * @param standby_label 输出：备用分区标签名
 * @param running_version 输出：当前分区版本
 */
void local_ota_get_partition_info(char *running_label, char *standby_label, char *running_version);

/**
 * 更新本地OTA消息框文本（由lvgl_display.c调用）
 * @param text 新的显示文本
 */
void local_ota_update_msgbox(const char *text);

/**
 * 关闭本地OTA消息框
 */
void local_ota_close_msgbox(void);

/**
 * 取消正在进行的本地OTA下载
 */
void local_ota_cancel(void);

#endif