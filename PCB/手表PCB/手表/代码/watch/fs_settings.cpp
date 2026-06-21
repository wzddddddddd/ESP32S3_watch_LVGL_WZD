#include <lvgl.h>
#include "fullscreen_interfaces.h"
#include "SDScan.h"
#include "ImageProcessor.h"
#include <LittleFS.h>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFi.h>
#include <Preferences.h>
#include <vector>
#include <WiFiUdp.h>
#include <NTPClient.h>

LV_FONT_DECLARE(chinese_24);
extern SdFs sd;

static WiFiUDP ntpUDP;
static NTPClient timeClient(ntpUDP, "ntp.aliyun.com", 28800, 60000); 


// 电池相关常量
#define BATTERY_ADC_PIN         4
#define VOLTAGE_DIVIDER_RATIO   2.0f
#define BATTERY_EMPTY_V         3.3f
#define BATTERY_FULL_V          4.2f

// 状态枚举与全局变量
enum task_state {
    STATE_IDLE,
    STATE_WAIT_ANIM,     // 等待动画结束
    STATE_SCANNING,      // 扫描SD卡
    STATE_DECODING,      // 解码中
    STATE_WRITING,       // 写入Flash中
    STATE_SUCCESS,       // 成功
    STATE_ERROR          // 失败
};
// 时间同步状态
static char time_sync_status[64] = {0};
static volatile bool time_sync_done = false;
static volatile bool time_sync_success = false;
static lv_timer_t* time_sync_timer = nullptr;
// 线程通信变量
static volatile task_state current_state = STATE_IDLE;
static volatile int task_progress = 0;
static char status_message[64] = {0};
static TaskHandle_t wp_task_handle = nullptr;

// UI 对象
static lv_obj_t* mainList = nullptr;
static lv_obj_t* subContainer = nullptr;
static lv_timer_t* ioTimer = nullptr;          // 物理按键检测
static lv_timer_t* uiMonitorTimer = nullptr;   // 壁纸任务UI刷新
static lv_obj_t* progressBar = nullptr;
static lv_obj_t* progressLabel = nullptr;

// ================= WiFi 全局变量 =================
static Preferences prefs;
static std::vector<String> scanned_ssid_list;
static bool wifi_scan_done = false;
static SemaphoreHandle_t scan_mutex = NULL;
static bool scanning = false;

// WiFi 配置页 UI 控件句柄
static lv_obj_t* ssid_dropdown = nullptr;
static lv_obj_t* slot_dropdown = nullptr;
static lv_obj_t* password_ta = nullptr;
static lv_obj_t* kb = nullptr;
static lv_timer_t* scan_check_timer = nullptr;
static lv_obj_t* wifiList = nullptr; 

// ================= 辅助函数声明 =================
static void on_decode_progress(int percent);
static void on_decode_error(ImageProcessor::ErrorCode error, const char* message);
static void set_status_msg(const char* msg);

// UI 动画与交互函数
static void anim_del_sub_obj_cb(lv_anim_t* a);
static void slide_animation(lv_obj_t* obj, int32_t start_y, int32_t end_y, lv_anim_ready_cb_t ready_cb);
static void create_sub_page_base();
static void cleanup_sub_page();
static void close_sub_page();

// 功能函数声明
static void show_time_sync_page();
static void show_wifi_page();                   
static void init_wallpaper_ui();
static void start_wallpaper_task();
static void show_wallpaper_page();
static void show_battery_calibration_page();
static void show_carousel_interval_page();
static void show_storage_page();
static void handle_menu_selection(const char* itemText);

// WiFi 相关新增函数声明
static void wifi_scan_task(void* param);
static void scan_check_timer_cb(lv_timer_t* timer);
static void kb_event_cb(lv_event_t* e);
static void ta_event_cb(lv_event_t* e);
static void save_wifi_config_cb(lv_event_t* e);
static void trigger_rescan_cb(lv_event_t* e);
static void config_btn_cb(lv_event_t* e);     


static bool scan_sd_card_for_wallpaper(String& imagePath);
static ImageProcessor::ErrorCode decode_image(const String& imagePath, uint16_t** outputBuffer, size_t* outputSize);
static bool write_to_littlefs(uint16_t* outputBuffer, size_t outputSize);
void wallpaper_worker_task(void* param);
static void wallpaper_ui_monitor_cb(lv_timer_t* timer);
static void handle_back_button();
static void io_poll_timer_cb(lv_timer_t* timer);
static void list_item_cb(lv_event_t* e);
// 时间同步相关函数
static void time_sync_wifi_btn_cb(lv_event_t* e);
static void time_sync_manual_btn_cb(lv_event_t* e);
static void wifi_ntp_sync_task(void* param);
static void time_sync_ui_monitor_cb(lv_timer_t* timer);
static void time_manual_set_btn_cb(lv_event_t* e);
//电池电压校准
static lv_timer_t* cal_timer = nullptr;
static lv_obj_t* voltage_label = nullptr;
static lv_obj_t* battery_percent_label = nullptr;   // 显示校准后的电量
static lv_obj_t* cal_slider = nullptr;
static lv_obj_t* cal_value_label = nullptr;         // 显示当前校准百分比

// 线程安全的进度回调
static void on_decode_progress(int percent) {
    task_progress = (int)(percent * 0.8);
    snprintf(status_message, sizeof(status_message), "图像解码中...%d%%", percent);
}

static void on_decode_error(ImageProcessor::ErrorCode error, const char* message) {
    snprintf(status_message, sizeof(status_message), "错误: %s", message);
}

static void set_status_msg(const char* msg) {
    strncpy(status_message, msg, sizeof(status_message) - 1);
    status_message[sizeof(status_message) - 1] = '\0';
}

// ================= UI 动画函数 =================
static void anim_del_sub_obj_cb(lv_anim_t* a) {
    if (subContainer) {
        lv_obj_del(subContainer);
        subContainer = nullptr;
    }
    progressBar = nullptr;
    progressLabel = nullptr;
    if (uiMonitorTimer) {
        lv_timer_del(uiMonitorTimer);
        uiMonitorTimer = nullptr;
    }
}

static void slide_animation(lv_obj_t* obj, int32_t start_y, int32_t end_y, lv_anim_ready_cb_t ready_cb) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, start_y, end_y);
    lv_anim_set_time(&a, 500);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    if (ready_cb) lv_anim_set_ready_cb(&a, ready_cb);
    lv_anim_start(&a);
}

