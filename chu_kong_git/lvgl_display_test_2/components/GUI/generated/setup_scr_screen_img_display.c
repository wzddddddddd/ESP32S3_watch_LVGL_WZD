#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "img_display.h"

// 外部变量：当前选中的图片路径
extern char img_full_path[128];

void setup_scr_screen_img_display(lv_ui *ui)
{
    // 创建界面
    ui->screen_img_display = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_img_display, 240, 284);
    lv_obj_set_scrollbar_mode(ui->screen_img_display, LV_SCROLLBAR_MODE_OFF);

     lv_obj_clear_flag(ui->screen_img_display, LV_OBJ_FLAG_SCROLLABLE);

    // 黑色背景
    lv_obj_set_style_bg_opa(ui->screen_img_display, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_img_display, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);

// ★★★ 关键：传入 screen_img_display 作为父对象 ★★★
    if (strlen(img_full_path) > 0) {
        show_png_fast(img_full_path, ui->screen_img_display);
    }

    lv_obj_update_layout(ui->screen_img_display);
    events_init_screen_img_display(ui);
}