#include "novel_progress.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_crc.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "PROGRESS";

// NVS 命名空间（最大15字节）
#define NVS_NAMESPACE  "novel_prog"

/**
 * @brief 将任意长度的文件路径（含中文）转成 15 字节以内的 NVS key
 * 
 * 原理：CRC32 哈希
 *   "三体：地球往事.txt"  →  CRC32  →  0xA3F1B2C4  →  "bk_A3F1B2C4"
 *   不管文件名多长、是否中文，输出永远是 11 字节，不超限
 *   同一个文件名每次计算结果都一样，所以能正确存取
 * 
 * @param filepath   输入：文件完整路径（如 "/sdcard/宿命之环.txt"）
 * @param key_out    输出：生成的短 key（调用者保证至少16字节）
 * @param key_size   key_out 缓冲区大小
 */
static void filepath_to_nvs_key(const char *filepath, char *key_out, size_t key_size)
{
    /*
     * esp_crc32_le(初始种子, 数据指针, 数据长度)
     * 
     * 举例过程：
     *   "/sdcard/宿命之环.txt" 在内存中是一串字节：
     *   2F 73 64 63 61 72 64 2F E5 AE BF E5 91 BD ...
     *              ↓
     *        CRC32 哈希运算
     *              ↓
     *        0x1B4C8A55  （一个固定的32位数字）
     *              ↓
     *        sprintf → "bk_1B4C8A55"  （11字节，安全！）
     */
    uint32_t hash = esp_crc32_le(0, (const uint8_t *)filepath, strlen(filepath));
    snprintf(key_out, key_size, "bk_%08lX", (unsigned long)hash);
}

/**
 * @brief 保存阅读进度
 */
esp_err_t novel_progress_save(const char *novel_path, long offset)
{
    // 1. 文件路径 → NVS短key
    char key[16] = {0};
    filepath_to_nvs_key(novel_path, key, sizeof(key));

    // 2. 打开NVS
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS打开失败: %s", esp_err_to_name(err));
        return err;
    }

    // 3. 写入偏移量（用 int32 存储，足够存 2GB 文件的偏移）
    err = nvs_set_i32(handle, key, (int32_t)offset);
    if (err == ESP_OK) {
        err = nvs_commit(handle);  // 必须 commit 才真正写入 Flash
    }

    nvs_close(handle);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✅ 保存进度: \"%s\" key[%s] offset=%ld", novel_path, key, offset);
    } else {
        ESP_LOGE(TAG, "保存进度失败: %s", esp_err_to_name(err));
    }
    return err;
}

/**
 * @brief 加载阅读进度
 */
esp_err_t novel_progress_load(const char *novel_path, long *offset)
{
    // 1. 文件路径 → NVS短key（和保存时用同一个函数，所以结果一定一样）
    char key[16] = {0};
    filepath_to_nvs_key(novel_path, key, sizeof(key));

    // 2. 打开NVS（只读模式）
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        *offset = 0;
        ESP_LOGW(TAG, "NVS打开失败，从头开始阅读");
        return err;
    }

    // 3. 读取偏移量
    int32_t stored_offset = 0;
    err = nvs_get_i32(handle, key, &stored_offset);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // 没有保存过这本书的记录 → 从头开始
        *offset = 0;
        ESP_LOGI(TAG, "📖 首次阅读: \"%s\"，从头开始", novel_path);
    } else if (err == ESP_OK) {
        *offset = (long)stored_offset;
        ESP_LOGI(TAG, "📖 恢复进度: \"%s\" key[%s] offset=%ld", novel_path, key, *offset);
    } else {
        *offset = 0;
        ESP_LOGE(TAG, "读取进度失败: %s", esp_err_to_name(err));
    }

    nvs_close(handle);
    return err;
}

/**
 * @brief 删除阅读记录
 */
esp_err_t novel_progress_delete(const char *novel_path)
{
    char key[16] = {0};
    filepath_to_nvs_key(novel_path, key, sizeof(key));

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_erase_key(handle, key);
    if (err == ESP_OK) {
        nvs_commit(handle);
        ESP_LOGI(TAG, "🗑️ 已删除进度: \"%s\"", novel_path);
    }

    nvs_close(handle);
    return err;
}