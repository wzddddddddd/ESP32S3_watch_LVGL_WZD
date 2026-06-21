#ifndef FULLSCREEN_INTERFACES_H
#define FULLSCREEN_INTERFACES_H

#include <lvgl.h>
#include "File_Selection.h"
#include "atomic_utils.h"
#include "lgfx_config.h"
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <Arduino.h>
#include <cstdio>
#include <Preferences.h>
#include "RTCManager.h"   
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <math.h>
#include "SDScan.h"
LV_FONT_DECLARE(chinese_24);
LV_FONT_DECLARE(time_70);
LV_FONT_DECLARE(tip_display_35);
typedef enum {
    FS_TIME_SCREEN = 0,   // 时钟
    FS_PICTURE,            // 图片
    FS_NOVEL,              // 小说
    FS_VIDEO,              // 视频
    FS_MUSIC,              // 音乐
    FS_GAME,               // 游戏
    FS_CALCULATOR,         // 计算器
    FS_STOPWATCH,          // 计时/秒表
    FS_CALENDAR,           // 日历
    FS_TRANSFER,           // 下载/传输
    FS_SETTINGS,            // 设置
    FS_WEATHER             // 天气（新增）
} FullscreenFunction;

// 退出回调函数声明
void fs_do_adsorb();
void sleep();

extern SdFs sd;
extern Preferences preferences;
extern LGFX display; 

// SD卡状态声明
extern bool sd_card_inserted;      // SD卡是否已插入
extern bool sd_card_initialized;   // SD卡是否已完成初始化
extern int sd_card_insertion_event;// SD卡插拔事件
extern bool sd_card_init_event;    // SD卡初始化失败事件
extern bool sd_card_scan_event;    // SD卡扫描事件
extern bool sd_force_scan;         // 强制扫描标记
extern SemaphoreHandle_t sd_state_mutex;  // SD状态互斥锁

//电池全局变量
extern float battery_voltage;// 电池电压
extern int   battery_percentage;// 电量百分比
extern volatile float Calibration;//电压校准

extern volatile int slow_start_exit_flag;
extern volatile int slow_start_completed;
extern volatile int current_brightness;

// ==================== 倒计时全局变量====================
extern volatile bool g_countdown_active;      // true: 倒计时任务正在运行
extern volatile int  g_countdown_remaining;   // 剩余毫秒数
extern volatile int g_countdown_total;   // 总毫秒数

extern lv_obj_t* sliding_container;
extern lv_obj_t* fullscreen_container;

extern void destroy_file_selection_list(); 

// 创建全屏容器的函数声明
void create_fullscreen_container();
// 创建特定功能的全屏内容
void create_fullscreen_content(FullscreenFunction func);

#endif