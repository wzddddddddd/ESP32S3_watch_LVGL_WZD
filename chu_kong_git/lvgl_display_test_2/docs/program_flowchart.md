# ESP32 LVGL 智能触控终端程序流程图

本文档根据当前工程源码整理，重点覆盖 `main`、`components/bsp`、`components/GUI`、`components/ota` 中的业务逻辑。`components/lvgl` 属于 LVGL 第三方图形库源码，流程图中只把它作为图形框架调用，不展开其内部实现。

## 1. 程序总体流程

```mermaid
flowchart TD
    A([系统上电/复位]) --> B["ESP-IDF 启动运行 app_main()"]
    B --> C["低功耗启动恢复 power_sleep_boot_init()"]
    C --> D["创建 event_group 和 lvgl_runtime_event_group"]
    D --> E["创建 LVGL 消息队列 lvgl_msg_queue"]
    E --> F["初始化 SD 卡、按键、硬件定时器、存储工作线程、低功耗管理"]
    F --> G["初始化 NVS"]
    G --> H{"NVS 是否需要擦除重建?"}
    H -- 是 --> I["nvs_flash_erase() 后重新 nvs_flash_init()"]
    H -- 否 --> J["继续"]
    I --> J
    J --> K["初始化 TCP/IP 网络层 esp_netif_init()"]
    K --> L["创建 sntp_time_task"]
    L --> M["创建 sntp_interval_task"]
    M --> N["创建 lvgl_diaplay_task"]
    N --> O["初始化 WiFi 管理器, 默认关闭 WiFi"]
    O --> P["初始化天气服务 weather_service_init()"]
    P --> Q["进入 app_main 主循环"]
    Q --> R["扫描实体按键 Key_GetNum()"]
    R --> S{"按键值?"}
    S -- 确认键 3 --> T["发送 LVGL_MSG_KEY_CONFIRM"]
    S -- 上键 2 --> U["发送 LVGL_MSG_KEY_UP"]
    S -- 下键 1 --> V["发送 LVGL_MSG_KEY_DOWN"]
    S -- 无按键 --> W["延时 50 ms"]
    T --> W
    U --> W
    V --> W
    W --> R
```

## 2. 任务与模块关系

```mermaid
flowchart LR
    subgraph APP["主程序 main/lvgl_display.c"]
        A1["app_main()"]
        A2["lvgl_msg_queue"]
        A3["lvgl_diaplay_task"]
    end

    subgraph UI["GUI 组件 components/GUI"]
        B1["setup_ui() 创建初始时钟界面"]
        B2["events_init_*.c 处理触摸/手势/点击"]
        B3["ui_load_scr_animation() 切换界面"]
    end

    subgraph BSP["BSP 外设 components/bsp"]
        C1["ST7789 LCD 显示"]
        C2["CST816T 触摸"]
        C3["SD 卡/文件系统"]
        C4["WiFi 管理"]
        C5["SNTP/RTC 时间"]
        C6["天气服务"]
        C7["视频播放"]
        C8["低功耗"]
    end

    subgraph STORAGE["存储工作线程 main/storage_worker.c"]
        D1["扫描小说/视频文件"]
        D2["打开小说并读取分页"]
        D3["解析视频文件路径"]
        D4["睡眠前保存阅读进度"]
    end

    subgraph OTA["升级模块 components/ota"]
        E1["本地 OTA"]
        E2["OneNET OTA"]
        E3["分区切换/重启"]
    end

    A1 --> A2
    A1 --> A3
    A3 --> B1
    A3 --> B2
    B2 --> A2
    A2 --> A3
    A3 --> B3
    A3 --> C1
    A3 --> C2
    A3 --> D1
    A3 --> D2
    A3 --> D3
    D1 --> A2
    D2 --> A2
    D3 --> A2
    B2 --> C4
    C4 --> A2
    C5 --> A2
    C6 --> A2
    B2 --> E1
    B2 --> E2
    E1 --> A2
    E2 --> A2
    C8 --> A3
    C8 --> C1
    C8 --> C4
    C8 --> D4
```