static void create_sub_page_base() {
    if (subContainer) return;
    subContainer = lv_obj_create(lv_scr_act());
    lv_obj_set_size(subContainer, 240, 280);
    lv_obj_set_pos(subContainer, 0, 280);
    lv_obj_set_style_bg_color(subContainer, lv_color_hex(0x212121), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(subContainer, 0, 0);
    lv_obj_set_style_pad_all(subContainer, 0, 0);
    lv_obj_clear_flag(subContainer, LV_OBJ_FLAG_SCROLLABLE);
    slide_animation(subContainer, 280, 0, NULL);
}

static void cleanup_sub_page() {
    if (subContainer) {
        lv_obj_del(subContainer);
        subContainer = nullptr;
    }
    progressBar = nullptr;
    progressLabel = nullptr;
    if (uiMonitorTimer) {
        lv_timer_del(uiMonitorTimer);
        uiMonitorTimer = nullptr;
    }
}

static void close_sub_page() {
    if (subContainer && lv_obj_get_y(subContainer) == 0) {
        slide_animation(subContainer, 0, 280, anim_del_sub_obj_cb);
    } else {
        cleanup_sub_page();
    }
}

// ================= UI 监控定时器回调 =================
static void wallpaper_ui_monitor_cb(lv_timer_t* timer) {
    if (progressLabel && lv_obj_is_valid(progressLabel)) {
        lv_label_set_text(progressLabel, status_message);
    }
    if (progressBar && lv_obj_is_valid(progressBar)) {
        lv_bar_set_value(progressBar, task_progress, LV_ANIM_ON);
    }

    if (current_state == STATE_SUCCESS || current_state == STATE_ERROR) {
        static int end_delay = 0;
        end_delay++;
        if (end_delay > 40) {
            end_delay = 0;
            slide_animation(subContainer, 0, 280, anim_del_sub_obj_cb);
            lv_timer_del(timer);
            uiMonitorTimer = nullptr;
        }
    }
}

// ================= 返回按键逻辑 =================
static void handle_back_button() {
    // 检查当前是否处于敏感操作状态
    bool is_sensitive_operation = false;
    
    // 检查是否在 WiFi 时间同步过程中
    if (time_sync_timer != nullptr && !time_sync_done) {
        is_sensitive_operation = true;
        Serial.println("时间同步中，禁止退出");
    }
    
    // 检查是否在 WiFi 扫描过程中
    if (scanning || wifi_scan_done == false) {
        // 更精确的判断：如果扫描定时器存在且扫描未完成
        if (scan_check_timer != nullptr && !wifi_scan_done) {
            is_sensitive_operation = true;
            Serial.println("WiFi扫描中，禁止退出");
        }
    }
    
    // 检查是否在 WiFi 配置页面的扫描过程中
    if (ssid_dropdown && lv_obj_is_valid(ssid_dropdown)) {
        // 如果下拉框显示"扫描中..."，说明正在扫描
        char buf[32];
        lv_dropdown_get_selected_str(ssid_dropdown, buf, sizeof(buf));
        if (strcmp(buf, "扫描中...") == 0) {
            is_sensitive_operation = true;
            Serial.println("WiFi扫描中，禁止退出");
        }
    }
    
    // 如果在操作中，直接返回，不执行退出
    if (is_sensitive_operation) {
        return;
    }

    if (current_state != STATE_IDLE && current_state != STATE_SUCCESS && current_state != STATE_ERROR) {
        Serial.println("任务处理中，请稍候...");
        return;
    }

    if (subContainer && lv_obj_get_y(subContainer) == 0) {
        slide_animation(subContainer, 0, 280, anim_del_sub_obj_cb);
    } else {
        if (ioTimer) { lv_timer_del(ioTimer); ioTimer = nullptr; }
        if (subContainer) {
            lv_obj_del(subContainer);
            subContainer = nullptr;
        }
        if (uiMonitorTimer) {
            lv_timer_del(uiMonitorTimer);
            uiMonitorTimer = nullptr;
        }
        if (wp_task_handle) {
            vTaskDelete(wp_task_handle);
            wp_task_handle = nullptr;
        }
        fs_do_adsorb();  
    }
}

// ================= IO 按键轮询 =================
static void io_poll_timer_cb(lv_timer_t* timer) {
    static bool button_pressed = false;
    if (digitalRead(6) == HIGH) {
        if (!button_pressed) button_pressed = true;
    } else {
        if (button_pressed) {
            button_pressed = false;
            handle_back_button();
        }
    }
}

// ================= 主菜单列表项点击回调 =================
static void list_item_cb(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    const char* itemText = lv_list_get_btn_text(mainList, btn);
    handle_menu_selection(itemText);
}

// ================= 各菜单项实现 =================

// 时间同步
static void show_time_sync_page() {
    // 清除子页面原有内容（如果有）
    lv_obj_clean(subContainer);

    // 创建两个按钮
    lv_obj_t* btn_wifi = lv_btn_create(subContainer);
    lv_obj_set_size(btn_wifi, 200, 50);
    lv_obj_align(btn_wifi, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_bg_color(btn_wifi, lv_color_hex(0x3498db), 0);
    lv_obj_add_event_cb(btn_wifi, time_sync_wifi_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* label_wifi = lv_label_create(btn_wifi);
    lv_label_set_text(label_wifi, "NTP 同步");
    lv_obj_set_style_text_font(label_wifi, &chinese_24, 0);
    lv_obj_center(label_wifi);

    lv_obj_t* btn_manual = lv_btn_create(subContainer);
    lv_obj_set_size(btn_manual, 200, 50);
    lv_obj_align(btn_manual, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_color(btn_manual, lv_color_hex(0x2ecc71), 0);
    lv_obj_add_event_cb(btn_manual, time_sync_manual_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* label_manual = lv_label_create(btn_manual);
    lv_label_set_text(label_manual, "手动同步");
    lv_obj_set_style_text_font(label_manual, &chinese_24, 0);
    lv_obj_center(label_manual);
}

void wifi_ntp_sync_task(void* param) {
    time_sync_done = false;
    time_sync_success = false;
    strcpy(time_sync_status, "正在扫描 WiFi...");

    // 扫描 WiFi 网络
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    int n = WiFi.scanNetworks();
    if (n == 0) {
        strcpy(time_sync_status, "未扫描到任何 WiFi");
        time_sync_done = true;
        vTaskDelete(NULL);
        return;
    }

    // 读取保存的 5 个槽位
    Preferences prefs;
    prefs.begin("watch", true);
    String ssid_list[5];
    String pass_list[5];
    for (int i = 0; i < 5; i++) {
        char key_ssid[16], key_pass[16];
        snprintf(key_ssid, sizeof(key_ssid), "WiFiSSID%d", i+1);
        snprintf(key_pass, sizeof(key_pass), "WiFiPass%d", i+1);
        ssid_list[i] = prefs.getString(key_ssid, "");
        pass_list[i] = prefs.getString(key_pass, "");
    }
    prefs.end();

    // 按槽位顺序尝试连接
    bool connected = false;
    for (int i = 0; i < 5; i++) {
        if (ssid_list[i].isEmpty()) continue;

        // 检查扫描结果中是否包含此 SSID
        bool found = false;
        for (int j = 0; j < n; j++) {
            if (WiFi.SSID(j) == ssid_list[i]) {
                found = true;
                break;
            }
        }
        if (!found) continue;

        snprintf(time_sync_status, sizeof(time_sync_status), "正在连接 %s...", ssid_list[i].c_str());
        WiFi.begin(ssid_list[i].c_str(), pass_list[i].c_str());

        // 等待连接，超时 10 秒
        int timeout = 20; // 20 * 500ms = 10s
        while (--timeout > 0 && WiFi.status() != WL_CONNECTED) {
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        if (WiFi.status() == WL_CONNECTED) {
            connected = true;
            break;
        }
        WiFi.disconnect();
    }

    if (!connected) {
        strcpy(time_sync_status, "连接失败，无可用 WiFi");
        time_sync_done = true;
        vTaskDelete(NULL);
        return;
    }

    strcpy(time_sync_status, "WiFi 已连接，同步时间中...");

    // NTPClient 同步时间
    timeClient.begin();
    int retryCount = 0;
    const int MAX_RETRIES = 3;
    bool timeSynced = false;

    while (retryCount < MAX_RETRIES && !timeSynced) {
        if (timeClient.update()) {
            timeSynced = true;
            // 设置系统时间
            struct timeval tv;
            tv.tv_sec = timeClient.getEpochTime();
            tv.tv_usec = 0;
            settimeofday(&tv, NULL);
            strcpy(time_sync_status, "时间同步成功");
            time_sync_success = true;
        } else {
            retryCount++;
            snprintf(time_sync_status, sizeof(time_sync_status), "同步重试 %d/%d...", retryCount, MAX_RETRIES);
            vTaskDelay(pdMS_TO_TICKS(500)); // 等待0.5秒再重试
        }
    }

    if (!timeSynced) {
        strcpy(time_sync_status, "NTP 同步超时");
    }

    // 如果成功且 RTC 可用，同步到外部 RTC（添加重试机制）
    if (time_sync_success && rtcManager.isAvailable()) {
        strcpy(time_sync_status, "正在同步到 RTC...");
        
        const int RTC_RETRY_COUNT = 3;
        bool rtc_sync_success = false;
        
        for (int retry = 1; retry <= RTC_RETRY_COUNT; retry++) {
            if (retry > 1) {
                snprintf(time_sync_status, sizeof(time_sync_status), 
                         "RTC 同步重试 %d/%d...", retry, RTC_RETRY_COUNT);
                vTaskDelay(pdMS_TO_TICKS(200)); // 重试前等待200ms
            }
            
            if (rtcManager.syncFromSystem()) {
                rtc_sync_success = true;
                break;
            }
            
            // 重试前稍微延迟
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        if (rtc_sync_success) {
            strlcat(time_sync_status, " (RTC 同步成功)", sizeof(time_sync_status));
        } else {
            strlcat(time_sync_status, " (RTC 同步失败)", sizeof(time_sync_status));
            // 可以选择记录错误，但不影响主流程
            Serial.println("RTC sync failed after 3 attempts");
        }
    }

    time_sync_done = true;

    // 断开 WiFi 关闭射频
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    vTaskDelete(NULL);
}

static void time_sync_ui_monitor_cb(lv_timer_t* timer) {
    // 查找状态标签
    lv_obj_t* label = lv_obj_get_child(subContainer, 0);
    if (label && lv_obj_check_type(label, &lv_label_class)) {
        lv_label_set_text(label, time_sync_status);
    }

    if (time_sync_done) {
        // 任务完成，停止定时器
        lv_timer_del(time_sync_timer);
        time_sync_timer = nullptr;

        // 添加一个返回按钮
        lv_obj_t* btn_back = lv_btn_create(subContainer);
        lv_obj_set_size(btn_back, 100, 40);
        lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_set_style_bg_color(btn_back, lv_color_hex(0xe74c3c), 0);
        lv_obj_add_event_cb(btn_back, [](lv_event_t* e) {
            lv_obj_clean(subContainer);
            show_time_sync_page(); // 返回上一级
        }, LV_EVENT_CLICKED, NULL);
        lv_obj_t* label_back = lv_label_create(btn_back);
        lv_label_set_text(label_back, "返回");
        lv_obj_set_style_text_font(label_back, &chinese_24, 0);
        lv_obj_center(label_back);
    }
}
static void time_sync_wifi_btn_cb(lv_event_t* e) {
    // 清空子页面
    lv_obj_clean(subContainer);

    // 创建一个状态标签
    lv_obj_t* status_label = lv_label_create(subContainer);
    lv_label_set_text(status_label, "准备中...");
    lv_obj_set_style_text_font(status_label, &chinese_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(status_label, lv_color_white(), LV_STATE_DEFAULT);
    // 设置宽度并允许自动换行
    lv_obj_set_width(status_label, 220);
    lv_label_set_long_mode(status_label, LV_LABEL_LONG_WRAP);
    lv_obj_center(status_label);   // 居中后仍会在宽度内换行

    // 初始化状态变量
    time_sync_done = false;
    strcpy(time_sync_status, "初始化...");

    // 创建监控定时器
    if (time_sync_timer) lv_timer_del(time_sync_timer);
    time_sync_timer = lv_timer_create(time_sync_ui_monitor_cb, 100, NULL);

    // 启动任务
    xTaskCreatePinnedToCore(wifi_ntp_sync_task, "WiFiNTPSync", 8192, NULL, 1, NULL, 0);
}
static void time_sync_manual_btn_cb(lv_event_t* e) {
    lv_obj_clean(subContainer);

    // 获取当前时间作为默认值
    struct tm now;
    getLocalTime(&now);
    // 创建下拉框：年 (2020-2030)
    lv_obj_t* year_dd = lv_dropdown_create(subContainer);
    lv_dropdown_set_options(year_dd, "2020\n2021\n2022\n2023\n2024\n2025\n2026\n2027\n2028\n2029\n2030");
    lv_obj_set_size(year_dd, 80, 45);
    lv_obj_align(year_dd, LV_ALIGN_TOP_LEFT, 10, 20);
    lv_dropdown_set_selected(year_dd, now.tm_year + 1900 - 2020);
    lv_obj_set_style_text_font(year_dd, &chinese_24, 0);
    lv_obj_t* year_list = lv_dropdown_get_list(year_dd);
    lv_obj_set_style_text_font(year_list, &chinese_24, LV_PART_MAIN);
    lv_dropdown_set_symbol(year_dd, NULL);

    // 月 (1-12)
    lv_obj_t* month_dd = lv_dropdown_create(subContainer);
    lv_dropdown_set_options(month_dd, "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12");
    lv_obj_set_size(month_dd, 60, 45);
    lv_obj_align(month_dd, LV_ALIGN_TOP_LEFT, 100, 20);
    lv_dropdown_set_selected(month_dd, now.tm_mon); // tm_mon 0-11
    lv_obj_set_style_text_font(month_dd, &chinese_24, 0);
    lv_obj_t* month_list = lv_dropdown_get_list(month_dd);
    lv_obj_set_style_text_font(month_list, &chinese_24, LV_PART_MAIN);
    lv_dropdown_set_symbol(month_dd, NULL);

    // 日 (1-31)
    lv_obj_t* day_dd = lv_dropdown_create(subContainer);
    lv_dropdown_set_options(day_dd, "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31");
    lv_obj_set_size(day_dd, 60, 45);
    lv_obj_align(day_dd, LV_ALIGN_TOP_LEFT, 170, 20);
    lv_dropdown_set_selected(day_dd, now.tm_mday - 1);
    lv_obj_set_style_text_font(day_dd, &chinese_24, 0);
    lv_obj_t* day_list = lv_dropdown_get_list(day_dd);
    lv_obj_set_style_text_font(day_list, &chinese_24, LV_PART_MAIN);
    lv_dropdown_set_symbol(day_dd, NULL);

    // 时 (0-23)
    lv_obj_t* hour_dd = lv_dropdown_create(subContainer);
    lv_dropdown_set_options(hour_dd, "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23");
    lv_obj_set_size(hour_dd, 70, 45);
    lv_obj_align(hour_dd, LV_ALIGN_TOP_LEFT, 40, 80);
    lv_dropdown_set_selected(hour_dd, now.tm_hour);
    lv_obj_set_style_text_font(hour_dd, &chinese_24, 0);
    lv_obj_t* hour_list = lv_dropdown_get_list(hour_dd);
    lv_obj_set_style_text_font(hour_list, &chinese_24, LV_PART_MAIN);
    lv_dropdown_set_symbol(hour_dd, NULL);

    // 分 (0-59)
    lv_obj_t* min_dd = lv_dropdown_create(subContainer);
    lv_dropdown_set_options(min_dd, "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59");
    lv_obj_set_size(min_dd, 70, 45);
    lv_obj_align(min_dd, LV_ALIGN_TOP_LEFT, 130, 80);
    lv_dropdown_set_selected(min_dd, now.tm_min);
    lv_obj_set_style_text_font(min_dd, &chinese_24, 0);
    lv_obj_t* min_list = lv_dropdown_get_list(min_dd);
    lv_obj_set_style_text_font(min_list, &chinese_24, LV_PART_MAIN);
    lv_dropdown_set_symbol(min_dd, NULL);

    // 设置按钮
    lv_obj_t* set_btn = lv_btn_create(subContainer);
    lv_obj_set_size(set_btn, 120, 40);
    lv_obj_align(set_btn, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_obj_set_style_bg_color(set_btn, lv_color_hex(0x2ecc71), 0);
    lv_obj_add_event_cb(set_btn, time_manual_set_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* set_label = lv_label_create(set_btn);
    lv_label_set_text(set_label, "设置时间");
    lv_obj_set_style_text_font(set_label, &chinese_24, 0);
    lv_obj_center(set_label);

    // 返回按钮
    lv_obj_t* back_btn = lv_btn_create(subContainer);
    lv_obj_set_size(back_btn, 100, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0xe74c3c), 0);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        lv_obj_clean(subContainer);
        show_time_sync_page();
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "返回");
    lv_obj_set_style_text_font(back_label, &chinese_24, 0);
    lv_obj_center(back_label);
}
static void time_manual_set_btn_cb(lv_event_t* e) {
    // 获取子页面中所有的下拉框（按创建顺序：年、月、日、时、分）
    lv_obj_t* year_dd = lv_obj_get_child(subContainer, 0);
    lv_obj_t* month_dd = lv_obj_get_child(subContainer, 1);
    lv_obj_t* day_dd = lv_obj_get_child(subContainer, 2);
    lv_obj_t* hour_dd = lv_obj_get_child(subContainer, 3);
    lv_obj_t* min_dd = lv_obj_get_child(subContainer, 4);

    if (!year_dd || !month_dd || !day_dd || !hour_dd || !min_dd) return;

    int year = lv_dropdown_get_selected(year_dd) + 2020;
    int month = lv_dropdown_get_selected(month_dd) + 1; // 下拉索引0对应1月
    int day = lv_dropdown_get_selected(day_dd) + 1;
    int hour = lv_dropdown_get_selected(hour_dd);
    int minute = lv_dropdown_get_selected(min_dd);

    struct tm t = {0};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = 0;


    time_t t_sec = mktime(&t);
    struct timeval now = { .tv_sec = t_sec };
    settimeofday(&now, NULL);
    // 同步到外部 RTC
    if (rtcManager.isAvailable()) {
        rtcManager.syncFromSystem();
    }

    // 显示成功提示
    lv_obj_clean(subContainer);
    lv_obj_t* msg = lv_label_create(subContainer);
    lv_label_set_text(msg, "时间设置成功");
    lv_obj_set_style_text_font(msg, &chinese_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(msg, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_center(msg);

    // 添加返回按钮
    lv_obj_t* back_btn = lv_btn_create(subContainer);
    lv_obj_set_size(back_btn, 100, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0xe74c3c), 0);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        lv_obj_clean(subContainer);
        show_time_sync_page();
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "返回");
    lv_obj_set_style_text_font(back_label, &chinese_24, 0);
    lv_obj_center(back_label);
}
//WiFi 列表项点击回调
static void wifi_item_cb(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    const char* wifiName = lv_list_get_btn_text(wifiList, btn);
    
    // 获取当前点击的是第几个按钮
    int index = lv_obj_get_index(btn); 
    
    String saved_pass = "未设置";

    if (index >= 1 && index <= 5) {
        prefs.begin("watch", true);
        char key_pass[16];
        snprintf(key_pass, sizeof(key_pass), "WiFiPass%d", index); 
        saved_pass = prefs.getString(key_pass, "无密码");
        prefs.end();
    }

    static const char* btns[] = {"确定", ""};
    char msg[128];
    // 使用从 Preferences 读取到的 saved_pass
    snprintf(msg, sizeof(msg), "SSID: %s\n密码: %s", 
             wifiName ? wifiName : "Unknown", 
             saved_pass.c_str());

    lv_obj_t* mbox = lv_msgbox_create(subContainer, "WiFi 信息", msg, btns, true);
    lv_obj_set_style_text_font(mbox, &chinese_24, 0);
    lv_obj_center(mbox);
    
    lv_obj_add_event_cb(mbox, [](lv_event_t* ev) {
        lv_msgbox_close(lv_event_get_current_target(ev));
    }, LV_EVENT_VALUE_CHANGED, NULL);
}

// ================= WiFi 扫描任务 =================
void wifi_scan_task(void* param) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    int n = WiFi.scanNetworks();

    if (scan_mutex == NULL) scan_mutex = xSemaphoreCreateMutex();

    xSemaphoreTake(scan_mutex, portMAX_DELAY);
    scanned_ssid_list.clear();
    for (int i = 0; i < n; ++i) {
        scanned_ssid_list.push_back(WiFi.SSID(i));
    }
    xSemaphoreGive(scan_mutex);

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    wifi_scan_done = true;
    scanning = false; 
    vTaskDelete(NULL);
}

// 扫描检查定时器回调
static void scan_check_timer_cb(lv_timer_t* timer) {
    if (wifi_scan_done && ssid_dropdown && lv_obj_is_valid(ssid_dropdown)) {
        String options = "";
        xSemaphoreTake(scan_mutex, portMAX_DELAY);
        if (scanned_ssid_list.empty()) {
            options = "未扫描到网络";
        } else {
            for (const auto& ssid : scanned_ssid_list) {
                options += ssid + "\n";
            }
            if (options.length() > 0) options.remove(options.length() - 1);
        }
        xSemaphoreGive(scan_mutex);
        lv_dropdown_set_options(ssid_dropdown, options.c_str());
        lv_dropdown_set_text(ssid_dropdown, NULL); 
        lv_timer_del(timer);
        scan_check_timer = nullptr;
        scanning = false;
        
        wifi_scan_done = true;
    }
}

// 键盘事件回调
static void kb_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        if (code == LV_EVENT_READY) {
            lv_obj_clear_state(password_ta, LV_STATE_FOCUSED);
        }
    }
}

// 文本框聚焦回调
static void ta_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_FOCUSED) {
        if (kb) {
            lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(kb);
        }
    }
}

// 保存配置回调
static void save_wifi_config_cb(lv_event_t* e) {
    if (!slot_dropdown || !ssid_dropdown || !password_ta) return;

    char buf[64];
    int slot_index = lv_dropdown_get_selected(slot_dropdown);
    int slot_num = slot_index + 1;   // 1~5

    lv_dropdown_get_selected_str(ssid_dropdown, buf, sizeof(buf));
    String target_ssid = String(buf);
    String target_pass = String(lv_textarea_get_text(password_ta));

    if (target_ssid.isEmpty() || target_ssid == "扫描中..." || target_ssid == "未扫描到网络") {
        lv_obj_t* mbox = lv_msgbox_create(subContainer, "错误", "请选择有效的 WiFi", NULL, true);
        lv_obj_set_style_text_font(mbox, &chinese_24, 0);
        lv_obj_center(mbox);
        lv_obj_add_event_cb(mbox, [](lv_event_t* ev){ lv_msgbox_close(lv_event_get_current_target(ev)); }, LV_EVENT_VALUE_CHANGED, NULL);
        return;
    }

    prefs.begin("watch", false);
    char key_ssid[16], key_pass[16];
    snprintf(key_ssid, sizeof(key_ssid), "WiFiSSID%d", slot_num);
    snprintf(key_pass, sizeof(key_pass), "WiFiPass%d", slot_num);
    prefs.putString(key_ssid, target_ssid);
    prefs.putString(key_pass, target_pass);
    prefs.end();

    lv_obj_clean(subContainer);
    show_wifi_page();   // 重新显示列表页
}

// 触发重新扫描
static void trigger_rescan_cb(lv_event_t* e) {
    if (scanning) return;
    scanning = true;
    wifi_scan_done = false;  // 重置完成标志
    
    lv_dropdown_set_options(ssid_dropdown, "扫描中...");
    lv_dropdown_set_text(ssid_dropdown, "扫描中...");
    if (scan_check_timer) lv_timer_del(scan_check_timer);
    scan_check_timer = lv_timer_create(scan_check_timer_cb, 200, NULL);
    xTaskCreatePinnedToCore(wifi_scan_task, "WiFiScan", 4096, NULL, 1, NULL, 0);
}

// 配置 WiFi 按钮回调
static void config_btn_cb(lv_event_t* e) {
    lv_obj_clean(subContainer);

    lv_obj_t* config_page = lv_obj_create(subContainer);
    lv_obj_set_size(config_page, 240, 280);
    lv_obj_set_style_bg_color(config_page, lv_color_hex(0x212121), 0);
    lv_obj_set_style_border_width(config_page, 0, 0);
    lv_obj_set_style_pad_all(config_page, 0, 0);
    lv_obj_center(config_page);

    //槽位下拉框
    slot_dropdown = lv_dropdown_create(config_page);
    lv_dropdown_set_options(slot_dropdown, "WiFi 槽位 1\nWiFi 槽位 2\nWiFi 槽位 3\nWiFi 槽位 4\nWiFi 槽位 5");
    lv_obj_set_width(slot_dropdown, 150);
    lv_obj_align(slot_dropdown, LV_ALIGN_TOP_RIGHT, -20, 10);
    lv_obj_set_style_text_font(slot_dropdown, &chinese_24, 0); 

    lv_obj_t* slot_list = lv_dropdown_get_list(slot_dropdown); 
    lv_obj_set_style_text_font(slot_list, &chinese_24, LV_PART_MAIN);
    lv_dropdown_set_symbol(slot_dropdown, NULL);

    lv_obj_t* label = lv_label_create(config_page);
    lv_label_set_text(label, "覆盖:");
    lv_obj_set_style_text_font(label, &chinese_24, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 20);

    ssid_dropdown = lv_dropdown_create(config_page);
    lv_obj_set_size(ssid_dropdown, 220, 50);
    lv_obj_align(ssid_dropdown, LV_ALIGN_TOP_MID, 0, 60);
    lv_dropdown_set_options(ssid_dropdown, "点击扫描按钮获取 WiFi 列表");
    lv_dropdown_set_text(ssid_dropdown, "请先扫描 WiFi");
    lv_obj_set_style_text_font(ssid_dropdown, &chinese_24, 0);


    lv_obj_t* ssid_list = lv_dropdown_get_list(ssid_dropdown);
    lv_obj_set_style_text_font(ssid_list, &chinese_24, LV_PART_MAIN);
    lv_dropdown_set_symbol(ssid_dropdown, NULL);

    // 密码输入框
    password_ta = lv_textarea_create(config_page);
    lv_obj_set_size(password_ta, 220, 50);
    lv_obj_align(password_ta, LV_ALIGN_TOP_MID, 0, 120);
    lv_textarea_set_placeholder_text(password_ta, "请输入密码...");
    lv_textarea_set_one_line(password_ta, true);
    lv_textarea_set_password_mode(password_ta, false);
    lv_obj_set_style_text_font(password_ta, &chinese_24, 0);
    lv_obj_add_event_cb(password_ta, ta_event_cb, LV_EVENT_ALL, NULL);

    // 扫描按钮
    lv_obj_t* scan_btn = lv_btn_create(config_page);
    lv_obj_set_size(scan_btn, 100, 40);
    lv_obj_align(scan_btn, LV_ALIGN_TOP_MID, -60, 180);  // 左半边
    lv_obj_set_style_bg_color(scan_btn, lv_color_hex(0x3498db), 0);
    lv_obj_add_event_cb(scan_btn, trigger_rescan_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* scan_label = lv_label_create(scan_btn);
    lv_label_set_text(scan_label, "扫描");
    lv_obj_set_style_text_font(scan_label, &chinese_24, 0);
    lv_obj_center(scan_label);

    // 保存按钮
    lv_obj_t* save_btn = lv_btn_create(config_page);
    lv_obj_set_size(save_btn, 100, 40);
    lv_obj_align(save_btn, LV_ALIGN_TOP_MID, 60, 180);   // 右半边
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x2ecc71), 0);
    lv_obj_add_event_cb(save_btn, save_wifi_config_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "保存");
    lv_obj_set_style_text_font(save_label, &chinese_24, 0);
    lv_obj_center(save_label);

    // 取消按钮
    lv_obj_t* cancel_btn = lv_btn_create(config_page);
    lv_obj_set_size(cancel_btn, 220, 40);
    lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0xe74c3c), 0);
    lv_obj_add_event_cb(cancel_btn, [](lv_event_t* e) {
        lv_obj_clean(subContainer);
        show_wifi_page();   // 返回 WiFi 列表页
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "取消");
    lv_obj_set_style_text_font(cancel_label, &chinese_24, 0);
    lv_obj_center(cancel_label);

    // 键盘
    kb = lv_keyboard_create(config_page);
    lv_obj_set_size(kb, 240, 120);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, password_ta);
    lv_obj_set_style_pad_all(kb, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(kb, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(kb, 4, LV_PART_ITEMS);
    lv_obj_set_style_text_font(kb, &lv_font_montserrat_14, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(kb, lv_palette_lighten(LV_PALETTE_BLUE, 1), LV_STATE_PRESSED | LV_PART_ITEMS);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_ALL, NULL);
}

// ================= 主 WiFi 列表页面 =================
static void show_wifi_page() {
    wifiList = lv_list_create(subContainer);
    lv_obj_set_size(wifiList, 240, 280);
    lv_obj_align(wifiList, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_border_width(wifiList, 0, 0);
    lv_obj_set_style_pad_all(wifiList, 0, 0);
    lv_obj_set_style_bg_color(wifiList, lv_color_black(), 0);

    // 配置 WiFi 按钮
    lv_obj_t* cfgBtn = lv_list_add_btn(wifiList, NULL, "    配置 WiFi");
    lv_obj_set_height(cfgBtn, 56);
    lv_obj_set_style_text_font(cfgBtn, &chinese_24, 0);
    lv_obj_set_style_bg_color(cfgBtn, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_text_color(cfgBtn, lv_color_hex(0x3498db), 0);
    lv_obj_set_style_border_side(cfgBtn, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(cfgBtn, 2, 0);
    lv_obj_set_style_border_color(cfgBtn, lv_color_hex(0x3498db), 0);
    lv_obj_add_event_cb(cfgBtn, config_btn_cb, LV_EVENT_CLICKED, NULL);

    // 读取 5 个槽位并显示
    prefs.begin("watch", true);
    for (int i = 1; i <= 5; i++) {
        char key[16];
        snprintf(key, sizeof(key), "WiFiSSID%d", i);
        String ssid = prefs.getString(key, "");

        String labelText;
        if (ssid.isEmpty()) {
            char buf[32];
            snprintf(buf, sizeof(buf), "槽位 %d (空)", i);
            labelText = String(buf);
        } else {
            labelText = ssid;
        }

        lv_obj_t* btn = lv_list_add_btn(wifiList, NULL, labelText.c_str());
        lv_obj_set_height(btn, 56);
        lv_obj_set_style_text_font(btn, ssid.isEmpty() ? &chinese_24 : &chinese_24, 0);
        lv_obj_set_style_radius(btn, 0, 0);
        lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x444444), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x212121), 0);
        lv_obj_set_style_text_color(btn, ssid.isEmpty() ? lv_color_hex(0x888888) : lv_color_white(), 0);
        lv_obj_add_event_cb(btn, wifi_item_cb, LV_EVENT_CLICKED, NULL);
    }
    prefs.end();
}

// ================= 功能页面 =================
static void show_wallpaper_page() {
    init_wallpaper_ui();
    start_wallpaper_task();
}
static void cal_timer_cb(lv_timer_t* timer) {
    // 检查所有 UI 对象是否有效（与原来相同）
    if (!voltage_label || !lv_obj_is_valid(voltage_label) ||
        !battery_percent_label || !lv_obj_is_valid(battery_percent_label) ||
        !cal_slider || !lv_obj_is_valid(cal_slider)) {
        if (cal_timer) {
            lv_timer_del(cal_timer);
            cal_timer = nullptr;
        }
        return;
    }

    // 采集电压（与电池任务相同）
    const int SAMPLES = 20;
    uint32_t sum = 0;
    for (int i = 0; i < SAMPLES; i++) {
        sum += analogReadMilliVolts(BATTERY_ADC_PIN);
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    float avg_mv = sum / (float)SAMPLES;
    float voltage = (avg_mv / 1000.0f) * VOLTAGE_DIVIDER_RATIO;

    // 获取当前滑动条校准值
    int cal_val = lv_slider_get_value(cal_slider);
    // 计算校准后电压
    float calibrated_voltage = voltage * (cal_val / 100.0f);

    // 更新电压标签
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2fV", calibrated_voltage);
    lv_label_set_text(voltage_label, buf);
    // 根据校准后电压计算百分比
    int cal_percent = (int)((calibrated_voltage - BATTERY_EMPTY_V) / (BATTERY_FULL_V - BATTERY_EMPTY_V) * 100);
    if (cal_percent < 0) cal_percent = 0;
    if (cal_percent > 100) cal_percent = 100;

    // 更新校准后电量标签
    snprintf(buf, sizeof(buf), "%d%%", cal_percent);
    lv_label_set_text(battery_percent_label, buf);
}

// 滑动条事件：更新校准值显示并刷新电量
static void cal_slider_event_cb(lv_event_t* e) {
    int cal_val = lv_slider_get_value(cal_slider);
    lv_label_set_text_fmt(cal_value_label, "校准: %d%%", cal_val);
    // 立即调用定时器回调更新电量预览
    if (cal_timer) {
        cal_timer_cb(cal_timer);
    }
}

// 保存按钮回调
static void save_cal_cb(lv_event_t* e) {
    int cal_val = lv_slider_get_value(cal_slider);
    Preferences prefs;
    prefs.begin("watch", false);
    prefs.putFloat("batt_cal", (float)cal_val);
    prefs.end();
    atomic_store_float(&Calibration, (float)cal_val);

    // 显示短暂提示
    lv_obj_t* msg = lv_label_create(subContainer);
    lv_label_set_text(msg, "已保存");
    lv_obj_set_style_text_font(msg, &chinese_24, 0);
    lv_obj_set_style_text_color(msg, lv_color_hex(0x2ecc71), 0);
    lv_obj_align(msg, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_timer_t* del_timer = lv_timer_create([](lv_timer_t* t) {
    lv_obj_t* obj = (lv_obj_t*)t->user_data;
        if (obj && lv_obj_is_valid(obj)) {
            lv_obj_del(obj);
        }
        lv_timer_del(t);
    }, 1000, msg);
}

// 返回按钮回调
static void back_cal_cb(lv_event_t* e) {
    if (cal_timer) {
        lv_timer_del(cal_timer);
        cal_timer = nullptr;
    }
    close_sub_page();  // 关闭子页面，subContainer 及其子对象将被删除
}

// 构建电池校准页面
static void show_battery_calibration_page() {
    // 确保先创建子容器
    create_sub_page_base();
    lv_obj_clean(subContainer);

    // 电压标签
    voltage_label = lv_label_create(subContainer);
    lv_obj_set_style_text_font(voltage_label, &chinese_24, 0);
    lv_obj_set_style_text_color(voltage_label, lv_color_white(), 0);
    lv_obj_align(voltage_label, LV_ALIGN_TOP_MID, 0, 20);

    // 电量百分比标签（校准后）
    battery_percent_label = lv_label_create(subContainer);
    lv_obj_set_style_text_font(battery_percent_label, &chinese_24, 0);
    lv_obj_set_style_text_color(battery_percent_label, lv_color_white(), 0);
    lv_obj_align(battery_percent_label, LV_ALIGN_TOP_MID, 0, 60);

    // 校准滑动条
    cal_slider = lv_slider_create(subContainer);
    lv_obj_set_width(cal_slider, 200);
    lv_obj_align(cal_slider, LV_ALIGN_CENTER, 0, -20);
    lv_slider_set_range(cal_slider, 80, 120);
    lv_slider_set_value(cal_slider, (int)atomic_load_float(&Calibration), LV_ANIM_OFF);
    lv_obj_add_event_cb(cal_slider, cal_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // 当前校准值显示
    cal_value_label = lv_label_create(subContainer);
    lv_label_set_text_fmt(cal_value_label, "校准: %d%%", (int)atomic_load_float(&Calibration));
    lv_obj_set_style_text_font(cal_value_label, &chinese_24, 0);
    lv_obj_set_style_text_color(cal_value_label, lv_color_white(), 0);
    lv_obj_align(cal_value_label, LV_ALIGN_CENTER, 0, 20);

    // 保存按钮
    lv_obj_t* save_btn = lv_btn_create(subContainer);
    lv_obj_set_size(save_btn, 80, 40);
    lv_obj_align(save_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x2ecc71), 0);
    lv_obj_add_event_cb(save_btn, save_cal_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "保存");
    lv_obj_set_style_text_font(save_label, &chinese_24, 0);
    lv_obj_center(save_label);

    // 返回按钮
    lv_obj_t* back_btn = lv_btn_create(subContainer);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0xe74c3c), 0);
    lv_obj_add_event_cb(back_btn, back_cal_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "返回");
    lv_obj_set_style_text_font(back_label, &chinese_24, 0);
    lv_obj_center(back_label);

    // 启动定时器（每500ms更新一次）
    cal_timer = lv_timer_create(cal_timer_cb, 500, NULL);
}

static void show_carousel_interval_page() {
    lv_obj_t* tip = lv_label_create(subContainer);
    lv_label_set_text(tip, "自动轮播间隔\n开发中...");
    lv_obj_set_style_text_font(tip, &chinese_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(tip, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(tip, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(tip);
}

static void show_storage_page() {
    // 清除子容器原有内容
    lv_obj_clean(subContainer);

    // 创建信息显示标签
    lv_obj_t* info_label = lv_label_create(subContainer);
    lv_obj_set_width(info_label, 220);                          // 留出左右边距
    lv_label_set_long_mode(info_label, LV_LABEL_LONG_WRAP);    // 自动换行
    lv_obj_set_style_text_font(info_label, &chinese_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(info_label, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_center(info_label);

    String info = "";

    //LittleFS 存储信息
    if (LittleFS.begin(true, "/littlefs", 10, "littlefs")) {
        size_t total = LittleFS.totalBytes();
        size_t used = LittleFS.usedBytes();
        size_t free = total - used;
        float totalMB = total / (1024.0f * 1024.0f);
        float freeMB = free / (1024.0f * 1024.0f);
        char buf[64];
        snprintf(buf, sizeof(buf), "内部存储:%.2f%.2fMB\n", totalMB, freeMB);
        info += buf;
        LittleFS.end();
    } else {
        info += "内部存储:挂载失败\n";
    }

    size_t freeHeap = ESP.getFreeHeap();
    info += "可用RAM:" + String(freeHeap / 1024) + "KB\n";

    if (psramFound()) {
        size_t totalPsram = ESP.getPsramSize();
        size_t freePsram = ESP.getFreePsram();
        float totalMB = totalPsram / (1024.0f * 1024.0f);
        float freeMB = freePsram / (1024.0f * 1024.0f);
        char buf[64];
        snprintf(buf, sizeof(buf), "PSRAM:%.1f/%.1fMB\n", totalMB, freeMB);
        info += buf;
    } else {
        info += "PSRAM:未启用\n";
    }

    if (isSDCardReady()) {
        uint32_t totalMB = getSDCardSizeMB();
        uint32_t freeMB = getSDCardFreeSpaceMB();
        float totalGB = totalMB / 1024.0f;
        float freeGB = freeMB / 1024.0f;
        char buf[64];
        snprintf(buf, sizeof(buf), "SD:%.2f/%.2fGB", totalGB, freeGB);
        info += buf;
    } else {
        info += "SD:未插入";
    }

    lv_label_set_text(info_label, info.c_str());
}

// ================= 菜单选择分发 =================
static void handle_menu_selection(const char* itemText) {
    create_sub_page_base();

    if (strcmp(itemText, "时间同步") == 0) {
        show_time_sync_page();
    } else if (strcmp(itemText, "WiFi") == 0) {
        show_wifi_page();
    } else if (strcmp(itemText, "壁纸") == 0) {
        show_wallpaper_page();
    } else if (strcmp(itemText, "电池校准") == 0) {
        show_battery_calibration_page();
    } else if (strcmp(itemText, "自动轮播间隔") == 0) {
        show_carousel_interval_page();
    } else if (strcmp(itemText, "内存储存") == 0) {
        show_storage_page();
    } else {
        lv_obj_t* tip = lv_label_create(subContainer);
        char buf[64];
        snprintf(buf, sizeof(buf), "%s\n功能开发中...", itemText);
        lv_label_set_text(tip, buf);
        lv_obj_set_style_text_font(tip, &chinese_24, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(tip, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(tip, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(tip);
    }
}

// ================= 壁纸处理任务 =================
static bool scan_sd_card_for_wallpaper(String& imagePath) {
    set_status_msg("扫描壁纸文件夹...");
    vTaskDelay(pdMS_TO_TICKS(100));
    if (!sd.card() || sd.card()->errorCode()) return false;
    SdFile dir;
    if (!dir.open("/壁纸", O_RDONLY)) return false;
    const char* extensions[] = {".png", ".jpg", ".jpeg", ".PNG", ".JPG", ".JPEG"};
    SdFile file;
    bool found = false;
    while (file.openNext(&dir, O_RDONLY)) {
        char filename[256];
        file.getName(filename, sizeof(filename));
        for (const char* ext : extensions) {
            if (strstr(filename, ext) != nullptr) {
                imagePath = String("/壁纸/") + filename;
                found = true;
                break;
            }
        }
        file.close();
        if (found) break;
    }
    dir.close();
    return found;
}

static ImageProcessor::ErrorCode decode_image(const String& imagePath, uint16_t** outputBuffer, size_t* outputSize) {
    set_status_msg("正在解码图像...");
    ImageProcessor* imgProc = new ImageProcessor();
    imgProc->setProgressCallback(on_decode_progress);
    imgProc->setErrorCallback(on_decode_error);
    ImageProcessor::ErrorCode result = imgProc->loadAndProcessImage(
        imagePath.c_str(),
        outputBuffer,
        outputSize,
        240, 280
    );
    delete imgProc;
    return result;
}

static bool write_to_littlefs(uint16_t* outputBuffer, size_t outputSize) {
    set_status_msg("写入内置存储...");
    if (!LittleFS.begin(true, "/littlefs", 10, "littlefs")) {
        set_status_msg("LittleFS 挂载失败");
        return false;
    }
    File wallpaperFile = LittleFS.open("/wallpaper.bin", FILE_WRITE);
    if (!wallpaperFile) {
        set_status_msg("无法创建文件");
        LittleFS.end();
        return false;
    }
    const size_t CHUNK_SIZE = 2048;
    size_t written_total = 0;
    uint8_t* data_ptr = (uint8_t*)outputBuffer;
    bool write_failed = false;
    while (written_total < outputSize) {
        size_t remaining = outputSize - written_total;
        size_t to_write = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
        size_t res = wallpaperFile.write(data_ptr + written_total, to_write);
        if (res != to_write) {
            write_failed = true;
            break;
        }
        written_total += res;
        task_progress = 80 + (int)((float)written_total / outputSize * 20.0f);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    wallpaperFile.close();
    LittleFS.end();
    return !write_failed;
}

void wallpaper_worker_task(void* param) {
    current_state = STATE_WAIT_ANIM;
    task_progress = 0;
    set_status_msg("等待界面加载...");
    vTaskDelay(pdMS_TO_TICKS(500));

    current_state = STATE_SCANNING;
    String imagePath;
    bool found = scan_sd_card_for_wallpaper(imagePath);
    if (!found) {
        current_state = STATE_ERROR;
        set_status_msg("未找到壁纸文件(/壁纸/)");
        vTaskDelete(NULL);
        return;
    }

    current_state = STATE_DECODING;
    uint16_t* outputBuffer = nullptr;
    size_t outputSize = 0;
    ImageProcessor::ErrorCode result = decode_image(imagePath, &outputBuffer, &outputSize);
    if (result != ImageProcessor::SUCCESS || outputBuffer == nullptr) {
        current_state = STATE_ERROR;
        if (outputBuffer) free(outputBuffer);
        vTaskDelete(NULL);
        return;
    }

    current_state = STATE_WRITING;
    bool writeSuccess = write_to_littlefs(outputBuffer, outputSize);
    free(outputBuffer);

    if (!writeSuccess) {
        current_state = STATE_ERROR;
        set_status_msg("写入 Flash 失败");
    } else {
        task_progress = 100;
        current_state = STATE_SUCCESS;
        set_status_msg("设置成功!");
    }

    wp_task_handle = nullptr;
    vTaskDelete(NULL);
}

static void init_wallpaper_ui() {
    progressBar = lv_bar_create(subContainer);
    lv_obj_set_size(progressBar, 200, 20);
    lv_obj_align(progressBar, LV_ALIGN_CENTER, 0, 0);
    lv_bar_set_range(progressBar, 0, 100);
    lv_bar_set_value(progressBar, 0, LV_ANIM_OFF);

    progressLabel = lv_label_create(subContainer);
    lv_label_set_text(progressLabel, "初始化中...");
    lv_obj_set_style_text_font(progressLabel, &chinese_24, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(progressLabel, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_align(progressLabel, LV_ALIGN_CENTER, 0, 30);

    current_state = STATE_IDLE;
    task_progress = 0;
    uiMonitorTimer = lv_timer_create(wallpaper_ui_monitor_cb, 50, NULL);
}

static void start_wallpaper_task() {
    xTaskCreatePinnedToCore(wallpaper_worker_task, "WpTask", 20480, NULL, 1, &wp_task_handle, 0);
}

// ================= 主初始化函数 =================
void fs_create_settings(lv_obj_t* container) {
    mainList = lv_list_create(container);
    lv_obj_set_size(mainList, 240, 280);
    lv_obj_center(mainList);

    const char* item_texts[] = {
        "时间同步", "WiFi", "壁纸", "电池校准", "自动轮播间隔", "内存储存"
    };

    for (int i = 0; i < 6; i++) {
        lv_obj_t* btn = lv_list_add_btn(mainList, NULL, item_texts[i]);
        lv_obj_set_height(btn, 50);
        lv_obj_set_style_text_font(btn, &chinese_24, LV_STATE_DEFAULT);
        lv_obj_add_event_cb(btn, list_item_cb, LV_EVENT_CLICKED, NULL);
    }

    if (ioTimer == nullptr) {
        ioTimer = lv_timer_create(io_poll_timer_cb, 50, NULL);
    }
}