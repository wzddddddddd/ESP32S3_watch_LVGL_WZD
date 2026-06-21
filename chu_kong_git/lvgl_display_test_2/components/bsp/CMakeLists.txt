idf_component_register(
    SRCS "My_timer.c" "ui_mjpeg_player.c" "mjpeg_frame.c" "video_player.c" "power_sleep.c" "peripheral_sleep.c" "img_display.c" "novel_progress.c" "SD_card.c" "st7789_driver.c" "cst816t_driver.c"  "wifi_manager.c" "ntp_time.c" "get_weather.c"
        "i2c_bus_lock.c" "sd3078.c" "rtc_time_service.c"
        "novel_progress.c" "img_display.c"
    INCLUDE_DIRS    "."  "${CMAKE_SOURCE_DIR}/main"
    REQUIRES     esp_lcd  
                 fatfs      
                 esp_wifi   
                 esp_http_client
                 json   
                 lvgl
                 jpeg
                 GUI
                 nvs_flash         
)
