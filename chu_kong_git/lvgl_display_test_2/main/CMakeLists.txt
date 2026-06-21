idf_component_register(
    SRCS "lvgl_display.c" "lv_port.c" "storage_worker.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl GUI bsp ota
)
