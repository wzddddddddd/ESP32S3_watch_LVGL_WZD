#include <lvgl.h>
#include "fullscreen_interfaces.h"
#include "File_Selection.h"
#include "driver/gpio.h"
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "AudioFileSourceSdFat.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include "arduinoFFT.h"
#include <Preferences.h>
#include <vector>
#include <math.h>

extern void fs_do_adsorb();                    
extern FileSelectionInstance* g_file_selection_instance;     
extern SdFs sd;
extern bool sd_force_scan; 
extern SemaphoreHandle_t sd_state_mutex;
extern Preferences preferences;

LV_FONT_DECLARE(chinese_24);


#define DEBOUNCE_THRESHOLD      20
#define STATE_CHANGE_COOLDOWN   1000
#define TIMER_PERIOD            10
#define MUSIC_PATH_PREFIX       "/音乐/"
#define SCAN_LIST_PATH          "/ScanList/music.txt" 

// FFT 常量
#define REF_MAX_MAGNITUDE 400000.0f
#define FFT_SAMPLES  256
#define FFT_BINS     50

// I2S 引脚定义
#define I2S_BCLK  47
#define I2S_LRCLK 48
#define I2S_DIN   21

//前向声明
static void audioTask(void *pvParameters);

//结构体与枚举 

// 播放模式
enum PlayMode {
    MODE_SEQUENCE = 0,  // 顺序播放
    MODE_RANDOM,        // 随机播放
    MODE_LOOP           // 单曲循环
};

// 内存中的音乐条目缓存
struct MusicEntry {
    char name[128];      // 文件名
    char full_path[256]; // 完整路径
    bool played;         // 用于随机模式，标记是否已播放
};

// GPIO 状态
static bool last_gpio6_level = false;
static uint32_t stable_start_gpio6 = 0;
static bool last_sd_card_state = true;   
static lv_timer_t* gpio_timer = NULL;
static lv_obj_t* g_container = NULL;

// 播放列表状态
static std::vector<MusicEntry> playlist;
static int current_song_index = -1;
static PlayMode current_mode = MODE_SEQUENCE;

// UI 对象
static lv_obj_t *volume_slider = NULL;
static lv_obj_t *progress_slider = NULL;
static lv_obj_t *play_pause_btn = NULL;
static lv_obj_t *play_pause_label = NULL;
static lv_timer_t *progress_timer = NULL;
static lv_obj_t *prev_btn = NULL;
static lv_obj_t *next_btn = NULL;
static lv_obj_t *mode_btn = NULL;     
static lv_obj_t *mode_label = NULL;
static lv_obj_t *file_name_label = NULL;
static lv_obj_t *volume_icon = NULL;

// 消息框对象
static lv_obj_t* music_msgbox_bg = NULL;
static lv_obj_t* music_msgbox = NULL;

// 音频对象
static AudioGeneratorMP3 *mp3 = nullptr;
static AudioFileSourceSdFat *audio_file = nullptr;
static AudioOutputI2S *out = nullptr;
static TaskHandle_t audioTaskHandle = NULL;
static QueueHandle_t cmdQueue = NULL;

// 共享状态
typedef struct {
    volatile bool fileOpened;           // 文件是否已打开
    volatile uint32_t fileSizeBytes;    // 文件大小（字节）
    volatile uint32_t currentPosBytes;  // 当前播放位置（字节）
    volatile bool isPlaying;            // 是否正在播放
    volatile bool isPaused;             // 是否暂停
    volatile float volume;               // 当前音量（线性值）
    volatile bool taskQuitFlag;          // 任务退出标志
    volatile bool songFinished;          // 歌曲结束标志，用于通知主线程
    volatile bool openFailed;            // 文件打开失败标志
    char lastFailedPath[256];             // 上次失败的文件路径
} PlayerStatus;
static PlayerStatus playerStatus = {0};

//手动请求标志
static bool manual_request_pending = false;  // 当前打开请求是否由用户手动触发

// 命令类型枚举
enum CommandType {
    CMD_NONE = 0, 
    CMD_PLAY, 
    CMD_PAUSE, 
    CMD_TOGGLE_PAUSE, 
    CMD_SEEK_BYTES, 
    CMD_SET_VOLUME, 
    CMD_QUIT_TASK, 
    CMD_OPEN_FILE
};

// 队列命令结构体
typedef struct {
    CommandType cmd;
    union {
        int32_t seekPos;
        float volume;       // 线性音量值 (0.0 - 1.0)
        char filePath[256];
    } param;
} QueuedCommand;

// FFT 变量
static volatile float fft_magnitudes[FFT_BINS] = {0};
static volatile bool   fft_new_data = false;
static double    fft_real[FFT_SAMPLES];
static double    fft_imag[FFT_SAMPLES];
static int16_t   fft_sample_buf[FFT_SAMPLES];
static uint16_t  fft_sample_idx = 0;
static bool      fft_input_ready = false;
static arduinoFFT fft(fft_real, fft_imag, FFT_SAMPLES, 11008);

// 画布相关
#define BAR_WIDTH       3
#define BAR_SPACING     1
#define MAX_BAR_HEIGHT  20
#define CANVAS_WIDTH    (FFT_BINS * (BAR_WIDTH + BAR_SPACING) - BAR_SPACING)
#define CANVAS_HEIGHT   (MAX_BAR_HEIGHT + 2)
static uint8_t canvas_buf[CANVAS_WIDTH * CANVAS_HEIGHT * 2];
static lv_obj_t* fft_canvas = NULL;

