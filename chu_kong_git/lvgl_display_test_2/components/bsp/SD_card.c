#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/spi_common.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "SD_card.h"
#include "dirent.h"
#include "gui_guider.h"




#define EXAMPLE_MAX_CHAR_SIZE    64

static const char *TAG = "SD";

#define MOUNT_POINT "/sdcard"   //挂载点名称
#define SD_INIT_RETRY_MAX 3
#define SD_INIT_RETRY_DELAY_MS 300
#define SD_INIT_POWERUP_DELAY_MS 250
#define SD_SPI_CLK_GPIO GPIO_NUM_39
#define SD_SPI_MOSI_GPIO GPIO_NUM_40
#define SD_SPI_MISO_GPIO GPIO_NUM_41
#define SD_SPI_CS_GPIO GPIO_NUM_42

#define SD_CMD_GO_IDLE_STATE 0

// 前向声明（static 函数定义在文件末尾）
static int sd_scan_file_count(void);
static esp_err_t sd_card_send_idle_sequence(sdmmc_card_t *card);


static esp_err_t s_example_write_file(const char *path, char *data)
{
    ESP_LOGI(TAG, "Opening file %s", path);
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, data);
    fclose(f);
    ESP_LOGI(TAG, "File written");

    return ESP_OK;
}

static esp_err_t s_example_read_file(const char *path)
{
    ESP_LOGI(TAG, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[EXAMPLE_MAX_CHAR_SIZE];
    fgets(line, sizeof(line), f);
    fclose(f);

    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    return ESP_OK;
}



/**
 * @brief 整合后的SD卡初始化函数：初始化SD卡 + 预扫描数量 + 初始化列表 + 遍历文件
 */
static sdmmc_card_t *s_sd_card = NULL;  // 保存SD卡句柄，供跳转清理时使用

esp_err_t SD_card_init(void)
{
    esp_err_t ret = ESP_FAIL;
    sdmmc_card_t *card = NULL;
    ESP_LOGI(TAG, "开始初始化SD卡...");
    vTaskDelay(pdMS_TO_TICKS(SD_INIT_POWERUP_DELAY_MS));

    for (int attempt = 1; attempt <= SD_INIT_RETRY_MAX; attempt++) {
        esp_vfs_fat_sdmmc_mount_config_t mount_config = {
            .format_if_mount_failed = false,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024
        };
        sdmmc_host_t host = SDSPI_HOST_DEFAULT();
        host.max_freq_khz = SDMMC_FREQ_HIGHSPEED / 2;

        spi_bus_config_t bus_cfg = {
            .mosi_io_num = SD_SPI_MOSI_GPIO,
            .miso_io_num = SD_SPI_MISO_GPIO,
            .sclk_io_num = SD_SPI_CLK_GPIO,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 64 * 1024,
        };

        ret = spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
        if (ret == ESP_ERR_INVALID_STATE) {
            spi_bus_free(SPI3_HOST);
            vTaskDelay(pdMS_TO_TICKS(20));
            ret = spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
        }
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "SPI总线初始化失败，第%d/%d次：%s",
                     attempt, SD_INIT_RETRY_MAX, esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(SD_INIT_RETRY_DELAY_MS));
            continue;
        }

        sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
        slot_config.gpio_cs = SD_SPI_CS_GPIO;
        slot_config.host_id = SPI3_HOST;

        ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
        if (ret == ESP_OK) {
            break;
        }

        ESP_LOGW(TAG, "SD挂载失败，第%d/%d次：%s",
                 attempt, SD_INIT_RETRY_MAX, esp_err_to_name(ret));
        spi_bus_free(SPI3_HOST);
        vTaskDelay(pdMS_TO_TICKS(SD_INIT_RETRY_DELAY_MS));
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD卡初始化失败：%s", esp_err_to_name(ret));
        return ret;
    }

    // 打印SD卡信息（原有逻辑不变）
    sdmmc_card_print_info(stdout, card);
    ESP_LOGI(TAG, "SD卡挂载成功！");

    // ===== 新增：预扫描SD卡文件数量 =====
    int file_count = sd_scan_file_count();
    if (file_count < 0) {
        ESP_LOGE(TAG, "预扫描文件数量失败！");
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
        return ESP_FAIL;
    }

    // ===== 修改：按实际数量初始化文件列表 =====
    ret = sd_file_list_init(file_count);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "文件列表初始化失败！");
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
        return ret;
    }

    // 4. 遍历SD卡文件并存储到结构体（原有逻辑不变）
    ret = sd_scan_root_files_save_paths();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD卡文件遍历失败！");
        sd_file_list_deinit();
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
        return ret;
    }

    ESP_LOGI(TAG, "SD卡初始化+文件遍历完成！");

    // 保存SD卡句柄供后续跳转清理使用
    s_sd_card = card;

    return ESP_OK;
}

