#include <lvgl.h>
#include "fullscreen_interfaces.h"
#include <LittleFS.h>
#include <time.h>
#include <Arduino.h>

// 静态变量声明
static lv_obj_t* bg_img = nullptr;
static lv_obj_t* time_label = nullptr;        // 合并的时间标签
static lv_obj_t* separator_label = nullptr;   // 分隔符标签
static lv_obj_t* date_label = nullptr;        // 日期标签
static lv_timer_t* time_timer = nullptr;
static uint8_t* wallpaper_data = nullptr;
static lv_obj_t* battery_bar = nullptr;   // 电量条
static lv_obj_t* battery_area = nullptr;  // 电池外框
static lv_obj_t* battery_percent_label = nullptr;
static lv_obj_t* battery_cont = nullptr;  // 电池容器

// 手势相关变量
static lv_point_t gesture_start;
static bool gesture_active = false;
static const int16_t SWIPE_THRESHOLD = 50;

// 关机动画相关
static bool shutting_down = false;                // 是否正在执行关机动画
static lv_timer_t* shutdown_timer = nullptr;      // 关机动画定时器

// 屏幕尺寸常量
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 280

// Y坐标常量（X坐标将使用居中对齐）
#define TIME_POS_Y 80        // 时间标签Y坐标
#define SEPARATOR_POS_Y 125  // 分隔符Y坐标
#define DATE_POS_Y 140       // 日期标签Y坐标

#define DETECT_PIN 6      
#define CONTROL_PIN 8     
static lv_timer_t* detect_timer = nullptr;  // io定时器
static lv_timer_t* time_sleep_timer = nullptr; // 时钟界面休眠定时器

static void time_sleep_timer_cb(lv_timer_t* timer) {
    sleep();
}

// 星期几的中文表示
static const char* weekdays_cn[] = {
    "星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"
};

// 更新显示
static void update_time_display() {
    // 获取当前标准时间
    time_t now = time(nullptr);
    
    // 转换为 GMT 时间结构体
    struct tm *ptm = gmtime(&now);
    
    // 防止 ptm 为空指针
    if (ptm == nullptr) {
        // 时间未就绪时显示默认值
        if (time_label) lv_label_set_text(time_label, "00:00");
        if (date_label) lv_label_set_text(date_label, "1/1  星期日");
        return;
    }

    // 提取时、分
    int hours = ptm->tm_hour;
    int minutes = ptm->tm_min;
    int month = ptm->tm_mon + 1;
    int day = ptm->tm_mday;
    int weekday = ptm->tm_wday;

    // 格式化时间显示（小时:分钟）
    char time_buf[10];
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d", hours, minutes);
    
    // 格式化日期显示（月/日   星期几）
    char date_buf[30];
    snprintf(date_buf, sizeof(date_buf), "%d/%d  %s", month, day, weekdays_cn[weekday]);

    if (time_label) lv_label_set_text(time_label, time_buf);
    if (date_label) lv_label_set_text(date_label, date_buf);

    // 更新电池显示
    if (battery_bar) {
        int pct = atomic_load_int(&battery_percentage); 
        pct = constrain(pct, 0, 100);
        int bar_width = (pct * 22) / 100;            
        lv_obj_set_width(battery_bar, bar_width);

        lv_color_t bat_color;
        if (pct <= 20)      bat_color = lv_palette_main(LV_PALETTE_RED);
        else if (pct <= 30) bat_color = lv_palette_main(LV_PALETTE_AMBER);
        else                bat_color = lv_palette_main(LV_PALETTE_GREEN);
        lv_obj_set_style_bg_color(battery_bar, bat_color, LV_PART_MAIN);
    }
        // 更新百分比标签
    if (battery_percent_label) {
        int pct = atomic_load_int(&battery_percentage);
        pct = constrain(pct, 0, 100);
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", pct);
        lv_label_set_text(battery_percent_label, buf);
    }
}
static void shutdown_anim_cb(lv_timer_t* timer) {
    static int brightness = -1; 
    if (brightness == -1) {
        // 从全局变量获取起始亮度
        brightness = atomic_load_int(&current_brightness);
    }
    
    brightness -= 5;
    if (brightness <= 0) {
        brightness = 0;
        display.setBrightness(0);
        atomic_store_int(&current_brightness, 0);
        // 停止定时器并休眠
        lv_timer_del(shutdown_timer);
        shutdown_timer = nullptr;
        shutting_down = false;
        sleep();   
    } else {
        display.setBrightness(brightness);
        atomic_store_int(&current_brightness, brightness);
    }
}
// 定时器回调
static void time_timer_cb(lv_timer_t * timer) {
    update_time_display();
}
// 检测引脚状态定时器回调
static void detect_timer_cb(lv_timer_t * timer) {
    static bool last_state = LOW;
    bool current_state = digitalRead(DETECT_PIN);
    
    // 检测下降沿（高->低）
    if (current_state == LOW && last_state == HIGH) {
        // 如果已经在关机动画中，忽略本次触发
        if (shutting_down) {
            last_state = current_state;
            return;
        }
        
        // 设置退出标志，让启动任务立即退出
        atomic_store_int(&slow_start_exit_flag, 1);
        
        // 获取当前亮度作为起始亮度
        int start_brightness = atomic_load_int(&current_brightness);
        
        //关机动画定时器
        shutting_down = true;
        shutdown_timer = lv_timer_create(shutdown_anim_cb, 20, NULL);
    }
    
    last_state = current_state;
}
// 清理资源
static void cleanup_time_resources() {
    if (time_timer) {
        lv_timer_del(time_timer);
        time_timer = nullptr;
    }
    if (detect_timer) {
        lv_timer_del(detect_timer);
        detect_timer = nullptr;
    }
    if (time_sleep_timer) {
        lv_timer_del(time_sleep_timer);
        time_sleep_timer = nullptr;
    }
    if (wallpaper_data) {
        free(wallpaper_data);
        wallpaper_data = nullptr;
    }
    if (shutdown_timer) {
        lv_timer_del(shutdown_timer);
        shutdown_timer = nullptr;
    }
    if (bg_img) {
        lv_obj_del(bg_img);
        bg_img = nullptr;
    }
    if (time_label) {
        lv_obj_del(time_label);
        time_label = nullptr;
    }
    if (separator_label) {
        lv_obj_del(separator_label);
        separator_label = nullptr;
    }
    if (date_label) {
        lv_obj_del(date_label);
        date_label = nullptr;
    }
    if (battery_bar) {
        lv_obj_del(battery_bar);
        battery_bar = nullptr;
    }
    if (battery_area) {
        lv_obj_del(battery_area);
        battery_area = nullptr;
    }
    if (battery_percent_label) {
        lv_obj_del(battery_percent_label);
        battery_percent_label = nullptr;
    }
    if (battery_cont) {
        lv_obj_del(battery_cont);
        battery_cont = nullptr;
    }
}