// 内部标志
static bool music_state_machine = false;
static bool need_switch_to_music = false;
static uint32_t last_state_change_ms = 0;
static String initial_file_name = ""; 
static volatile bool is_dragging_progress = false;

// ==================== 函数声明 ====================
static void clean_music_interface(void);
static void quitAudioTaskSafely();
static void stopAndReleaseResources();
static void play_song_by_index(int index, bool manual); 
static void play_next_auto();
static void play_next_manual();
static void play_prev_manual();
static void load_playlist_to_ram(const char* initial_filename);
static void update_mode_ui();
static void update_filename_ui(const char* filename);
static void show_file_not_found_msgbox();
static void msgbox_btn_cb(lv_event_t * e);
// 音量转换函数
static float linear_to_log_volume(int slider_value);
static int log_to_linear_slider(float log_volume);

// ==================== 播放列表与逻辑函数 ====================
//从SD卡的ScanList加载音乐列表到内存
static void load_playlist_to_ram(const char* initial_filename) {
    playlist.clear();
    current_song_index = 0;

    FsFile file;
    char line[256];
    
    // 从文件选择逻辑创建的扫描列表中读取
    if (file.open(SCAN_LIST_PATH, O_RDONLY)) {
        while (file.fgets(line, sizeof(line))) {
            line[strcspn(line, "\r\n")] = 0; // 移除换行符
            if (strlen(line) == 0) continue;
            if (strncmp(line, "FileCount:", 10) == 0) continue;

            MusicEntry entry;
            strncpy(entry.name, line, sizeof(entry.name) - 1);
            snprintf(entry.full_path, sizeof(entry.full_path), "%s%s", MUSIC_PATH_PREFIX, line);
            entry.played = false;
            
            playlist.push_back(entry);

            // 检查这是否是选中的文件
            if (initial_filename && strcmp(line, initial_filename) == 0) {
                current_song_index = playlist.size() - 1;
            }
        }
        file.close();
        Serial.printf("播放列表已加载: %d 首歌曲。当前索引: %d\n", playlist.size(), current_song_index);
    } else {
        Serial.println("无法打开音乐扫描列表！");
        // 备用方案：如果列表打开失败，添加单个文件
        if (initial_filename) {
            MusicEntry entry;
            strncpy(entry.name, initial_filename, sizeof(entry.name));
            snprintf(entry.full_path, sizeof(entry.full_path), "%s%s", MUSIC_PATH_PREFIX, initial_filename);
            entry.played = false;
            playlist.push_back(entry);
        }
    }
}

//重置随机播放的已播放标志
static void reset_random_flags() {
    for (auto &entry : playlist) {
        entry.played = false;
    }
    // 标记当前歌曲为已播放
    if (current_song_index >= 0 && current_song_index < playlist.size()) {
        playlist[current_song_index].played = true;
    }
}

//播放指定索引的歌曲
static void play_song_by_index(int index, bool manual) {
    if (playlist.empty()) return;
    if (index < 0) index = playlist.size() - 1;
    if (index >= playlist.size()) index = 0;

    current_song_index = index;
    MusicEntry* entry = &playlist[current_song_index];

    // 为随机模式标记为已播放
    entry->played = true;

    // 更新UI
    update_filename_ui(entry->name);

    // 设置手动请求标志
    manual_request_pending = manual;

    // 发送命令到音频任务
    QueuedCommand qcmd;
    qcmd.cmd = CMD_OPEN_FILE;
    strncpy(qcmd.param.filePath, entry->full_path, sizeof(qcmd.param.filePath) - 1);
    qcmd.param.filePath[sizeof(qcmd.param.filePath) - 1] = '\0';
    xQueueSend(cmdQueue, &qcmd, portMAX_DELAY);
}

//自动下一首
static void play_next_auto() {
    if (playlist.empty()) return;

    int next_index = current_song_index;

    switch (current_mode) {
        case MODE_SEQUENCE:
            next_index++;
            if (next_index >= playlist.size()) next_index = 0;
            break;

        case MODE_LOOP:
            // 重复当前歌曲
            next_index = current_song_index;
            break;

        case MODE_RANDOM:
            // 寻找未播放的歌曲
            {
                std::vector<int> unplayed_indices;
                for (int i = 0; i < playlist.size(); i++) {
                    if (!playlist[i].played) unplayed_indices.push_back(i);
                }

                if (unplayed_indices.empty()) {
                    reset_random_flags();
                    // 如果可能，选择除了当前歌曲之外的任意一首
                    if (playlist.size() > 1) {
                        do {
                            next_index = random(0, playlist.size());
                        } while(next_index == current_song_index);
                    } else {
                        next_index = 0;
                    }
                } else {
                    int r = random(0, unplayed_indices.size());
                    next_index = unplayed_indices[r];
                }
            }
            break;
    }

    play_song_by_index(next_index, false); // 自动切换，manual=false
}

