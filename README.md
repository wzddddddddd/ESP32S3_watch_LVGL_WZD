# ESP32S3_watch_LVGL_WZD
基于 ESP32-S3R8开发的LVGL手表。系统集成SPI触摸屏、 SD卡、SD3078，实现了精准计时、视频与图像显示、小 说阅读、本地与OTA双重升级以及益智游戏游玩。 1、基于 ESP32-S3 R8 开发，移植 LVGL 图形库，通过 Gui Guider 与 AI 协作完成 UI 交互设计 。 2、实现基于 FatFs 的 SD 卡文件检索，支持png图像与mjpeg视频及电子书阅读 。 3、使用wifi进行时间同步，将时间写入sd3078，并连接wifi就同步，做到精准计时，也可手动完成时间设置 4、使用OTA与SD卡bin文件进行固件升级，并做多分区下载 5、两种低功耗，待机模式进入light sleep通过点击按键和屏幕唤醒，关机模式进入deep sleep按下确认键唤醒。