## 3. LVGL 显示任务流程

```mermaid
flowchart TD
    A([lvgl_diaplay_task 启动]) --> B{"消息队列是否已创建?"}
    B -- 否 --> C["lvgl_msg_queue_init()"]
    B -- 是 --> D["记录 LVGL 任务句柄"]
    C --> D
    D --> E["lv_port_init()"]
    E --> F["lv_init()"]
    F --> G["ST7789 LCD 初始化"]
    G --> H["CST816T 触摸初始化"]
    H --> I["RTC 初始化, 从 RTC 同步系统时间"]
    I --> J["注册 LVGL 显示驱动 lv_disp_init()"]
    J --> K["注册 LVGL 输入驱动 lv_indev_init()"]
    K --> L["创建 LVGL tick 定时器"]
    L --> M["打开 LCD 背光"]
    M --> N["初始化 LVGL 文件系统 my_fs_init()"]
    N --> O["设置图片缓存"]
    O --> P["setup_ui() 创建并加载时钟界面"]
    P --> Q["创建 500 ms 时间刷新定时器"]
    Q --> R["置位 LVGL_RT_BIT_TASK_READY"]
    R --> S{{"LVGL 主循环"}}
    S --> T{"是否处于睡眠 g_is_sleeping?"}
    T -- 是 --> U["置位 LVGL_RT_BIT_PAUSED_ACK"]
    U --> V["健康诊断 lvgl_runtime_health_tick()"]
    V --> W["延时 20 ms"]
    W --> S
    T -- 否 --> X["清除 PAUSED_ACK"]
    X --> Y["处理 lvgl_msg_queue 中所有待处理消息"]
    Y --> Z["调用 lv_task_handler() 执行 LVGL 刷新和事件"]
    Z --> AA{"本轮耗时是否超过 200 ms?"}
    AA -- 是 --> AB["记录 UI loop overrun 诊断"]
    AA -- 否 --> AC["计算下一次延时 10-500 ms"]
    AB --> AC
    AC --> AD["周期健康诊断"]
    AD --> AE["vTaskDelay(time_till_next)"]
    AE --> S
```

## 4. LVGL 消息队列处理流程

```mermaid
flowchart TD
    A([lvgl_process_msg_queue]) --> B{"队列是否有消息?"}
    B -- 否 --> Z([返回 LVGL 主循环])
    B -- 是 --> C["取出 lvgl_msg_t"]
    C --> D{"消息类型"}

    D -- WiFi 已连接 --> E["更新 WiFi 页面和快捷面板状态"]
    D -- WiFi 已断开 --> F["显示 Disconnected, 快捷面板置为关闭"]

    D -- 按键确认 --> G["key_nav_confirm_selected()"]
    D -- 按键上/下 --> H{"当前是否小说阅读页?"}
    H -- 是 --> I["请求上一页/下一页"]
    H -- 否 --> J["移动列表选中项"]

    D -- NTP 状态 --> K["刷新快捷时间同步状态"]
    D -- 天气状态/更新 --> L["刷新天气页面数据"]

    D -- OTA 状态/进度 --> M["更新 OneNET 或本地 OTA 状态窗口"]
    D -- OTA 完成 --> N["关闭本地 OTA 弹窗并显示成功结果"]

    D -- 小说列表刷新请求 --> O["storage_request_novel_list_refresh()"]
    D -- 视频列表刷新请求 --> P["storage_request_video_list_refresh()"]
    D -- 小说打开请求 --> Q["storage_request_novel_open_by_name/path()"]
    D -- 小说打开成功 --> R["切换到 novel_display 并请求读取当前页"]
    D -- 小说翻页请求 --> S["storage_request_novel_page_next/prev/sync()"]
    D -- 小说页准备好 --> T["取出文本并更新阅读标签"]

    D -- 视频打开请求 --> U["storage_request_video_resolve_by_name()"]
    D -- 视频路径解析成功 --> V["记录路径和格式, 切换到 video_player"]
    D -- 视频停止请求 --> W["video_player_stop_async()"]
    D -- 视频帧到达 --> X["设置 lv_img 图像源并标记帧已显示"]
    D -- 返回视频列表 --> Y["切回 video_list"]

    E --> B
    F --> B
    G --> B
    I --> B
    J --> B
    K --> B
    L --> B
    M --> B
    N --> B
    O --> B
    P --> B
    Q --> B
    R --> B
    S --> B
    T --> B
    U --> B
    V --> B
    W --> B
    X --> B
    Y --> B
```