//下一首按钮逻辑
static void play_next_manual() {
    if (playlist.empty()) return;
    int next_index = current_song_index;

    if (current_mode == MODE_RANDOM) {
        // 随机模式下手动下一首：从未播放列表中随机选一首
        std::vector<int> unplayed_indices;
        for (int i = 0; i < playlist.size(); i++) {
            if (!playlist[i].played) unplayed_indices.push_back(i);
        }
        if (unplayed_indices.empty()) {
            reset_random_flags();
            // 所有都已播放，重置后随机选择（可能重复）
            next_index = random(0, playlist.size());
        } else {
            int r = random(0, unplayed_indices.size());
            next_index = unplayed_indices[r];
        }
        play_song_by_index(next_index, true); // 手动切换，manual=true
    } else {
        // 顺序与循环模式（手动下一首即使在循环模式下也强制下一首）
        next_index++;
        if (next_index >= playlist.size()) next_index = 0;
        play_song_by_index(next_index, true); // 手动切换，manual=true
    }
}

//上一首按钮逻辑
static void play_prev_manual() {
    if (playlist.empty()) return;
    int next_index = current_song_index - 1;
    if (next_index < 0) next_index = playlist.size() - 1;
    play_song_by_index(next_index, true); // 手动切换，manual=true
}

//显示“文件未找到”的消息框
static void show_file_not_found_msgbox(void) {
    if (music_msgbox_bg) return; // 已显示 - 避免重复弹出

    music_msgbox_bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(music_msgbox_bg, 240, 280);
    lv_obj_align(music_msgbox_bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(music_msgbox_bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(music_msgbox_bg, LV_OPA_50, 0);
    lv_obj_set_style_border_width(music_msgbox_bg, 0, 0);
    // 禁止背景滑动
    lv_obj_clear_flag(music_msgbox_bg, LV_OBJ_FLAG_SCROLLABLE);

    music_msgbox = lv_obj_create(music_msgbox_bg);
    lv_obj_set_size(music_msgbox, 200, 150);
    lv_obj_align(music_msgbox, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(music_msgbox, lv_color_make(50, 50, 50), 0);
    lv_obj_set_style_border_color(music_msgbox, lv_color_white(), 0);
    lv_obj_set_style_border_width(music_msgbox, 2, 0);
    lv_obj_set_style_radius(music_msgbox, 10, 0);
    // 禁止消息框滑动
    lv_obj_clear_flag(music_msgbox, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * label = lv_label_create(music_msgbox);
    lv_label_set_text(label, "找不到该文件\n是否重新扫描");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, &chinese_24, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -25);
    // 禁止标签滑动
    lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * btn_cont = lv_obj_create(music_msgbox);
    lv_obj_set_size(btn_cont, 180, 40);
    lv_obj_align(btn_cont, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_opa(btn_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_cont, 0, 0);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    // 禁止按钮容器滑动
    lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * btn_yes = lv_btn_create(btn_cont);
    lv_obj_set_size(btn_yes, 70, 30);
    lv_obj_set_style_bg_color(btn_yes, lv_color_make(0, 100, 0), 0);
    lv_obj_add_event_cb(btn_yes, msgbox_btn_cb, LV_EVENT_CLICKED, NULL);
    // 禁止按钮滑动
    lv_obj_clear_flag(btn_yes, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t * label_yes = lv_label_create(btn_yes);
    lv_label_set_text(label_yes, "是");
    lv_obj_set_style_text_color(label_yes, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_yes, &chinese_24, 0);
    lv_obj_center(label_yes);
    // 禁止标签滑动
    lv_obj_clear_flag(label_yes, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * btn_no = lv_btn_create(btn_cont);
    lv_obj_set_size(btn_no, 70, 30);
    lv_obj_set_style_bg_color(btn_no, lv_color_make(100, 0, 0), 0);
    lv_obj_add_event_cb(btn_no, msgbox_btn_cb, LV_EVENT_CLICKED, NULL);
    // 禁止按钮滑动
    lv_obj_clear_flag(btn_no, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t * label_no = lv_label_create(btn_no);
    lv_label_set_text(label_no, "否");
    lv_obj_set_style_text_color(label_no, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_no, &chinese_24, 0);
    lv_obj_center(label_no);
    // 禁止标签滑动
    lv_obj_clear_flag(label_no, LV_OBJ_FLAG_SCROLLABLE);
}
//消息框按钮回调函数
static void msgbox_btn_cb(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);
    const char * txt = lv_label_get_text(label);
    
    if (strcmp(txt, "是") == 0) {
        fs_do_adsorb(); // 返回主界面
        // 清理并强制重新扫描
        clean_music_interface();
        destroy_file_selection_list();
        
        if (gpio_timer) {
            lv_timer_del(gpio_timer);
            gpio_timer = NULL;  
        }
        
        if (xSemaphoreTake(sd_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            sd_force_scan = true;
            xSemaphoreGive(sd_state_mutex);
        }

    } else {
        // 关闭消息框
        if (music_msgbox_bg) {
            lv_obj_del(music_msgbox_bg);
            music_msgbox_bg = NULL;
            music_msgbox = NULL;
        }
        // 选择否：标记当前文件为已播放并自动跳过
        if (current_song_index >= 0 && current_song_index < playlist.size()) {
            playlist[current_song_index].played = true;
        }
        play_next_auto();
    }
}

// ==================== UI 辅助函数 ====================

//更新文件名标签的显示
static void update_filename_ui(const char* full_filename) {
    if (!file_name_label) return;

    // 1. 移除扩展名
    char name_buf[128];
    strncpy(name_buf, full_filename, sizeof(name_buf) - 1);
    name_buf[sizeof(name_buf)-1] = '\0';
    char* dot = strrchr(name_buf, '.');
    if (dot) *dot = '\0';

    lv_label_set_text(file_name_label, name_buf);

    // 2. 设置文本颜色为纯白且不透明
    lv_obj_set_style_text_color(file_name_label, lv_color_white(), 0);
    lv_obj_set_style_text_opa(file_name_label, LV_OPA_COVER, 0);

    // 3. 长度检查与滚动动画
    // 先设置宽度为内容自适应来获取真实宽度
    lv_label_set_long_mode(file_name_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(file_name_label, LV_SIZE_CONTENT);
    lv_obj_update_layout(file_name_label);
    
    int width = lv_obj_get_width(file_name_label);
    if (width > 240) {
        lv_obj_set_width(file_name_label, 240);
        lv_label_set_long_mode(file_name_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    } else {
        lv_obj_set_width(file_name_label, width); // 适应内容宽度
        lv_label_set_long_mode(file_name_label, LV_LABEL_LONG_CLIP);
    }
    
    // 4. 对齐 (Y 偏移 -30)
    lv_obj_align(file_name_label, LV_ALIGN_CENTER, -5, -30);
}

//更新模式按钮的显示图标
static void update_mode_ui() {
    if (!mode_label) return;
    
    const char* icon = "";
    switch(current_mode) {
        case MODE_SEQUENCE: icon = LV_SYMBOL_RIGHT; break;
        case MODE_RANDOM:   icon = LV_SYMBOL_SHUFFLE; break;
        case MODE_LOOP:     icon = LV_SYMBOL_LOOP; break;
    }
    lv_label_set_text(mode_label, icon);
}

static int log_to_linear_slider(float log_volume) {
    if (log_volume <= 0.009f) return 0;
    if (log_volume >= 1.0f) return 100;
    
    float normalized = (log_volume - 0.009f) / (1.0f - 0.009f);
    if (normalized < 0) normalized = 0;
    if (normalized > 1) normalized = 1;
    
    return (int)(100.0f * sqrtf(normalized));
}

static float linear_to_log_volume(int slider_value) {
    if (slider_value <= 0) return 0.009f;
    if (slider_value >= 100) return 1.0f;
    
    float normalized = slider_value / 100.0f;
    return 0.009f + (1.0f - 0.009f) * (normalized * normalized);
}

// ==================== UI 回调函数 ====================

//模式按钮点击回调
static void mode_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        // 循环切换: 顺序 -> 随机 -> 单曲循环 -> 顺序
        if (current_mode == MODE_SEQUENCE) current_mode = MODE_RANDOM;
        else if (current_mode == MODE_RANDOM) current_mode = MODE_LOOP;
        else current_mode = MODE_SEQUENCE;

        if (current_mode == MODE_RANDOM) reset_random_flags();
        
        update_mode_ui();
        
        // 保存播放模式到Preferences
        preferences.begin("watch", false);
        preferences.putInt("Playback_Mode", (int)current_mode);
        preferences.end();
        
        Serial.printf("模式切换为: %d 并已保存\n", current_mode);
    }
}

//上一首按钮点击回调
static void prev_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        play_prev_manual();
    }
}

//下一首按钮点击回调
static void next_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        play_next_manual();
    }
}

//播放/暂停按钮点击回调
static void play_pause_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        QueuedCommand qcmd = {CMD_TOGGLE_PAUSE, {0}};
        xQueueSend(cmdQueue, &qcmd, 0);
    }
}

// 音量滑块事件回调
static void volume_slider_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        int32_t slider_val = lv_slider_get_value(volume_slider);
    }
    
    if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
        int32_t slider_val = lv_slider_get_value(volume_slider);
        float log_volume = linear_to_log_volume(slider_val);
        
        // 保存音量到Preferences
        preferences.begin("watch", false);
        preferences.putFloat("Volume", log_volume);
        preferences.end();
        
        QueuedCommand qcmd = {CMD_SET_VOLUME, {.volume = log_volume}};
        xQueueSend(cmdQueue, &qcmd, 0);
    }
}