/**
 * @brief 获取SD卡句柄（供OTA跳转前清理使用）
 * @return sdmmc_card_t* SD卡句柄，未初始化返回NULL
 */
static esp_err_t sd_card_send_idle_sequence(sdmmc_card_t *card)
{
    if (card == NULL || card->host.do_transaction == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    sdmmc_command_t cmd = {
        .opcode = SD_CMD_GO_IDLE_STATE,
        .arg = 0,
        .flags = SCF_CMD_BC | SCF_RSP_R0,
        .data = NULL,
        .datalen = 0,
        .buflen = 0,
        .blklen = 0,
        .timeout_ms = 20,
    };

    return card->host.do_transaction(card->host.slot, &cmd);
}

void SD_card_prepare_for_sleep(void)
{
    if (s_sd_card == NULL) {
        return;
    }

    esp_err_t ret = sd_card_send_idle_sequence(s_sd_card);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "SD idle command failed: %s", esp_err_to_name(ret));
    }
}

void SD_card_deinit(void)
{
    if (s_sd_card == NULL) {
        sd_file_list_deinit();
        return;
    }

    ESP_LOGI(TAG, "deinit SD card");
    sd_file_list_deinit();
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, s_sd_card);

    esp_err_t ret = spi_bus_free(SPI3_HOST);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "spi_bus_free failed during SD deinit: %s", esp_err_to_name(ret));
    }

    s_sd_card = NULL;
}

sdmmc_card_t* SD_card_get_card(void)
{
    return s_sd_card;
}

// ====== 新增：全局阅读进度变量 ======
 long g_file_offset = 0;        // 当前读取的文件偏移量（记录到哪一页）
static const int g_read_len = 450;    // 每页读取500字节（和你原代码一致）
// 复用你原有的缓冲区定义（移到全局，避免重复定义）
const char *novel_path = MOUNT_POINT"/宿命之环.txt";
char read_buf[1024] = {0};
char display_buf[1200] = {0};
// ===================================
FILE *fp = NULL;