## 5. 界面导航主流程

```mermaid
flowchart TD
    A([开机默认界面]) --> B["clock_screen 时钟界面"]
    B --> C{"时钟界面手势"}
    C -- 上滑/进入菜单 --> D["menu_screen 主菜单"]
    C -- 下滑/快捷入口 --> E["快捷面板: WiFi、同步时间、亮度、天气"]

    D --> F{"点击菜单项"}
    F -- 小说 --> G["novel_list 小说列表"]
    F -- 图片 --> H["screen_img_list 图片列表"]
    F -- 设置 --> I["setting_screen 设置"]
    F -- 视频 --> J["video_list 视频列表"]
    F -- 游戏 --> K["screen_game 游戏列表"]
    D -- 下滑返回 --> B

    G --> G1["加载页面时发送 NOVEL_LIST_REFRESH_REQ"]
    G1 --> G2["storage_worker 扫描 /sdcard/novels"]
    G2 --> G3["列表渲染"]
    G3 --> G4{"点击小说?"}
    G4 -- 是 --> G5["发送 NOVEL_OPEN_REQ"]
    G5 --> G6["打开小说并切换 novel_display"]
    G6 --> G7["上/下键或翻页消息读取上一页/下一页"]
    G7 --> G6
    G -- 右滑 --> D
    G6 -- 右滑 --> G

    H --> H1["点击图片后 show_png_fast() 显示图片"]
    H1 --> H
    H -- 右滑 --> D

    J --> J1["加载页面时发送 VIDEO_LIST_REFRESH_REQ"]
    J1 --> J2["storage_worker 扫描 /sdcard/videos"]
    J2 --> J3["点击视频发送 VIDEO_OPEN_REQ"]
    J3 --> J4["解析路径和格式"]
    J4 --> J5["切换 video_player 并启动 video_player_start()"]
    J5 --> J6["视频任务解码并投递 VIDEO_FRAME"]
    J6 --> J5
    J5 -- 返回/停止 --> J

    K --> K1{"选择游戏"}
    K1 -- 2048 --> K2["open_game_2048_screen()"]
    K1 -- 记忆游戏 --> K3["open_game_memory_screen()"]
    K1 -- 贪吃蛇 --> K4["open_game_snake_screen()"]
    K1 -- Flappy Bird --> K5["open_game_flappy_screen()"]
    K2 --> K
    K3 --> K
    K4 --> K
    K5 --> K
    K -- 右滑 --> D

    I --> I1{"设置项"}
    I1 -- 时间设置 --> I2["screen_time_set"]
    I1 -- WiFi 设置 --> I3["screen_wifi_set"]
    I1 -- OTA 升级 --> I4["screen_ota"]
    I2 -- 右滑 --> I
    I3 -- 右滑 --> I
    I4 -- 右滑 --> I
    I -- 右滑 --> D
```

## 6. 存储工作线程流程