//进度条拖动事件回调
static void progress_slider_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSING) {
        is_dragging_progress = true;
    }
    if (code == LV_EVENT_RELEASED && playerStatus.fileOpened) {
        is_dragging_progress = false;
        int32_t prog_val = lv_slider_get_value(progress_slider);
        uint32_t seek_pos = (uint32_t)((float)prog_val / 1000.0f * playerStatus.fileSizeBytes);
        QueuedCommand qcmd = {CMD_SEEK_BYTES, {.seekPos = seek_pos}};
        xQueueSend(cmdQueue, &qcmd, 0);
    }
}

// ==================== 核心 UI 构建 ====================

void music_player_ui(lv_obj_t* parent) {
    //重置状态
    memset((void*)&playerStatus, 0, sizeof(PlayerStatus));
    manual_request_pending = false; // 初始化手动请求标志
    
    // 从Preferences读取保存的音量
    preferences.begin("watch", false);
    float saved_volume = preferences.getFloat("Volume", 0.5f); // 默认0.5
    int saved_mode = preferences.getInt("Playback_Mode", MODE_SEQUENCE); // 默认顺序播放
    preferences.end();
    
    playerStatus.volume = saved_volume; // 设置保存的音量
    current_mode = (PlayMode)saved_mode; // 设置保存的播放模式

    //加载播放列表到内存
    load_playlist_to_ram(initial_file_name.c_str());

    //设置 UI 容器
    lv_obj_set_size(parent, 240, 280);

    //文件名标签
    file_name_label = lv_label_create(parent);
    lv_obj_set_style_text_font(file_name_label, &chinese_24, 0);
    lv_obj_set_style_text_align(file_name_label, LV_TEXT_ALIGN_CENTER, 0);
    // 设置文本颜色为纯白且不透明
    lv_obj_set_style_text_color(file_name_label, lv_color_white(), 0);
    lv_obj_set_style_text_opa(file_name_label, LV_OPA_COVER, 0);
    
    if (!playlist.empty()) {
        update_filename_ui(playlist[current_song_index].name);
    } else {
        lv_label_set_text(file_name_label, "无音乐文件");
    }

    //音量滑块
    volume_slider = lv_slider_create(parent);
    lv_obj_set_size(volume_slider, 140, 10);
    lv_obj_set_pos(volume_slider, 70, 25);
    lv_slider_set_range(volume_slider, 0, 100);
    // 设置初始值为对数映射后的值（从Preferences读取）
    int slider_value = log_to_linear_slider(saved_volume);
    lv_slider_set_value(volume_slider, slider_value, LV_ANIM_OFF);
    lv_obj_add_event_cb(volume_slider, volume_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(volume_slider, volume_slider_event_cb, LV_EVENT_RELEASED, NULL);

    //模式切换按钮
    mode_btn = lv_btn_create(parent);
    lv_obj_set_size(mode_btn, 40, 40);
    lv_obj_set_pos(mode_btn, 10, 5); 
    lv_obj_set_style_radius(mode_btn, 10, 0);
    lv_obj_set_style_bg_color(mode_btn, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(mode_btn, mode_btn_event_cb, LV_EVENT_CLICKED, NULL);
    mode_label = lv_label_create(mode_btn);
    update_mode_ui();
    lv_obj_center(mode_label);

    // 音量图标 (白色，不透明，大号)
    volume_icon = lv_label_create(parent);
    lv_label_set_text(volume_icon, LV_SYMBOL_VOLUME_MID);
    lv_obj_set_pos(volume_icon, 135, 0); 
    lv_obj_set_style_text_color(volume_icon, lv_color_white(), 0);
    lv_obj_set_style_text_opa(volume_icon, LV_OPA_COVER, 0);
    lv_obj_set_style_transform_zoom(volume_icon, 350, 0); // 放大约1.4倍 (256为原始大小)

    //进度滑块
    progress_slider = lv_slider_create(parent);
    lv_obj_set_size(progress_slider, 210, 10);
    lv_obj_align(progress_slider, LV_ALIGN_CENTER, 0, 45); 
    lv_slider_set_range(progress_slider, 0, 1000);
    lv_slider_set_value(progress_slider, 0, LV_ANIM_OFF);
    lv_obj_add_event_cb(progress_slider, progress_slider_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(progress_slider, progress_slider_event_cb, LV_EVENT_PRESSING, NULL);

    // FFT 画布
    int start_x = (240 - CANVAS_WIDTH) / 2 - 13;
    int base_y = 160;
    fft_canvas = lv_canvas_create(parent);
    lv_obj_set_size(fft_canvas, CANVAS_WIDTH, CANVAS_HEIGHT);
    lv_obj_set_pos(fft_canvas, start_x, base_y - CANVAS_HEIGHT);
    lv_canvas_set_buffer(fft_canvas, canvas_buf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(fft_canvas, lv_color_hex(0x000000), LV_OPA_TRANSP);

    // 控制按钮 (上一首，播放/暂停，下一首)
    int btn_diameter = 50;
    int spacing = 10;
    int total_width = 3 * btn_diameter + 2 * spacing;
    int start_x_btns = (240 - total_width - 32) / 2;

    // 上一首按钮
    prev_btn = lv_btn_create(parent);
    lv_obj_set_size(prev_btn, btn_diameter, btn_diameter);
    lv_obj_set_style_radius(prev_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(prev_btn, lv_color_hex(0xC0A000), 0); 
    lv_obj_set_pos(prev_btn, start_x_btns, 280 - btn_diameter - 30);
    lv_obj_add_event_cb(prev_btn, prev_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *prev_label_icon = lv_label_create(prev_btn);
    lv_label_set_text(prev_label_icon, LV_SYMBOL_PREV);
    lv_obj_center(prev_label_icon);

    // 播放/暂停按钮
    play_pause_btn = lv_btn_create(parent);
    lv_obj_set_size(play_pause_btn, btn_diameter, btn_diameter);
    lv_obj_set_style_radius(play_pause_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(play_pause_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_pos(play_pause_btn, start_x_btns + btn_diameter + spacing, 280 - btn_diameter - 30);
    lv_obj_add_event_cb(play_pause_btn, play_pause_btn_event_cb, LV_EVENT_CLICKED, NULL);
    play_pause_label = lv_label_create(play_pause_btn);
    lv_label_set_text(play_pause_label, LV_SYMBOL_PLAY);
    lv_obj_center(play_pause_label);

    // 下一首按钮
    next_btn = lv_btn_create(parent);
    lv_obj_set_size(next_btn, btn_diameter, btn_diameter);
    lv_obj_set_style_radius(next_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(next_btn, lv_color_hex(0xC0A000), 0);
    lv_obj_set_pos(next_btn, start_x_btns + 2*(btn_diameter + spacing), 280 - btn_diameter - 30);
    lv_obj_add_event_cb(next_btn, next_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *next_label_icon = lv_label_create(next_btn);
    lv_label_set_text(next_label_icon, LV_SYMBOL_NEXT);
    lv_obj_center(next_label_icon);

    //启动定时器和音频任务
    progress_timer = lv_timer_create([](lv_timer_t *timer){
        // 用于访问本地静态函数的Lambda表达式
        if (is_dragging_progress) return;

        // 检查音频任务发出的歌曲结束信号
        if (playerStatus.songFinished) {
            playerStatus.songFinished = false; // 清除标志
            play_next_auto(); // 触发下一首逻辑
            return;
        }

        // 新增：检查文件打开失败
        if (playerStatus.openFailed) {
            playerStatus.openFailed = false; // 清除标志
            String failedPath = String(playerStatus.lastFailedPath);
            
            // 查找失败文件索引
            int failedIndex = -1;
            for (size_t i = 0; i < playlist.size(); i++) {
                if (failedPath.equals(playlist[i].full_path)) {
                    failedIndex = i;
                    break;
                }
            }
            
            if (manual_request_pending) {
                // 手动切换失败：弹框
                manual_request_pending = false;
                show_file_not_found_msgbox();
            } else {
                // 自动切换失败：静默跳过
                if (failedIndex != -1) {
                    playlist[failedIndex].played = true;
                }
                play_next_auto();
            }
            return;
        }

        // 更新进度滑块
        if (playerStatus.fileOpened && playerStatus.fileSizeBytes > 0) {
            float progress = (float)playerStatus.currentPosBytes / playerStatus.fileSizeBytes;
            int32_t prog_val = (int32_t)(progress * 1000.0f);
            lv_slider_set_value(progress_slider, prog_val, LV_ANIM_OFF);

            if (playerStatus.isPlaying) {
                lv_label_set_text(play_pause_label, playerStatus.isPaused ? LV_SYMBOL_PLAY : LV_SYMBOL_PAUSE);
            } else {
                lv_label_set_text(play_pause_label, LV_SYMBOL_PLAY);
            }
        }

        // 更新 FFT 显示
        if (fft_new_data) {
            fft_new_data = false;
            float maxVal = 0.001f;
            for (int i = 0; i < FFT_BINS; i++) if (fft_magnitudes[i] > maxVal) maxVal = fft_magnitudes[i];
            float normFactor = (maxVal > REF_MAX_MAGNITUDE) ? maxVal : REF_MAX_MAGNITUDE;

            lv_canvas_fill_bg(fft_canvas, lv_color_hex(0x000000), LV_OPA_TRANSP);
            lv_draw_rect_dsc_t rect_dsc;
            lv_draw_rect_dsc_init(&rect_dsc);
            rect_dsc.radius = 2;
            rect_dsc.bg_opa = LV_OPA_COVER;
            rect_dsc.border_width = 0;

            int bar_x = 0;
            for (int i = 0; i < FFT_BINS; i++) {
                float ratio = fft_magnitudes[i] / normFactor;
                int h = (int)(ratio * MAX_BAR_HEIGHT);
                if (h < 2) h = 2;
                if (h > MAX_BAR_HEIGHT) h = MAX_BAR_HEIGHT;
                uint16_t hue = (i * 360) / FFT_BINS;
                rect_dsc.bg_color = lv_color_hsv_to_rgb(hue, 100, 100);
                lv_canvas_draw_rect(fft_canvas, bar_x, CANVAS_HEIGHT - h, BAR_WIDTH, h, &rect_dsc);
                bar_x += BAR_WIDTH + BAR_SPACING;
            }
            lv_obj_invalidate(fft_canvas);
        }
    }, 20, NULL);

    if (cmdQueue == NULL) {
        cmdQueue = xQueueCreate(5, sizeof(QueuedCommand));
        xTaskCreatePinnedToCore(audioTask, "AudioTask", 8192, NULL, 2, &audioTaskHandle, 0);
    }

    // 在播放前先发送音量设置命令
    // 等待音频任务启动
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // 发送保存的音量到音频任务
    QueuedCommand volCmd = {CMD_SET_VOLUME, {.volume = saved_volume}};
    xQueueSend(cmdQueue, &volCmd, portMAX_DELAY);
    
    // 播放初始文件
    if (!playlist.empty()) {
        play_song_by_index(current_song_index, false); // 初始播放视为自动
    }
}

// ==================== 生命周期与 GPIO 函数 ====================

//清理音乐播放器界面及资源
static void clean_music_interface(void) {
    quitAudioTaskSafely();
    if (progress_timer) { lv_timer_del(progress_timer); progress_timer = NULL; }
    stopAndReleaseResources();

    // 清空内存播放列表
    playlist.clear();

    // 删除 UI 对象
    if (volume_slider) { lv_obj_del(volume_slider); volume_slider = NULL; }
    if (progress_slider) { lv_obj_del(progress_slider); progress_slider = NULL; }
    if (play_pause_btn) { lv_obj_del(play_pause_btn); play_pause_btn = NULL; }
    if (fft_canvas) { lv_obj_del(fft_canvas); fft_canvas = NULL; }
    if (prev_btn) { lv_obj_del(prev_btn); prev_btn = NULL; }
    if (next_btn) { lv_obj_del(next_btn); next_btn = NULL; }
    if (mode_btn) { lv_obj_del(mode_btn); mode_btn = NULL; }
    if (file_name_label) { lv_obj_del(file_name_label); file_name_label = NULL; }
    if (volume_icon) { lv_obj_del(volume_icon); volume_icon = NULL; }
    
    // 如果消息框打开则关闭
    if (music_msgbox_bg) { lv_obj_del(music_msgbox_bg); music_msgbox_bg = NULL; music_msgbox = NULL; }

    Serial.println("音乐界面已清理，内存列表已清除。");
}

//文件选择回调函数
static void music_file_selected_cb(const char* filename, void* user_data) {
    if (strcmp(filename, SCAN_SPECIAL_FILENAME) == 0) {
        fs_do_adsorb();
        clean_music_interface();
        destroy_file_selection_list();
        if (gpio_timer) {
            lv_timer_del(gpio_timer);
            gpio_timer = NULL;
        }
        if (xSemaphoreTake(sd_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            sd_force_scan = true;
            xSemaphoreGive(sd_state_mutex);
        }

        return;
    }
    
    Serial.printf("已选择音乐: %s\n", filename);
    initial_file_name = String(filename);
    need_switch_to_music = true;
}

//GPIO 状态检查定时器回调
static void gpio_check_cb(lv_timer_t* timer) {
    uint32_t now = lv_tick_get();

    // SD 卡状态检查
    static bool current_sd_state = true;
    if (xSemaphoreTake(sd_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        current_sd_state = sd_card_inserted;
        xSemaphoreGive(sd_state_mutex);
    }
    if (last_sd_card_state == true && current_sd_state == false) {
        Serial.println("SD卡拔出");
        fs_do_adsorb(); 
    
        if (out) {
            out->stop(); 
        }
    
        //资源释放
        clean_music_interface();
    
        //销毁列表并返回
        destroy_file_selection_list();
        if (gpio_timer) {
            lv_timer_del(gpio_timer);
            gpio_timer = NULL;
        }
    
        music_state_machine = false;
        need_switch_to_music = false;
        
    
        last_sd_card_state = current_sd_state;    
        return;
    }
    last_sd_card_state = current_sd_state;

    // GPIO 6 逻辑 (退出)
    bool current6 = gpio_get_level(GPIO_NUM_6);
    if (current6 != last_gpio6_level) {
        stable_start_gpio6 = now;
        last_gpio6_level = current6;
    }
    else if (current6 == 1 && (now - stable_start_gpio6) >= DEBOUNCE_THRESHOLD) {
        if (now - last_state_change_ms > STATE_CHANGE_COOLDOWN) {
            last_state_change_ms = now;
            if (music_state_machine) {
                // 退出音乐界面 -> 返回文件选择
                if (fullscreen_container) lv_obj_move_foreground(fullscreen_container);
                clean_music_interface();
                file_selection_create(g_container, FILE_TYPE_MUSIC, music_file_selected_cb, NULL);
                music_state_machine = false;
            } else {
                // 退出文件选择界面 -> 返回主界面
                fs_do_adsorb();
                destroy_file_selection_list();
                
                if (gpio_timer) { lv_timer_del(gpio_timer); gpio_timer = NULL; }
                return;
            }
        }
    }

    // 请求切换到音乐界面
    if (need_switch_to_music) {
        destroy_file_selection_list();
        music_player_ui(g_container);
        music_state_machine = true;
        need_switch_to_music = false;
    }
}

// ==================== 音频任务 ====================

//音频采样回调函数
static void onSample(int16_t left, int16_t right, int rate) {
    (void)rate;
    int16_t mono = (left + right) / 2;
    fft_sample_buf[fft_sample_idx++] = mono;
    if (fft_sample_idx >= FFT_SAMPLES) {
        fft_sample_idx = 0;
        for (uint16_t i = 0; i < FFT_SAMPLES; i++) {
            fft_real[i] = (double)fft_sample_buf[i];
            fft_imag[i] = 0.0;
        }
        fft_input_ready = true;
    }
}

// 音频处理任务
static void audioTask(void *pvParameters) {
    (void)pvParameters;
    out = new AudioOutputI2S();
    out->SetPinout(I2S_BCLK, I2S_LRCLK, I2S_DIN);
    out->SetGain(0.1f); // 初始增益，将被后续命令覆盖

    while (1) {
        if (playerStatus.taskQuitFlag) {
            if (mp3 && mp3->isRunning()) mp3->stop();
            if (mp3) { delete mp3; mp3 = nullptr; }
            if (audio_file) { delete audio_file; audio_file = nullptr; }
            if (out) { delete out; out = nullptr; }
            if (cmdQueue) xQueueReset(cmdQueue);
            vTaskDelete(NULL);
            break;
        }

        QueuedCommand qcmd;
        if (xQueueReceive(cmdQueue, &qcmd, 0) == pdTRUE) {
            switch (qcmd.cmd) {
                case CMD_OPEN_FILE:
                    if (mp3) { mp3->stop(); delete mp3; mp3 = nullptr; }
                    if (audio_file) { delete audio_file; audio_file = nullptr; }
                    audio_file = new AudioFileSourceSdFat(qcmd.param.filePath);
                    if (audio_file->isOpen()) {
                        playerStatus.fileOpened = true;
                        playerStatus.fileSizeBytes = audio_file->getSize();
                        playerStatus.currentPosBytes = 0;
                        mp3 = new AudioGeneratorMP3();
                        mp3->setSampleCallback(onSample);
                        mp3->begin(audio_file, out);
                        playerStatus.isPlaying = true;
                        playerStatus.isPaused = false;
                        playerStatus.openFailed = false; // 清除失败标志
                    } else {
                        // 文件打开失败处理
                        playerStatus.fileOpened = false;
                        playerStatus.openFailed = true;
                        strncpy(playerStatus.lastFailedPath, qcmd.param.filePath, sizeof(playerStatus.lastFailedPath) - 1);
                        playerStatus.lastFailedPath[sizeof(playerStatus.lastFailedPath) - 1] = '\0';
                        Serial.printf("音频文件打开失败: %s\n", qcmd.param.filePath);
                    }
                    break;
                case CMD_PLAY:
                    if (audio_file && audio_file->isOpen()) {
                         playerStatus.isPaused = false;
                         playerStatus.isPlaying = true;
                    }
                    break;
                case CMD_PAUSE:
                    if (playerStatus.isPlaying) playerStatus.isPaused = true;
                    break;
                case CMD_TOGGLE_PAUSE:
                     if (audio_file && audio_file->isOpen()) {
                        playerStatus.isPaused = !playerStatus.isPaused;
                        if (!playerStatus.isPaused && !mp3->isRunning()) {
                             mp3->begin(audio_file, out);
                             playerStatus.isPlaying = true;
                        }
                     }
                     break;
                case CMD_SEEK_BYTES:
                    if (audio_file && audio_file->isOpen()) {
                        if (qcmd.param.seekPos < playerStatus.fileSizeBytes) {
                            audio_file->seek(qcmd.param.seekPos, SEEK_SET);
                            playerStatus.currentPosBytes = qcmd.param.seekPos;
                        }
                    }
                    break;
                case CMD_SET_VOLUME:
                    if (out != nullptr) { // 必须检查
                        out->SetGain(qcmd.param.volume);
                    }
                    break;
                case CMD_QUIT_TASK:
                    playerStatus.taskQuitFlag = true;
                    break;
                default: break;
            }
        }

        if (playerStatus.isPlaying && !playerStatus.isPaused) {
            if (mp3 && mp3->isRunning()) {
                if (!mp3->loop()) {
                    // 歌曲结束
                    playerStatus.isPlaying = false;
                    playerStatus.isPaused = false;
                    playerStatus.songFinished = true; // 通知主线程
                    Serial.println("歌曲播放结束，通知主线程。");
                }
            }
        }

        if (fft_input_ready) {
            fft_input_ready = false;
            fft.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
            fft.Compute(FFT_FORWARD);
            fft.ComplexToMagnitude();
            static int bin_map[50] = {
                0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,  // 前20条逐点
                21,23,25,27,29,31,33,35,38,41,44,47,50,53,56,59,62,65,68,69 // 后30条步进
            };
            for (int i = 0; i < 30; i++) {
                int bin_idx = bin_map[i];
                if (bin_idx >= FFT_SAMPLES/2) bin_idx = FFT_SAMPLES/2 - 1;
                fft_magnitudes[i] = (float)fft_real[bin_idx];
            }
            fft_new_data = true;
        }

        if (audio_file && audio_file->isOpen()) {
            static uint32_t last_ui_update = 0;
            if (millis() - last_ui_update >= 100) { 
                last_ui_update = millis();
                playerStatus.currentPosBytes = audio_file->getPos();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// ==================== 导出函数 ====================

// 创建音乐播放器模块
void fs_create_music(lv_obj_t* container) {
    g_container = container;
    if (gpio_timer) { lv_timer_del(gpio_timer); gpio_timer = NULL; }

    last_gpio6_level = gpio_get_level(GPIO_NUM_6);
    stable_start_gpio6 = lv_tick_get();
    
    if (xSemaphoreTake(sd_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        last_sd_card_state = sd_card_inserted;
        xSemaphoreGive(sd_state_mutex);
    } else {
        last_sd_card_state = true;
    }

    gpio_timer = lv_timer_create(gpio_check_cb, TIMER_PERIOD, NULL);
    
    // 从文件选择开始
    g_file_selection_instance = file_selection_create(container, FILE_TYPE_MUSIC, music_file_selected_cb, NULL);
}

//清理音乐播放器模块资源
void fs_cleanup_music(void) {
    destroy_file_selection_list();
    if (gpio_timer) { lv_timer_del(gpio_timer); gpio_timer = NULL; }
}

//安全退出音频任务
void quitAudioTaskSafely() {
    if (audioTaskHandle != NULL) {
        // 设置退出标志位
        playerStatus.taskQuitFlag = true;
        
        //发送退出命令（防止任务阻塞在队列接收处）
        if (cmdQueue != NULL) {
            QueuedCommand qcmd = {CMD_QUIT_TASK, {0}};
            xQueueSend(cmdQueue, &qcmd, 0);
        }
        // 增加等待时间，并确保任务删除
        uint32_t waitStart = millis();
        // 给 300ms 时间让任务清理其内部逻辑
        while (eTaskGetState(audioTaskHandle) != eDeleted && (millis() - waitStart) < 300) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        audioTaskHandle = NULL;
    }

    //只有任务退出了，才删除队列
    if (cmdQueue != NULL) {
        vQueueDelete(cmdQueue);
        cmdQueue = NULL;
    }
}
//停止并释放音频资源
void stopAndReleaseResources() {
    if (out) { out->stop(); delete out; out = nullptr; }
    memset((void*)&playerStatus, 0, sizeof(PlayerStatus));
    fft_new_data = false;
    fft_sample_idx = 0;
    fft_input_ready = false;
    is_dragging_progress = false;
}