/**
 * @brief 核心辅助函数：读取指定偏移量的内容并处理格式填充到 display_buf
 * @param target_offset 想要读取的文件偏移量
 * @return esp_err_t ESP_OK:成功 / ESP_ERR_NOT_FOUND:读到末尾 / ESP_FAIL:文件错误
 */
 esp_err_t novel_read_at_offset(long target_offset) {
    // 1. 文件句柄保护
    if (fp == NULL) {
        // 如果文件未打开，尝试打开默认路径
            ESP_LOGE(TAG, "文件打开失败");
            return ESP_FAIL;
        
    }

    // 2. 定位文件
    if (fseek(fp, target_offset, SEEK_SET) != 0) {
        ESP_LOGE(TAG, "文件定位失败 offset=%ld", target_offset);
        return ESP_FAIL;
    }

    // 3. 读取原始数据
    // 注意：这里不要关闭文件！文件应该在退出阅读模式时关闭。
    size_t read_len = fread(read_buf, 1, g_read_len, fp);
    if (read_len == 0) {
        return ESP_ERR_NOT_FOUND; // 读到末尾了
    }
    read_buf[read_len] = '\0'; // 确保原始字符串结束

    // 4. 文本格式处理 (完全复用你的逻辑)
    int buf_idx = 0;
    int last_char_is_newline = 0;
    char *p = read_buf;
    const int MAX_BUF_LEN = sizeof(display_buf) - 1;

    // 清空输出缓冲区
    memset(display_buf, 0, sizeof(display_buf));

    // 首行缩进逻辑
    if (buf_idx + 4 <= MAX_BUF_LEN) {
        display_buf[buf_idx++] = ' ';
        display_buf[buf_idx++] = ' ';
        display_buf[buf_idx++] = ' ';
        display_buf[buf_idx++] = ' ';
        last_char_is_newline = 0;
    }

    // 遍历字符处理逻辑 (你的原始内容)
    while (*p != '\0' && buf_idx < MAX_BUF_LEN) {
        
        if (*p == 0xE3 && *(p+1) == 0x80 && *(p+2) == 0x80) {
            if (!last_char_is_newline) {
                if (buf_idx + 1 <= MAX_BUF_LEN) {
                    display_buf[buf_idx++] = '\n';
                    last_char_is_newline = 1;
                }
            }
            p += 3;
            continue;
        } else if (*p == ' ') {
            p++;
            continue;
        } else if ((*p & 0xF0) == 0xE0 && (*(p+1) & 0xC0) == 0x80 && (*(p+2) & 0xC0) == 0x80) {
            if (buf_idx + 3 > MAX_BUF_LEN) break;
            display_buf[buf_idx++] = *p;
            display_buf[buf_idx++] = *(p+1);
            display_buf[buf_idx++] = *(p+2);
            p += 3;
            last_char_is_newline = 0;
        } else if (*p >= '0' && *p <= '9') {
            if (buf_idx + 1 > MAX_BUF_LEN) break;
            display_buf[buf_idx++] = *p;
            p += 1;
            last_char_is_newline = 0;
        } else if (*p == 0xEF &&( *(p+1) & 0xC0 )== 0x80 &&( *(p+2) & 0xC0) == 0x80) {
            if (buf_idx + 3 > MAX_BUF_LEN) break;
            display_buf[buf_idx++] = *p;
            display_buf[buf_idx++] = *(p+1);
            display_buf[buf_idx++] = *(p+2);
            p += 3;
            last_char_is_newline = 0;
        } else {
            p++;
            continue;
        }
    }
    
    // 补结束符
    display_buf[buf_idx] = '\0';

    return ESP_OK;
}







/**
 * @brief 读取下一页数据到 display_buf
 * @note 不操作 LVGL
 */
esp_err_t novel_next_page(void) {
    // 1. 计算目标偏移量
    // 如果是第一次读(offset=0且buf为空)，则读第0页；否则读下一页
    long target_offset = g_file_offset;
    
    // 只有当 display_buf 已经有内容时，才认为需要往后翻
    // (这是一个简单的防呆逻辑，你也可以直接写 target_offset = g_file_offset + g_read_len)
     if ((strlen(display_buf) > 0)) { 
        target_offset += g_read_len;
    }

    // 2. 尝试读取
    esp_err_t ret = novel_read_at_offset(target_offset);

    if (ret == ESP_OK) {
        // 读取成功，确认更新全局偏移量
        g_file_offset = target_offset;
        ESP_LOGI(TAG, "翻页成功(Next)，当前Offset: %ld", g_file_offset);
    } else if (ret == ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "已到末尾，无法翻页");
        // 可以在这里在这个 display_buf 里填入提示词，或者由外部处理
        // snprintf(display_buf, sizeof(display_buf), "\n\n--- 已经阅读到末尾 ---");
    }

    return ret;
}

/**
 * @brief 读取上一页数据到 display_buf
 * @note 不操作 LVGL
 */
esp_err_t novel_prev_page(void) {
    // 1. 边界检查
    if (g_file_offset == 0) {
        ESP_LOGW(TAG, "已经是第一页");
        // 即使是第一页，也重新读一遍，防止 buffer 为空
        return novel_read_at_offset(0); 
    }

    // 2. 计算目标偏移量
    long target_offset = g_file_offset - g_read_len;
    if (target_offset < 0) {
        target_offset = 0;
    }

    // 3. 尝试读取
    esp_err_t ret = novel_read_at_offset(target_offset);

    if (ret == ESP_OK) {
        // 读取成功，更新全局偏移量
        g_file_offset = target_offset;
        ESP_LOGI(TAG, "翻页成功(Prev)，当前Offset: %ld", g_file_offset);
    }

    return ret;
}












