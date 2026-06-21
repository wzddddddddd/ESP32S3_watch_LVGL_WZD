#include <lvgl.h>
#include "fullscreen_interfaces.h"

void fs_create_transfer(lv_obj_t* container) {
    // 创建标题
    lv_obj_t* title = lv_label_create(container);
    lv_label_set_text(title, "文件传输");
    lv_obj_set_style_text_font(title, &chinese_24, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // 创建返回按钮
    lv_obj_t* btn = lv_btn_create(container);
    lv_obj_set_size(btn, 100, 40);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    lv_obj_t* btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "");
    lv_obj_center(btn_label);
    
    // 返回按钮点击事件
    lv_obj_add_event_cb(btn, [](lv_event_t* e) {
        fs_do_adsorb();
    }, LV_EVENT_CLICKED, NULL);
    
    // 文件传输内容占位
    lv_obj_t* content = lv_label_create(container);
    lv_label_set_text(content, "");
    lv_obj_set_style_text_align(content, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(content);
}