```mermaid
flowchart TD
    A([storage_worker_init]) --> B["创建命令队列、互斥锁、睡眠信号量"]
    B --> C["分配小说缓存、视频缓存、扫描临时缓存"]
    C --> D["创建 worker_task"]
    D --> E{{"worker_task 循环等待命令"}}
    E --> F{"命令类型"}

    F -- 扫描小说 --> G["scan_dir('/sdcard/novels', '.txt')"]
    G --> H["更新小说缓存"]
    H --> I["发送 LVGL_MSG_NOVEL_LIST_READY"]

    F -- 扫描视频 --> J["scan_dir('/sdcard/videos', 视频后缀集合)"]
    J --> K["更新视频缓存"]
    K --> L["发送 LVGL_MSG_VIDEO_LIST_READY"]

    F -- 按名称打开小说 --> M{"缓存中是否找到文件?"}
    M -- 否 --> N["重新扫描小说目录"]
    N --> O{"是否找到?"}
    M -- 是 --> P["handle_novel_open(path)"]
    O -- 是 --> P
    O -- 否 --> Q["发送 LVGL_MSG_NOVEL_OPEN_ERROR"]
    P --> R["关闭旧 fp, fopen 新小说"]
    R --> S["加载阅读进度 novel_progress_load()"]
    S --> T["发送 LVGL_MSG_NOVEL_OPEN_READY"]

    F -- 小说分页 --> U["novel_read_at_offset/next_page/prev_page"]
    U --> V{"读取是否成功?"}
    V -- 是 --> W["缓存页面文本和偏移"]
    W --> X["发送 LVGL_MSG_NOVEL_PAGE_READY"]
    V -- 否 --> Q

    F -- 关闭小说/睡眠准备 --> Y["保存阅读进度并 fclose(fp)"]
    Y --> Z["必要时通知 UI 或释放睡眠信号量"]

    F -- 解析视频名称 --> AA{"缓存中是否找到视频?"}
    AA -- 否 --> AB["重新扫描视频目录"]
    AB --> AC{"是否找到?"}
    AA -- 是 --> AD["保存视频路径并识别格式"]
    AC -- 是 --> AD
    AC -- 否 --> AE["发送 LVGL_MSG_VIDEO_OPEN_ERROR"]
    AD --> AF["发送 LVGL_MSG_VIDEO_OPEN_READY"]

    I --> E
    L --> E
    T --> E
    Q --> E
    X --> E
    Z --> E
    AE --> E
    AF --> E
```

## 7. 视频播放流程

```mermaid
flowchart TD
    A([点击视频文件]) --> B["发送 LVGL_MSG_VIDEO_OPEN_REQ"]
    B --> C["storage_worker 解析完整路径和格式"]
    C --> D["发送 LVGL_MSG_VIDEO_OPEN_READY"]
    D --> E["LVGL 切换到 video_player 页面"]
    E --> F["setup_scr_video_player() 调用 video_player_start()"]
    F --> G["创建控制锁、状态锁、退出信号量"]
    G --> H{"是否已有视频正在播放?"}
    H -- 是 --> I([直接返回])
    H -- 否 --> J["分配多帧缓冲区"]
    J --> K["创建 video_task"]
    K --> L{"视频格式"}
    L -- RGB565 原始帧 --> M["循环读取一帧 RGB565"]
    L -- MJPEG/其他 --> N["jpeg_frame_start() 取 JPEG 帧"]
    N --> O["jpg2rgb565() 解码成 RGB565"]
    M --> P["publish_filled_frame()"]
    O --> P
    P --> Q["发送 LVGL_MSG_VIDEO_FRAME"]
    Q --> R["LVGL 线程更新 video_player_img"]
    R --> S["video_player_mark_frame_presented() 释放旧前台帧"]
    S --> T{"收到停止/暂停/睡眠?"}
    T -- 暂停 --> U["短延时等待恢复"]
    U --> T
    T -- 停止或睡眠 --> V["退出解码循环, 释放状态, 通知退出信号量"]
    T -- 继续 --> L
    V --> W([播放结束])
```

## 8. WiFi、时间和天气流程