// 全局文件列表（静态，存到DIRAM，不占IRAM）
 sd_file_list_t s_file_list = {
    .files = NULL,
    .count = 0,
    .max_count = 0
};

/**
 * @brief 扩展文件列表容量
 */
static bool sd_file_list_expand(int need_count) {
    if (s_file_list.max_count >= need_count) return true;

    // 每次扩容10个文件位置（足够日常使用）
    int new_max = need_count;
    sd_file_info_t *new_files = (sd_file_info_t*)realloc(s_file_list.files, new_max * sizeof(sd_file_info_t));
    if (new_files == NULL) {
        ESP_LOGE(TAG, "文件列表扩容失败！需要容量：%d", new_max);
        return false;
    }

    s_file_list.files = new_files;
    s_file_list.max_count = new_max;
    return true;
}

/**
 * @brief 初始化文件列表（按实际文件数量分配容量）
 * @param need_count 需要的容量（文件总数）
 * @return esp_err_t ESP_OK:成功 / ESP_FAIL:失败
 */
 esp_err_t sd_file_list_init(int need_count) {
    if (s_file_list.files != NULL) {
        sd_file_list_deinit(); // 释放旧内存
    }

    // 容量至少为1（避免0个文件时扩容失败），实际数量+2（留少量冗余）
    int init_count = need_count > 0 ? (need_count + 2) : 1;
    if (!sd_file_list_expand(init_count)) {
        ESP_LOGE(TAG, "文件列表初始化失败！需要容量：%d", init_count);
        return ESP_FAIL;
    }

    s_file_list.count = 0;
    ESP_LOGI(TAG, "文件列表初始化成功，容量：%d（实际文件数：%d）", init_count, need_count);
    return ESP_OK;
}
/**
 * @brief 遍历根目录，仅保存文件路径（排除文件夹）
 */
esp_err_t sd_scan_root_files_save_paths(void) {

    if (s_file_list.files == NULL) {
        ESP_LOGE(TAG, "请先调用 sd_file_list_init() 初始化文件列表！");
        return ESP_FAIL;
    }

    // 清空原有文件数据（保留数组容量）
    s_file_list.count = 0;

    // 打开SD卡根目录
    char root_path[64] = {0};
    snprintf(root_path, sizeof(root_path), "%s/", MOUNT_POINT);
    DIR *root_dir = opendir(root_path);
    if (root_dir == NULL) {
        ESP_LOGE(TAG, "打开SD卡根目录失败！路径：%s", root_path);
        ESP_LOGE(TAG, "检查：1.SD卡是否挂载 2.路径是否正确 3.SD卡是否格式化");
        return ESP_FAIL;
    }

    // 打印遍历日志头
    ESP_LOGI(TAG, "==================== SD卡根目录文件 ====================");
    ESP_LOGI(TAG, "根目录路径：%s", root_path);
    ESP_LOGI(TAG, "---------------------------------------------------------");

    // 遍历目录项，仅处理普通文件
    struct dirent *entry = NULL;
    while ((entry = readdir(root_dir)) != NULL) {
        // 跳过.、.. 以及文件夹
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (entry->d_type != DT_REG) continue; // 只保留普通文件

        // 扩容检查
        if (!sd_file_list_expand(s_file_list.count + 1)) {
            closedir(root_dir);
            return ESP_FAIL;
        }

        // 填充文件信息
        sd_file_info_t *file = &s_file_list.files[s_file_list.count];
        // 保存文件名
        strncpy(file->name, entry->d_name, sizeof(file->name) - 1);
        file->name[sizeof(file->name) - 1] = '\0';
        // 保存完整路径
        snprintf(file->full_path, sizeof(file->full_path), "%s/%s", MOUNT_POINT, entry->d_name);

        // 打印文件信息
        ESP_LOGI(TAG, "[%02d] 📄 文件 | 名称：%s | 完整路径：%s", 
                 s_file_list.count + 1, file->name, file->full_path);

        s_file_list.count++; // 文件计数+1
    }

    // 打印遍历结果
    ESP_LOGI(TAG, "---------------------------------------------------------");
    if (s_file_list.count == 0) {
        ESP_LOGW(TAG, "根目录中未找到任何文件！");
    } else {
        ESP_LOGI(TAG, "遍历完成，共保存 %d 个文件路径", s_file_list.count);
    }
    ESP_LOGI(TAG, "=========================================================");

    // 关闭目录句柄
    closedir(root_dir);
    return ESP_OK;
}

