#ifndef NOVEL_PROGRESS_H
#define NOVEL_PROGRESS_H

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief 保存某本小说的阅读偏移量
 * @param novel_path 小说完整路径（如 "/sdcard/宿命之环.txt"），中文也行
 * @param offset     当前阅读偏移量
 * @return ESP_OK 成功
 */
esp_err_t novel_progress_save(const char *novel_path, long offset);

/**
 * @brief 加载某本小说的阅读偏移量
 * @param novel_path 小说完整路径
 * @param offset     输出：上次的偏移量（未找到记录则返回0）
 * @return ESP_OK 成功 / ESP_ERR_NVS_NOT_FOUND 无记录
 */
esp_err_t novel_progress_load(const char *novel_path, long *offset);

/**
 * @brief 删除某本小说的阅读记录
 * @param novel_path 小说完整路径
 * @return ESP_OK 成功
 */
esp_err_t novel_progress_delete(const char *novel_path);

#endif // NOVEL_PROGRESS_H