// 触摸事件回调
static void touch_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t* indev = lv_indev_get_act();
    
    if (code == LV_EVENT_PRESSED) {
        lv_indev_get_point(indev, &gesture_start);
        gesture_active = true;
    }
    else if (code == LV_EVENT_RELEASED) {
        if (!gesture_active) return;
        
        lv_point_t gesture_end;
        lv_indev_get_point(indev, &gesture_end);
        
        int16_t dy = gesture_end.y - gesture_start.y;
        
        if (dy > SWIPE_THRESHOLD) {
            fs_do_adsorb();
            cleanup_time_resources();
            
        }
        
        gesture_active = false;
    }
    else if (code == LV_EVENT_SCROLL) {
        lv_point_t vect;
        lv_indev_get_vect(indev, &vect);
        
        if (vect.y > SWIPE_THRESHOLD) {
            fs_do_adsorb();
            cleanup_time_resources();
            
        }
    }
}

void fs_create_time(lv_obj_t* container) {
    // 设置容器样式
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    
    // 创建背景图片
    bg_img = lv_img_create(container);
    lv_obj_set_size(bg_img, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_align(bg_img, LV_ALIGN_CENTER, 0, 0);
    
    // 尝试从LittleFS加载壁纸
    bool wallpaper_loaded = false;
    if (LittleFS.begin(true, "/littlefs", 10, "littlefs")) {
        if (LittleFS.exists("/wallpaper.bin")) {
            static lv_img_dsc_t img_dsc;
            img_dsc.header.w = SCREEN_WIDTH;
            img_dsc.header.h = SCREEN_HEIGHT;
            img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR; 
            
            File file = LittleFS.open("/wallpaper.bin", FILE_READ);
            if (file) {
                size_t fileSize = file.size();
                size_t expected_size = SCREEN_WIDTH * SCREEN_HEIGHT * 2;
                if (fileSize >= expected_size) {
                    wallpaper_data = (uint8_t*)malloc(expected_size);
                    if (wallpaper_data) {
                        file.read(wallpaper_data, expected_size);
                        img_dsc.data = wallpaper_data;
                        img_dsc.data_size = expected_size;
                        // 8.3.11 使用 lv_img_set_src 设置图像源
                        lv_img_set_src(bg_img, &img_dsc);
                        wallpaper_loaded = true;
                    }
                }
                file.close();
            }
        }
        LittleFS.end();
    }
        
    // 如果壁纸加载失败，使用纯白背景
    if (!wallpaper_loaded) {
        lv_obj_set_style_bg_color(bg_img, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(bg_img, LV_OPA_COVER, LV_STATE_DEFAULT);
    }
    
    // 创建时间标签
    time_label = lv_label_create(container);
    lv_label_set_text(time_label, "00:00");
    lv_obj_set_style_text_font(time_label, &time_70, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(time_label, lv_color_black(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_align(time_label, LV_ALIGN_TOP_MID, 0, TIME_POS_Y);
    
    // 创建分隔符标签
    separator_label = lv_label_create(container);
    lv_label_set_text(separator_label, "------------");
    lv_obj_set_style_text_font(separator_label, &chinese_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(separator_label, lv_color_black(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(separator_label, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_align(separator_label, LV_ALIGN_TOP_MID, 0, SEPARATOR_POS_Y);
    
    // 创建日期标签
    date_label = lv_label_create(container);
    lv_label_set_text(date_label, "1/1  星期日");
    lv_obj_set_style_text_font(date_label, &chinese_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(date_label, lv_color_black(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(date_label, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_align(date_label, LV_ALIGN_TOP_MID, 0, DATE_POS_Y);

    // 创建电池容器（水平排列百分比和图标）
battery_cont = lv_obj_create(container);
lv_obj_set_size(battery_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);  // 尺寸由内容决定
lv_obj_align(battery_cont, LV_ALIGN_TOP_MID, 0, -5);              // 整体居中，y=5
lv_obj_set_style_bg_opa(battery_cont, LV_OPA_TRANSP, 0);
lv_obj_set_style_border_width(battery_cont, 0, 0);
lv_obj_set_style_pad_all(battery_cont, 0, 0);
lv_obj_set_flex_flow(battery_cont, LV_FLEX_FLOW_ROW);             // 水平排列
lv_obj_set_style_flex_cross_place(battery_cont, LV_FLEX_ALIGN_CENTER, 0); // 垂直居中
lv_obj_set_style_pad_column(battery_cont, 2, 0);                  // 间距2像素

// 创建百分比标签（放入容器）
battery_percent_label = lv_label_create(battery_cont);  
lv_obj_set_style_text_font(battery_percent_label, &lv_font_montserrat_12, LV_STATE_DEFAULT);
lv_obj_set_style_text_color(battery_percent_label, lv_color_make(200, 200, 200), LV_STATE_DEFAULT); // 根据背景色调整
lv_label_set_text(battery_percent_label, "100%");

// 创建电池外框（放入容器）
battery_area = lv_obj_create(battery_cont);
lv_obj_set_size(battery_area, 24, 12);
lv_obj_set_style_bg_opa(battery_area, LV_OPA_0, 0);
lv_obj_set_style_border_width(battery_area, 1, 0);
lv_obj_set_style_border_color(battery_area, lv_color_make(200, 200, 200), 0);  // 白色背景用黑边框
lv_obj_set_style_radius(battery_area, 1, 0);
lv_obj_set_style_pad_all(battery_area, 0, 0);
lv_obj_clear_flag(battery_area, LV_OBJ_FLAG_SCROLLABLE);

// 电池正极小凸起（仍创建在 container 上，但对齐到 battery_area）
lv_obj_t* battery_tip = lv_obj_create(container);
lv_obj_set_size(battery_tip, 2, 5);
lv_obj_align_to(battery_tip, battery_area, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
lv_obj_set_style_bg_color(battery_tip, lv_color_make(200, 200, 200), 0);
lv_obj_set_style_bg_opa(battery_tip, LV_OPA_COVER, 0);
lv_obj_set_style_border_width(battery_tip, 0, 0);
lv_obj_set_style_radius(battery_tip, 1, 0);

// 内部电量条（battery_area 的子对象）
battery_bar = lv_obj_create(battery_area);
lv_obj_set_size(battery_bar, 0, lv_pct(100));
lv_obj_set_align(battery_bar, LV_ALIGN_LEFT_MID);
lv_obj_set_style_border_width(battery_bar, 0, 0);
lv_obj_set_style_bg_opa(battery_bar, LV_OPA_COVER, 0);
lv_obj_set_style_radius(battery_bar, 0, 0);
lv_obj_clear_flag(battery_bar, LV_OBJ_FLAG_SCROLLABLE);
    
    // 初始化显示
    update_time_display();

    
    // 创建定时器
    time_timer = lv_timer_create(time_timer_cb, 1000, NULL);
    detect_timer = lv_timer_create(detect_timer_cb, 50, NULL);
    time_sleep_timer = lv_timer_create(time_sleep_timer_cb, 300000, NULL);
    
    // 添加触摸事件监听
    lv_obj_add_event_cb(container, touch_event_cb, LV_EVENT_ALL, NULL);
    
    // 添加手势提示标签
    lv_obj_t* hint_label = lv_label_create(container);
    lv_label_set_text(hint_label, "下拉返回");
    lv_obj_set_style_text_font(hint_label, &chinese_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(hint_label, lv_color_make(100, 100, 100), LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(hint_label, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_align(hint_label, LV_ALIGN_BOTTOM_MID, 0, 10); 
}