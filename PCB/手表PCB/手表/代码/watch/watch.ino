#include <lvgl.h>
#include <LovyanGFX.hpp>
#include "lgfx_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "ui_manager.h"
#include "SDScan.h"
#include "atomic_utils.h" 
#include "RTCManager.h"   
#include "esp_adc_cal.h"


//SD卡状态信号量 
bool sd_card_inserted = false;
bool sd_card_initialized = false;
int sd_card_insertion_event = 0;
bool sd_card_init_event = false;// SD卡初始化失败事件
bool sd_card_scan_event = false;
bool sd_force_scan = false;
bool sd_card_warming= false;// SD卡预热中（预写入）
SemaphoreHandle_t sd_state_mutex = NULL;
QueueHandle_t scan_queue = NULL;

// SPI总线占用标志
static bool spi_busy_flag = false;


static esp_adc_cal_characteristics_t adc_chars;
static bool is_calibrated = false;

// 电池监测
float battery_voltage = 0.0f;
int   battery_percentage = 0;
volatile float Calibration = 100;

// 定义全局信号量
SemaphoreHandle_t video_flush_sem = NULL;

hw_timer_t *watchdog_timer = NULL;
TaskHandle_t slow_start_task_handle = NULL;

volatile int slow_start_exit_flag = 0;      // 立即退出启动任务
volatile int slow_start_completed = 0;      // 启动动画正常执行完毕
volatile int current_brightness = 0;        // 当前屏幕亮度

#define BATTERY_ADC_PIN         4
#define ADC_MAX_VALUE       4096.0f
#define ADC_REF_VOLTAGE       3.0f
#define VOLTAGE_DIVIDER_RATIO 2.0f
#define BATTERY_FULL_V        4.2f
#define BATTERY_EMPTY_V       3.3f
#define FILTER_ALPHA          0.3f


LGFX display;
Preferences preferences;

#define TFT_HOR_RES   240
#define TFT_VER_RES   280
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 5 * (LV_COLOR_DEPTH / 8))
static lv_color_t draw_buf[DRAW_BUF_SIZE];

static lv_disp_draw_buf_t draw_buf_dsc;
static lv_disp_drv_t disp_drv;
static lv_disp_t * disp;

static lv_indev_drv_t indev_drv;
static lv_indev_t * indev;


void ARDUINO_ISR_ATTR onWatchdogTimeout() {
    esp_restart();  // 直接重启
}

void slow_start_task(void* parameter) {
    // 读取保存的亮度值
    preferences.begin("watch", true);
    int target_brightness = preferences.getUChar("brightness", 128);
    preferences.end();
    int step_delay = 20; // 每步延迟(ms)
    int steps = 50; // 总步数
    
    display.setBrightness(0);
    atomic_store_int(&current_brightness, 0);   // 更新当前亮度
    vTaskDelay(pdMS_TO_TICKS(100));
    
    for (int i = 1; i <= steps; i++) {
        // 检查退出标志
        if (atomic_load_int(&slow_start_exit_flag)) {
            // 要求退出，立即结束任务
            vTaskDelete(NULL);
            return;
        }
        
        float progress = (float)i / steps; 
        float gamma = 2.2; 
        int brightness = (int)(target_brightness * pow(progress, gamma));
        brightness = constrain(brightness, 0, target_brightness);
        
        display.setBrightness(brightness);
        atomic_store_int(&current_brightness, brightness);  // 更新当前亮度
        vTaskDelay(pdMS_TO_TICKS(step_delay));
    }
    
    // 确保最终达到目标亮度
    display.setBrightness(target_brightness);
    atomic_store_int(&current_brightness, target_brightness);
    
    // 标记启动动画正常完成
    atomic_store_int(&slow_start_completed, 1);
    
    vTaskDelete(NULL);
}


void my_print(const char* buf) {
    Serial.println(buf);
    Serial.flush();
}