```mermaid
flowchart TD
    A([WiFi 管理初始化]) --> B["创建默认事件循环和 STA 网卡"]
    B --> C["注册 WIFI_EVENT 和 IP_EVENT 回调"]
    C --> D["esp_wifi_init/start, 随后 app_main 默认 wifi_manager_stop()"]
    D --> E{"用户打开 WiFi 或输入 SSID 连接?"}
    E -- 快捷面板打开 --> F["quick_wifi_worker_task 调用 wifi_manager_start()"]
    E -- WiFi 设置页连接 --> G["wifi_manager_connect(ssid, pwd)"]
    F --> H["WIFI_EVENT_STA_START 后 esp_wifi_connect()"]
    G --> H
    H --> I{"是否获取 IP?"}
    I -- 是 --> J["IP_EVENT_STA_GOT_IP"]
    J --> K["wifi_state_handler(WIFI_STATE_CONNECTED)"]
    K --> L["发送 LVGL_MSG_WIFI_CONNECTED"]
    K --> M["trigger_ntp_sync() 请求 SNTP 同步"]
    K --> N["weather_request_sync() 请求天气更新"]
    I -- 否/断开 --> O["WIFI_EVENT_STA_DISCONNECTED"]
    O --> P{"重连次数是否小于 6?"}
    P -- 是 --> H
    P -- 否 --> Q["发送 LVGL_MSG_WIFI_DISCONNECTED"]

    M --> R["sntp_interval_task 收到触发"]
    R --> S["SNTP 同步系统时间"]
    S --> T["同步 RTC, 更新 timeinfo 缓存"]
    T --> U["发送 LVGL_MSG_NTP_SYNC_STATUS"]

    N --> V["weather_task 发送 HTTP 请求"]
    V --> W{"JSON 解析是否成功?"}
    W -- 是 --> X["更新 weather_snapshot"]
    X --> Y["发送 LVGL_MSG_WEATHER_UPDATED"]
    W -- 否 --> Z["发送 LVGL_MSG_WEATHER_STATUS 错误状态"]
```

## 9. OTA 升级流程

```mermaid
flowchart TD
    A([设置页进入 OTA 菜单]) --> B{"选择升级方式"}

    B -- OneNET OTA --> C["进入 screen_ota_onenet"]
    C --> D["点击 Check & upgrade"]
    D --> E["onenet_ota_start() 创建 OneNET OTA 任务"]
    E --> F["上报当前版本"]
    F --> G{"云端是否有新版本?"}
    G -- 否 --> H["发送 OTA 状态: 无新版本/失败"]
    G -- 是 --> I["上报升级状态"]
    I --> J["esp_https_ota 下载固件"]
    J --> K{"下载是否成功?"}
    K -- 否 --> H
    K -- 是 --> L["提示下载成功, 等待用户点击跳转"]
    L --> M["onenet_ota_jump_and_restart() 切换分区并重启"]

    B -- 本地 OTA --> N["进入 screen_ota_local"]
    N --> O["列出 SD 卡固件文件"]
    O --> P["点击固件文件"]
    P --> Q["local_ota_start(file_path) 创建 local_ota_task"]
    Q --> R["打开固件文件, 获取大小"]
    R --> S["获取运行分区和下一 OTA 分区"]
    S --> T["擦除目标分区并 esp_ota_begin()"]
    T --> U["循环 fread 文件块"]
    U --> V{"是否取消/读写失败?"}
    V -- 是 --> W["esp_ota_abort(), 发送失败/取消"]
    V -- 否 --> X["esp_ota_write() 写入 Flash"]
    X --> Y["发送进度 LVGL_MSG_OTA_STATUS/PROGRESS"]
    Y --> Z{"文件是否写完?"}
    Z -- 否 --> U
    Z -- 是 --> AA["esp_ota_end() 校验"]
    AA --> AB{"校验是否成功?"}
    AB -- 否 --> W
    AB -- 是 --> AC["发送 OTA_COMPLETE SUCCESS"]
    AC --> AD["UI 显示成功面板"]
    AD --> AE{"用户选择"}
    AE -- 跳转 --> AF["local_ota_switch_partition() 并重启"]
    AE -- 退出 --> AG["返回 OTA 菜单"]

    B -- 分区切换 --> AH["进入 screen_ota_switch"]
    AH --> AI["点击切换按钮"]
    AI --> AF
```

