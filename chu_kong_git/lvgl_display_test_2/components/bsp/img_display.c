#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "img_display.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"

#define TAG  "IMG"

static lv_obj_t *g_img       = NULL;
static uint8_t  *img_buf     = NULL;
static lv_img_dsc_t img_dsc;

// ==================== 自定义文件系统驱动 ====================

static void *my_fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    (void)drv;
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "/sdcard/%s", path);
    return fopen(full_path, (mode == LV_FS_MODE_WR) ? "wb" : "rb");
}

static lv_fs_res_t my_fs_close(lv_fs_drv_t *drv, void *file_p)
{
    (void)drv;
    fclose((FILE *)file_p);
    return LV_FS_RES_OK;
}

static lv_fs_res_t my_fs_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br)
{
    (void)drv;
    *br = fread(buf, 1, btr, (FILE *)file_p);
    return LV_FS_RES_OK;
}

static lv_fs_res_t my_fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence)
{
    (void)drv;
    int w = SEEK_SET;
    if (whence == LV_FS_SEEK_CUR) w = SEEK_CUR;
    else if (whence == LV_FS_SEEK_END) w = SEEK_END;
    fseek((FILE *)file_p, (long)pos, w);
    return LV_FS_RES_OK;
}

static lv_fs_res_t my_fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
    (void)drv;
    *pos_p = (uint32_t)ftell((FILE *)file_p);
    return LV_FS_RES_OK;
}

void my_fs_init(void)
{
    static lv_fs_drv_t fs_drv;
    lv_fs_drv_init(&fs_drv);
    fs_drv.letter   = 'S';
    fs_drv.open_cb  = my_fs_open;
    fs_drv.close_cb = my_fs_close;
    fs_drv.read_cb  = my_fs_read;
    fs_drv.seek_cb  = my_fs_seek;
    fs_drv.tell_cb  = my_fs_tell;
    lv_fs_drv_register(&fs_drv);
    ESP_LOGI(TAG, "文件系统驱动注册完成，盘符: S");
}

// ==================== PNG 快速显示 ====================

// ★★★ 加了 parent 参数 ★★★
void show_png_fast(const char *sd_path, lv_obj_t *parent)
{
    int64_t t0 = esp_timer_get_time();

    // 第1步：清除旧图片（只清内存，不删对象）
    // ★★★ 不再调用 lv_obj_del，因为界面切换时 auto_del 会自动删 ★★★
    if (g_img != NULL) {
        lv_img_cache_invalidate_src(&img_dsc);
        // 只有对象还有效时才删
        if (lv_obj_is_valid(g_img)) {
            lv_obj_del(g_img);
        }
        g_img = NULL;
    }

    // 第2步：打开文件
    FILE *f = fopen(sd_path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "打开失败: %s", sd_path);
        return;
    }

    // 第3步：获取大小
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    // 第4步：释放旧缓冲
    if (img_buf) {
        heap_caps_free(img_buf);
        img_buf = NULL;
    }

    // 第5步：分配内存（优先 PSRAM，没有则用普通内存）
    img_buf = heap_caps_malloc(fsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!img_buf) {
        // PSRAM 分配失败，尝试普通内存
        img_buf = malloc(fsize);
    }
    if (!img_buf) {
        ESP_LOGE(TAG, "内存分配失败！需要 %ld 字节", fsize);
        fclose(f);
        return;
    }

    int64_t t1 = esp_timer_get_time();

    // 第6步：一次性读入内存
    fread(img_buf, 1, fsize, f);
    fclose(f);

    int64_t t2 = esp_timer_get_time();

    // 第7步：构建图片描述符
    memset(&img_dsc, 0, sizeof(img_dsc));
    img_dsc.header.cf   = LV_IMG_CF_RAW_ALPHA;
    img_dsc.data_size   = fsize;
    img_dsc.data        = img_buf;

    // ★★★ 第8步：在指定的父对象上创建图片 ★★★
    g_img = lv_img_create(parent);     // ← 用传入的 parent，不用 lv_scr_act()
    lv_img_set_src(g_img, &img_dsc);
    lv_obj_align(g_img, LV_ALIGN_CENTER, 0, 0);

     // ★★★ 禁止图片被拖动和点击 ★★★
    lv_obj_clear_flag(g_img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(g_img, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(g_img, LV_OBJ_FLAG_EVENT_BUBBLE);   // 事件冒泡给父对象处理
    
    int64_t t3 = esp_timer_get_time();

    ESP_LOGI(TAG, "--- 图片加载耗时 ---");
    ESP_LOGI(TAG, "打开+分配: %lld ms", (t1 - t0) / 1000);
    ESP_LOGI(TAG, "SD卡读取:  %lld ms", (t2 - t1) / 1000);
    ESP_LOGI(TAG, "创建对象:  %lld ms", (t3 - t2) / 1000);
    ESP_LOGI(TAG, "总计:      %lld ms", (t3 - t0) / 1000);
}


lv_obj_t *img_display_get_obj(void)
{
    return g_img;
}

void img_display_free(void)
{
    // ★★★ 只释放内存，不删 LVGL 对象 ★★★
    // 因为界面切换时 auto_del=true 已经自动删除了
    if (g_img != NULL) {
        lv_img_cache_invalidate_src(&img_dsc);
        g_img = NULL;    // 只清指针，不调 lv_obj_del
    }
    if (img_buf) {
        heap_caps_free(img_buf);
        img_buf = NULL;
    }
    ESP_LOGI(TAG, "图片资源已释放");
}