void my_disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
    // 检查SPI是否被SD卡初始化占用
    if (atomic_load_bool(&spi_busy_flag)) {
        // 跳过
        lv_disp_flush_ready(disp_drv);
        return;
    }

    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    display.startWrite();
    display.setAddrWindow(area->x1, area->y1, w, h);
    display.writePixels((lgfx::rgb565_t*)color_p, w * h);
    display.endWrite();
    lv_disp_flush_ready(disp_drv);
    if (video_flush_sem != NULL) {
        xSemaphoreGive(video_flush_sem);
    }
}

void my_touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data) {
    uint16_t x, y;
    bool touched = display.getTouch(&x, &y);
    data->state = touched ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    if (touched) {
        data->point.x = x;
        data->point.y = y;
    }
}

static uint32_t my_tick(void) {
    return millis();
}
// 预写入函数
void pre_write_warm_sd_card() {
    if (xSemaphoreTake(sd_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        sd_card_warming = true;
        xSemaphoreGive(sd_state_mutex);
    }
    // 预写文件
    const char* warm_file = "/warm_sd.tmp";
    FsFile warmFile = sd.open(warm_file, O_WRONLY | O_CREAT | O_TRUNC);
    Serial.printf("%lu\n", millis());
    if (warmFile) {
        warmFile.write('0');
        warmFile.close();
        //删除临时文件
        sd.remove(warm_file);
        Serial.printf("%lu\n", millis());
    } else {
        Serial.println("SD卡预写入失败");
    }
    if (xSemaphoreTake(sd_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        sd_card_warming = false;
        xSemaphoreGive(sd_state_mutex);
    }
}
// ==================== SD卡初始化 ====================
bool initialize_sd_card() {
    bool init_result = false;   
    setScanProgressCallback(onScanProgress);    

    //SD卡初始化期间禁止屏幕刷新使用SPI
    atomic_store_bool(&spi_busy_flag, true);   // 设置标志
    vTaskDelay(pdMS_TO_TICKS(10));
    bool sd_init_ok = initializeSDCard();      // 执行初始化
    atomic_store_bool(&spi_busy_flag, false);  // 清除标志


    if (!sd_init_ok) {
        Serial.println("SD卡初始化失败");    
        if (xSemaphoreTake(sd_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            sd_card_initialized = false;
            sd_card_init_event = true;
            xSemaphoreGive(sd_state_mutex);
        }
        init_result = false;
    } else {
        Serial.println("SD卡初始化成功");

        // 预写入
        pre_write_warm_sd_card();

        init_result = true;
        ensureDirectoriesAndFiles();
        if (checkNeedRescan()) {
            if (xSemaphoreTake(sd_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                sd_card_scan_event = true;
                xSemaphoreGive(sd_state_mutex);
            }
            if (scan_queue != NULL) xQueueReset(scan_queue);
            performScan();
            if (xSemaphoreTake(sd_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                sd_card_scan_event = false;
                xSemaphoreGive(sd_state_mutex);
            }
        }
        // 设置SD卡初始化成功状态
        if (xSemaphoreTake(sd_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            sd_card_initialized = true;
            xSemaphoreGive(sd_state_mutex);
        }
    }
    return init_result;
}

void onScanProgress(int categoryIndex, int currentCount, const char* categoryName) {
    ScanData scan_data;
    scan_data.category_index = categoryIndex;
    scan_data.current_count = currentCount;
    if (categoryName) {
        strncpy(scan_data.category_name, categoryName, sizeof(scan_data.category_name)-1);
        scan_data.category_name[sizeof(scan_data.category_name)-1] = '\0';
    }
    if (scan_queue) xQueueSend(scan_queue, &scan_data, 0);
}

void sd_init_task(void* parameter) {  
    uint8_t last_sd_pin_state = HIGH;
    while (1) {
        if (!atomic_load_bool(&is_fullscreen_container_active)) {
            vTaskDelay(pdMS_TO_TICKS(200));
            if (digitalRead(15) == LOW) {
                vTaskDelay(pdMS_TO_TICKS(200));
                Serial.println("SD卡插入");
                if (xSemaphoreTake(sd_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    sd_card_inserted = true;
                    xSemaphoreGive(sd_state_mutex);
                }
                initialize_sd_card();
            } else {
                Serial.println("未检测到SD卡");
                if (xSemaphoreTake(sd_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    sd_card_inserted = false;
                    sd_card_initialized = false;
                    xSemaphoreGive(sd_state_mutex);
                }
            }
            last_sd_pin_state = digitalRead(15);
            while(1) {
                bool need_force_scan = false;
                if (xSemaphoreTake(sd_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    need_force_scan = sd_force_scan;
                    if (need_force_scan) sd_force_scan = false;
                    xSemaphoreGive(sd_state_mutex);
                }
                if (need_force_scan) {
                    Serial.println("触发SD卡重新扫描");
                    if (xSemaphoreTake(sd_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                        sd_card_scan_event = true;
                        xSemaphoreGive(sd_state_mutex);
                    }
                    if (scan_queue) xQueueReset(scan_queue);
                    performScan();
                    if (xSemaphoreTake(sd_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                        sd_card_scan_event = false;
                        xSemaphoreGive(sd_state_mutex);
                    }
                }
                uint8_t curr_sd_pin_state = digitalRead(15);
                vTaskDelay(pdMS_TO_TICKS(200));
                if (last_sd_pin_state == HIGH && curr_sd_pin_state == LOW) {
                    Serial.println("SD卡插入");
                    if (xSemaphoreTake(sd_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                        sd_card_insertion_event = 1;
                        sd_card_inserted = true;
                        xSemaphoreGive(sd_state_mutex);
                    }
                    initialize_sd_card();
                } else if (last_sd_pin_state == LOW && curr_sd_pin_state == HIGH) {
                    Serial.println("[SD检测] 上升沿触发,SD卡拔出");
                    if (xSemaphoreTake(sd_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                        sd_card_insertion_event = -1;
                        sd_card_inserted = false;
                        sd_card_initialized = false;
                        xSemaphoreGive(sd_state_mutex);
                    }
                }    
                last_sd_pin_state = curr_sd_pin_state;
            }
            vTaskDelete(NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
void battery_monitor_task(void* parameter) {
    gpio_reset_pin((gpio_num_t)BATTERY_ADC_PIN);
    pinMode(BATTERY_ADC_PIN, ANALOG); 
    gpio_pullup_dis((gpio_num_t)BATTERY_ADC_PIN);
    gpio_pulldown_dis((gpio_num_t)BATTERY_ADC_PIN);
    analogReadResolution(12);
    analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);

    float filtered_voltage = 0.0f;
    const int SAMPLES_PER_CYCLE = 20;
    TickType_t last_wake_time = xTaskGetTickCount();

    while (1) {
        // 采样并计算滤波电压
        uint32_t mv_sum = 0;
        for (int i = 0; i < SAMPLES_PER_CYCLE; i++) {
            mv_sum += analogReadMilliVolts(BATTERY_ADC_PIN);
            vTaskDelay(pdMS_TO_TICKS(2));
        }
        float avg_pin_mv = (float)mv_sum / SAMPLES_PER_CYCLE;
        float battery_voltage_raw = (avg_pin_mv / 1000.0f) * VOLTAGE_DIVIDER_RATIO;
        if (filtered_voltage < 0.1f) {
            filtered_voltage = battery_voltage_raw;
        } else {
            filtered_voltage = (FILTER_ALPHA * battery_voltage_raw) + ((1.0f - FILTER_ALPHA) * filtered_voltage);
        }

        // 读取校准系数
        float cal = atomic_load_float(&Calibration);
        // 用校准系数修正电压
        float calibrated_voltage = filtered_voltage * (cal / 100.0f);

        // 根据校准后的电压计算电量百分比
        int base_percent = (int)((calibrated_voltage - BATTERY_EMPTY_V) / (BATTERY_FULL_V - BATTERY_EMPTY_V) * 100.0f);
        if (base_percent < 0) base_percent = 0;
        if (base_percent > 100) base_percent = 100;

        atomic_store_float(&battery_voltage, filtered_voltage);   // 存储原始电压
        atomic_store_int(&battery_percentage, base_percent);      // 存储校准后百分比

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(60000));
    }
}
void sleep(){
    display.setBrightness(0);
    analogWrite(45, 0);
    digitalWrite(45, LOW);
    rtc_gpio_deinit(GPIO_NUM_8);
    rtc_gpio_set_direction(GPIO_NUM_8, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(GPIO_NUM_8, 1);
    gpio_hold_en(GPIO_NUM_8); 
    gpio_deep_sleep_hold_en();
    rtc_gpio_deinit(GPIO_NUM_6);
    rtc_gpio_set_direction(GPIO_NUM_6, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_en(GPIO_NUM_6); 
    rtc_gpio_pullup_dis(GPIO_NUM_6);  
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_6, 1); 
    Serial.flush(); 
    esp_deep_sleep_start();
}
void setup() {
    Serial.begin(115200);
    sd_state_mutex = xSemaphoreCreateMutex();
    scan_queue = xQueueCreate(10, sizeof(ScanData));

    pinMode(15, INPUT_PULLUP);

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_5) | (1ULL << GPIO_NUM_6) | (1ULL << GPIO_NUM_7);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;  
    gpio_config(&io_conf);

    // 从 Preferences 读取校准值
    preferences.begin("watch", true);
    float saved_cal = preferences.getFloat("batt_cal", 100);
    preferences.end();
    atomic_store_float(&Calibration, saved_cal);  

    xTaskCreatePinnedToCore(battery_monitor_task, "BatteryMon", 3072, NULL, 1, NULL, 1);
     
    display.init();
    display.setRotation(0);
    display.setBrightness(0);
    display.setColorDepth(16);
    display.fillScreen(TFT_BLACK);
    
    if (rtcManager.begin()) {
        // 将 RTC 时间同步到系统
        if (rtcManager.syncToSystem()) {
            Serial.println("系统时间已从外部 RTC 同步");
        } else {
            Serial.println("RTC 同步失败");
        }
    } else {
        Serial.println("外部 RTC 不存在");
    }

    lv_init();
    

#if LV_USE_LOG
    lv_log_register_print_cb(my_print);
#endif

    // 显示驱动
    lv_disp_draw_buf_init(&draw_buf_dsc, draw_buf, NULL, DRAW_BUF_SIZE);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_HOR_RES;
    disp_drv.ver_res = TFT_VER_RES;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf_dsc;
    disp_drv.sw_rotate = 1;
    disp_drv.rotated = LV_DISP_ROT_NONE;
    disp = lv_disp_drv_register(&disp_drv);

    // 触摸驱动
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    indev = lv_indev_drv_register(&indev_drv);

    // 屏幕背景
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_STATE_DEFAULT);

    ui_init_all();

    // 创建二值信号量
    video_flush_sem = xSemaphoreCreateBinary();


    xTaskCreatePinnedToCore(sd_init_task, "SD Init", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(slow_start_task,"SlowStart",2048,NULL,1,&slow_start_task_handle,1);

    watchdog_timer = timerBegin(1000000);               
    timerAttachInterrupt(watchdog_timer, &onWatchdogTimeout); 
    timerAlarm(watchdog_timer, 10 * 1000000, false, 0);

    Serial.println("系统初始化完成");
} 
void loop() {
    if (watchdog_timer) {
        timerWrite(watchdog_timer, 0);
    }
    ui_update_uptime();
    ui_check_sd_card_event();
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(5));
}