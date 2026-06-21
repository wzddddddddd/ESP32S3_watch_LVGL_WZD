#include "lvgl.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include "gui_guider.h"
#include "video_player.h"
#include "lvgl_display.h"
#include "setup_scr_video_list.h"
#include "setup_scr_video_player.h"

#define MOUNT_POINT "/sdcard"

char g_video_filepath[512];
video_format_t g_video_format;

static void video_list_item_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    lv_obj_t *btn = lv_event_get_current_target(e);
    const char *file_name = lv_list_get_btn_text(guider_ui.video_list_list, btn);
    if (file_name == NULL || file_name[0] == '\0') return;
    ESP_LOGI("VIDEO_LIST", "select video name: %s", file_name);
    lvgl_msg_send(LVGL_MSG_VIDEO_OPEN_REQ, 0, file_name);
}

static void video_list_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            ui_load_scr_animation(&guider_ui,
                &guider_ui.menu_screen, guider_ui.menu_screen_del,
                &guider_ui.video_list_del, setup_scr_menu_screen,
                LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
        }
    }
}

void events_init_video_list(lv_ui *ui)
{
    lv_obj_add_event_cb(ui->video_list, video_list_event_handler, LV_EVENT_ALL, ui);

    for (int i = 0; i < _LIST_NUMBER; i++) {
        if (ui->video_list_list_item[i] == NULL) continue;
        lv_obj_add_event_cb(
            ui->video_list_list_item[i],
            video_list_item_handler,
            LV_EVENT_CLICKED,
            NULL
        );
    }
}
