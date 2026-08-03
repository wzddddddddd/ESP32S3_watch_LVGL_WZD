# ESP32-S3 Low-Power Touch Watch Based on LVGL

> **重要声明：板子不是我做的，板子作者为 COCONUT_E。** 原作者 B 站视频： [BV1LPQQBAEbr](https://www.bilibili.com/video/BV1LPQQBAEbr)，开源硬件页面：[OSHWHub - Smart Watch Multi-Function Terminal](https://oshwhub.com/coconet/smart-watch-multi-function-termi)。

基于 `ESP32-S3R8`、`FreeRTOS` 与 `LVGL` 实现的低功耗触控智能手表原型项目。该项目围绕“小型 MCU 平台上实现可交互、可联网、可升级、可低功耗运行的图形化终端”这一目标展开，完成了表盘显示、触控交互、SD 卡多媒体浏览、小说阅读、时间同步、天气获取、双路径 OTA 升级以及休眠唤醒等完整功能链路。

这份仓库不仅包含主工程代码，还保留了 PCB、3D 结构、字体资源、演示素材以及答辩文档，适合从“软硬件一体化毕业设计 / 嵌入式项目作品集”的角度进行展示。

## Demo

视频演示链接将在这里补充：

- 在线演示视频：https://www.bilibili.com/video/BV1V8jt65EDW/?
- 功能速览图：见下方截图与 `docs/` 目录资源

> 后续我会在这里补上视频链接，方便面试官先看演示，再决定是否深入阅读代码或下载工程。

## Project Overview

这个项目的核心目标，是在资源受限的 ESP32-S3 平台上实现一个具有实际交互价值的手表系统原型，并验证以下能力：

- MCU 平台驱动图形界面与触控交互的可行性
- SD 卡本地资源管理与多媒体浏览能力
- Wi-Fi 联网后的时间同步、天气获取与远程升级能力
- 低功耗模式下的待机与唤醒策略设计
- 图形界面、后台任务、外设驱动、升级流程之间的系统协同

从仓库内容来看，主工程不仅是一个 LVGL 界面 Demo，而是一个相对完整的嵌入式应用原型，已经覆盖了 UI、存储、联网、RTC、升级、电源管理等典型模块。

## Core Features

### 1. 图形化手表界面

- 基于 `LVGL` 构建手表 UI
- 使用 `Gui Guider` 生成和组织部分界面资源
- 支持表盘、菜单、设置页、时间设置页、天气页、OTA 页等多页面交互
- 面向 240 x 284 分辨率触控屏进行界面适配

### 2. 触控与按键交互

- 触控芯片驱动：`CST816T`
- 屏幕驱动：`ST7789`
- 支持触控操作与实体按键配合使用
- 包含页面切换、小说翻页、视频选择、设置操作等交互流程

### 3. SD 卡资源管理与本地多媒体

- 基于 `FatFs` 实现 SD 卡文件访问
- 支持扫描和读取图片、小说、视频等本地资源
- 已实现图片浏览、小说阅读、视频资源列表与播放相关流程
- 主工程中可以看到对 `/sdcard/videos`、`/sdcard/novels` 等资源目录的处理逻辑

### 4. 时间系统与 RTC 守时

- 使用 `Wi-Fi + SNTP/NTP` 进行网络校时
- 将校时结果写入 `SD3078` RTC 芯片
- 支持断网后 RTC 持续守时
- 支持手动时间设置与联网自动同步两种路径

### 5. 天气获取

- 联网后可发起天气请求并在界面中展示结果
- 包含天气状态、温度、湿度、更新时间等信息展示逻辑
- 相关实现位于 `get_weather.c` 与天气界面生成代码中

### 6. 固件升级能力

项目中实现了两类升级方式：

- 本地升级：从 SD 卡读取 `bin` 文件，执行本地 OTA
- 联网升级：通过 OneNET OTA 相关模块执行远程升级

分区表中配置了双 OTA 应用分区：

- `ota_0`
- `ota_1`

这意味着项目不仅支持下载升级，还考虑到了嵌入式设备常见的 A/B 分区升级思路。

### 7. 低功耗与唤醒

- 支持 `light sleep`
- 支持 `deep sleep`
- 包含显示、触控、RTC、存储等外设在休眠前后的处理逻辑
- 适合展示“功能系统”与“功耗控制”之间的工程平衡

从仓库中的 `power_sleep.c`、`peripheral_sleep.c`、`program_flowchart.md` 以及功耗测试图表来看，这部分不仅实现了逻辑，还做了较完整的分析与验证。

## Technical Highlights

如果从面试视角看，这个项目最值得关注的点主要有以下几个：

- **软硬件协同完整**：不仅有应用代码，还有 PCB、3D 结构和资源组织
- **不是单一 Demo，而是系统原型**：界面、驱动、网络、升级、低功耗模块齐全
- **多任务协同明显**：LVGL 任务、存储任务、时间同步任务、联网与天气逻辑之间存在明确的协作关系
- **有产品化思路**：本地资源、在线同步、升级维护、休眠唤醒都更接近真实设备
- **保留了工程设计过程**：答辩资料、流程图、测试图、架构图对理解项目很有帮助

## System Architecture

从当前仓库结构和代码组织来看，主工程大致可以理解为以下层次：

### 应用层

- 手表 UI 页面
- 表盘、设置、天气、小说、图片、视频、升级等业务功能

### 中间调度层

- LVGL 消息队列
- 存储任务与资源扫描逻辑
- 界面事件与后台任务之间的消息传递

### 功能模块层

- Wi-Fi 管理
- NTP 时间同步
- RTC 时间服务
- 天气请求
- 本地 OTA / OneNET OTA
- 视频播放与 MJPEG 帧处理

### BSP / 驱动层

- ST7789 屏幕驱动
- CST816T 触控驱动
- SD3078 RTC 驱动
- SD 卡与 FatFs 接口
- 休眠与外设低功耗处理

## Repository Guide

这个仓库内容较多，建议按下面的顺序阅读：

### 1. 主工程代码

主工程位于：

`chu_kong_git/lvgl_display_test_2`

这是最值得优先阅读的部分，包含：

- `main/`：主任务、LVGL 调度、存储任务
- `components/bsp/`：屏幕、触控、RTC、SD 卡、Wi-Fi、天气、休眠等底层模块
- `components/ota/`：本地 OTA 与 OneNET OTA
- `components/GUI/`：界面生成代码与资源
- `docs/`：流程图、架构图、答辩图表、测试资料

### 2. PCB 设计资料

- `PCB/`

适合查看硬件设计与打板资料。

### 3. 3D 结构资料

- `3D/`

适合查看外壳、装配与结构优化过程。

### 4. 素材与资源

- `SD卡文件存放/`
- `ziku/`

包含部分本地资源、字体和多媒体相关素材。

### 5. 参考与扩展内容

- `youxi/`

包含一些 LVGL / 游戏相关参考内容，更偏资料和扩展示例，不是主线工程入口。

## Main Project Structure

以主工程 `chu_kong_git/lvgl_display_test_2` 为例，关键目录如下：

```text
chu_kong_git/lvgl_display_test_2
├─ main/                 # 主任务、LVGL 调度、消息处理、存储任务
├─ components/
│  ├─ bsp/               # 屏幕、触控、SD 卡、RTC、Wi-Fi、天气、低功耗
│  ├─ ota/               # 本地 OTA、OneNET OTA
│  ├─ GUI/               # GUI 资源与界面生成代码
│  ├─ jpeg/              # JPEG 处理相关模块
│  └─ lvgl/              # LVGL 组件代码
├─ docs/                 # 架构图、流程图、答辩图、测试资料
├─ partitions.csv        # OTA 双分区表
├─ sdkconfig             # 工程配置
└─ CMakeLists.txt
```

## Hardware / Software Stack

> **板子不是我做的，板子作者为 COCONUT_E。** 原作者 B 站视频： [BV1LPQQBAEbr](https://www.bilibili.com/video/BV1LPQQBAEbr)，开源硬件页面：[OSHWHub - Smart Watch Multi-Function Terminal](https://oshwhub.com/coconet/smart-watch-multi-function-termi)。

### Hardware

- MCU：`ESP32-S3R8`
- LCD：`ST7789`
- Touch：`CST816T`
- RTC：`SD3078`
- Storage：`SD Card`

### Software

- `ESP-IDF`
- `FreeRTOS`
- `LVGL`
- `FatFs`
- `esp_wifi`
- `esp_http_client`
- `NVS`

## Build and Flash

主工程使用 `ESP-IDF` 构建，工程入口目录为：

```bash
cd chu_kong_git/lvgl_display_test_2
```

常见命令如下：

```bash
idf.py build
idf.py flash
idf.py monitor
```

如果串口和目标板已经连接，也可以直接：

```bash
idf.py -p <PORT> flash monitor
```

### Build Notes

- 分区表位于 `partitions.csv`，已配置双 OTA 应用分区
- `sdkconfig` 已保留在仓库中，可作为工程配置参考
- 项目依赖屏幕、触控、RTC、SD 卡等实际硬件，若仅在 PC 上阅读代码，重点建议查看模块接口和流程图

## Recommended Reading Path for Interviewers

如果你是面试官，建议按下面顺序快速了解本项目：

1. 阅读本 README，建立整体认知
2. 查看 `chu_kong_git/lvgl_display_test_2/docs/` 下的架构图、流程图和测试图
3. 查看 `main/lvgl_display.c` 与 `main/storage_worker.c`，理解任务与消息协作
4. 查看 `components/bsp/`，了解屏幕、触控、RTC、Wi-Fi、天气、休眠等模块
5. 查看 `components/ota/`，了解升级设计
6. 如需了解硬件实现，再查看 `PCB/` 与 `3D/`

## Project Value

这个项目的价值不在于单个页面或单个驱动，而在于它把嵌入式图形界面、存储、多媒体、联网、升级与低功耗整合到了一个完整原型中。对于毕业设计、求职作品集或嵌入式 GUI / IoT 方向面试来说，这类项目能够比较清楚地展示以下能力：

- 独立完成 MCU 应用系统搭建
- 具备 GUI 与嵌入式交互设计能力
- 能处理多模块协同与任务调度
- 理解联网、升级、低功耗等真实设备问题
- 具备从软件到硬件资料整理与展示的完整工程意识

## Screenshots and Documents

仓库中已经保留了一批可辅助阅读的图表和文档，位于：

- `chu_kong_git/lvgl_display_test_2/docs/program_flowchart.md`
- `chu_kong_git/lvgl_display_test_2/docs/主架构.png`
- `chu_kong_git/lvgl_display_test_2/docs/software_architecture_layers_largefont.png`
- `chu_kong_git/lvgl_display_test_2/docs/time_sync_layers_largefont.png`
- `chu_kong_git/lvgl_display_test_2/docs/OTA架构.png`
- `chu_kong_git/lvgl_display_test_2/docs/power_test/`

后续可以继续在 README 中插入：

- 演示视频链接
- 表盘 / 设置页 / 天气页截图
- 功耗测试结果摘要
- 系统架构图缩略图

## Notes

> **板子不是我做的，板子作者为 COCONUT_E。** 原作者 B 站视频： [BV1LPQQBAEbr](https://www.bilibili.com/video/BV1LPQQBAEbr)，开源硬件页面：[OSHWHub - Smart Watch Multi-Function Terminal](https://oshwhub.com/coconet/smart-watch-multi-function-termi)。

- 仓库当前保留了较多过程性文件、素材和参考内容，这是项目开发痕迹的一部分
- 若后续希望进一步提升仓库专业度，可以再做一次目录精简，把主工程、资料、素材分层整理
- 当前 README 优先服务“展示项目能力”和“帮助面试官快速理解”，后续我也可以继续帮你补成中英双语版，或者加上视频封面与项目亮点对比图
