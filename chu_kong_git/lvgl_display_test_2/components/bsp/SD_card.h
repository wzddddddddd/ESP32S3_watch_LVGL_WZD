#ifndef SD_CARD_H_
#define SD_CARD_H_
#include "esp_err.h"
#include "driver/sdmmc_types.h"

esp_err_t SD_card_init(void);
void SD_card_prepare_for_sleep(void);
void SD_card_deinit(void);

/**
 * @brief 获取SD卡句柄（供OTA跳转前清理使用）
 * @return sdmmc_card_t* SD卡句柄，未初始化返回NULL
 */
sdmmc_card_t* SD_card_get_card(void);




 esp_err_t novel_read_at_offset(long target_offset);
esp_err_t novel_next_page(void);
esp_err_t novel_prev_page(void);





// 仅存储文件信息（无文件夹）
typedef struct {
    char full_path[512]; // 文件完整路径（如 /sdcard/novel1.txt）
    char name[256];      // 文件名（如 novel1.txt）
} sd_file_info_t;

// 文件列表结构体
typedef struct {
    sd_file_info_t *files; // 文件数组
    int count;             // 文件总数
    int max_count;         // 数组最大容量
} sd_file_list_t;



 extern sd_file_list_t s_file_list;

/**
 * @brief 初始化文件列表（必须先调用）
 * @return esp_err_t ESP_OK:成功
 */
 esp_err_t sd_file_list_init(int need_count);

/**
 * @brief 遍历SD卡根目录，仅保存文件路径（排除文件夹）
 * @return esp_err_t ESP_OK:遍历成功 ESP_FAIL:打开目录失败
 */
esp_err_t sd_scan_root_files_save_paths(void);

/**
 * @brief 定向扫描指定目录的指定后缀文件
 * @param dir_path 目标目录，如 "/sdcard/novels"
 * @param filter_ext 后缀过滤，如 ".txt"（包含点号，NULL表示不过滤）
 *                   支持组合如 ".png|.jpg"
 * @return ESP_OK成功，其他失败
 */
esp_err_t sd_scan_target_dir(const char* dir_path, const char* filter_ext);

/**
 * @brief 获取保存的文件列表
 * @return sd_file_list_t* 文件列表指针（NULL=未初始化）
 */
sd_file_list_t* sd_get_file_list(void);

/**
 * @brief 释放文件列表内存
 */
void sd_file_list_deinit(void);


#endif