## 10. 低功耗流程

```mermaid
flowchart TD
    A([power_sleep_init]) --> B["记录最后活动时间"]
    B --> C["创建 power_sleep_task"]
    C --> D{{"周期检查空闲时间"}}
    D --> E{"是否请求深睡眠?"}
    E -- 是 --> F["power_sleep_enter_deep_sleep()"]
    E -- 否 --> G{"是否请求浅睡眠或空闲超时?"}
    G -- 否 --> D
    G -- 是 --> H["power_sleep_enter_light_sleep()"]

    H --> I["设置 g_is_sleeping = true"]
    I --> J["等待 LVGL_PAUSED_ACK"]
    J --> K{"视频是否播放中?"}
    K -- 是 --> L["video_player_stop_wait() 并标记醒来后返回视频列表"]
    K -- 否 --> M["关闭背光和 LCD"]
    L --> M
    M --> N["记录 WiFi 状态并关闭 WiFi"]
    N --> O["外设准备浅睡眠, 配置按键/触摸唤醒"]
    O --> P["esp_light_sleep_start()"]
    P --> Q["唤醒后重初始化触摸/LCD/背光"]
    Q --> R{"睡前 WiFi 是否开启?"}
    R -- 是 --> S["wifi_manager_start()"]
    R -- 否 --> T["恢复 LVGL 运行"]
    S --> T
    T --> U{"是否需要返回视频列表?"}
    U -- 是 --> V["发送 LVGL_MSG_RETURN_TO_VIDEO_LIST"]
    U -- 否 --> W["重置睡眠计时"]
    V --> W
    W --> D

    F --> X["设置 g_is_sleeping = true 并等待 LVGL 暂停"]
    X --> Y["停止视频, storage_prepare_for_sleep() 保存阅读进度"]
    Y --> Z["关闭背光/LCD/WiFi/外设/I2C/SD 卡"]
    Z --> AA["等待唤醒键释放, 配置 EXT0 唤醒"]
    AA --> AB["esp_deep_sleep_start()"]
    AB --> AC([深睡眠后重新上电流程])
```

## 11. 关键源码对应关系

| 功能 | 主要源码 |
|---|---|
| 程序入口、任务创建、LVGL 消息处理 | `main/lvgl_display.c` |
| LVGL 显示/触摸移植 | `main/lv_port.c` |
| 小说/视频扫描和文件异步处理 | `main/storage_worker.c` |
| UI 页面创建 | `components/GUI/generated/setup_scr_*.c` |
| UI 触摸、手势、按钮事件 | `components/GUI/generated/events_init*.c` |
| LCD 驱动 | `components/bsp/st7789_driver.c` |
| 触摸驱动 | `components/bsp/cst816t_driver.c` |
| SD 卡与小说分页 | `components/bsp/SD_card.c` |
| WiFi 状态管理 | `components/bsp/wifi_manager.c` |
| SNTP/RTC 时间 | `components/bsp/ntp_time.c`, `components/bsp/rtc_time_service.c`, `components/bsp/sd3078.c` |
| 天气请求 | `components/bsp/get_weather.c` |
| 视频播放/解码 | `components/bsp/video_player.c`, `components/bsp/mjpeg_frame.c` |
| 低功耗 | `components/bsp/power_sleep.c`, `components/bsp/peripheral_sleep.c` |
| 本地 OTA | `components/ota/local_ota.c` |
| OneNET OTA | `components/ota/onenet_ota.c`, `components/ota/onenet_mqtt.c`, `components/ota/onenet_dm.c` |