/**
 * @brief 获取文件列表指针
 */
sd_file_list_t* sd_get_file_list(void) {
    if (s_file_list.files == NULL) {
        ESP_LOGW(TAG, "文件列表未初始化");
        return NULL;
    }
    return &s_file_list;
}

/**
 * @brief 释放文件列表内存
 */
void sd_file_list_deinit(void) {
    if (s_file_list.files != NULL) {
        free(s_file_list.files);
        s_file_list.files = NULL;
        s_file_list.count = 0;
        s_file_list.max_count = 0;
        ESP_LOGI(TAG, "文件列表内存已释放");
    }
}






/**
 * @brief 预扫描SD卡根目录，仅统计文件数量（排除文件夹，静态内部使用）
 * @return int 成功返回文件数量，失败返回-1
 */
static int sd_scan_file_count(void) {
    int file_count = 0;
    char root_path[64] = {0};
    snprintf(root_path, sizeof(root_path), "%s/", MOUNT_POINT);

    // 打开SD卡根目录
    DIR *root_dir = opendir(root_path);
    if (root_dir == NULL) {
        ESP_LOGE(TAG, "预扫描：打开SD卡根目录失败！路径：%s", root_path);
        return -1;
    }

    // 遍历目录，仅统计普通文件数量
    struct dirent *entry = NULL;
    while ((entry = readdir(root_dir)) != NULL) {
        // 跳过.、.. 以及文件夹
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (entry->d_type == DT_REG) { // 仅统计普通文件
            file_count++;
        }
    }

    closedir(root_dir);
    ESP_LOGI(TAG, "预扫描完成：SD卡根目录共有 %d 个文件", file_count);
    return file_count;
}

/**
 * @brief 定向扫描指定目录的指定后缀文件
 * @param dir_path 目标目录，如 "/sdcard/novels"
 * @param filter_ext 后缀过滤，如 ".txt"（包含点号，NULL表示不过滤）
 *                   支持组合如 ".png|.jpg"
 * @return ESP_OK成功，其他失败
 */
esp_err_t sd_scan_target_dir(const char* dir_path, const char* filter_ext)
{
    if (s_file_list.files == NULL) return ESP_FAIL;

    s_file_list.count = 0; // 清空当前列表

    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        ESP_LOGE(TAG, "打开目录失败: %s", dir_path);
        return ESP_FAIL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) continue; // 只处理普通文件

        // 后缀过滤（支持 ".png|.jpg" 组合，用 strcasestr 忽略大小写）
        if (filter_ext != NULL) {
            const char *ext = strrchr(entry->d_name, '.');
            if (ext == NULL) continue;
            if (strcasestr(filter_ext, ext) == NULL) continue;
        }

        // 扩容并保存
        if (sd_file_list_expand(s_file_list.count + 1)) {
            sd_file_info_t *file = &s_file_list.files[s_file_list.count];
            strncpy(file->name, entry->d_name, sizeof(file->name) - 1);
            // 关键：full_path 直接带子目录
            snprintf(file->full_path, sizeof(file->full_path), "%s/%s", dir_path, entry->d_name);
            s_file_list.count++;
        }
    }
    closedir(dir);
    ESP_LOGI(TAG, "扫描 %s 完成，找到 %d 个文件", dir_path, s_file_list.count);
    return ESP_OK;
}
