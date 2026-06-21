#ifndef IMG_DISPLAY_H
#define IMG_DISPLAY_H

#include "lvgl.h"
#include "esp_err.h"

/**
 * @brief 初始化自定义文件系统驱动（盘符 S: → /sdcard/）
 */
void my_fs_init(void);

/**
 * @brief 从 SD 卡加载 PNG 到内存并显示
 * @param sd_path  SD 卡文件路径，例如 "/sdcard/img_1.png"
 */
void show_png_fast(const char *sd_path, lv_obj_t *parent);
/**
 * @brief 获取当前图片对象指针
 * @return lv_obj_t* 图片对象，未创建时返回 NULL
 */
lv_obj_t *img_display_get_obj(void);

/**
 * @brief 释放图片资源（内存 + 对象）
 */
void img_display_free(void);

#endif /* IMG_DISPLAY_H */