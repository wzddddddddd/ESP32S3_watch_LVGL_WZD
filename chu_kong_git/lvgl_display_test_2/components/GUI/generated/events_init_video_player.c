#include "lvgl.h"
#include "gui_guider.h"
#include "video_player.h"
#include "lvgl_display.h"
#include "setup_scr_video_list.h"
#include "setup_scr_video_player.h"

static void video_player_touch_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        video_player_toggle_pause();
    }
}

static void video_player_close_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (lvgl_msg_send_nonblocking(LVGL_MSG_VIDEO_STOP_REQ, 0, NULL) == 0) {
            video_player_stop_async();
        }
        if (guider_ui.video_player_img != NULL) {
            lv_img_set_src(guider_ui.video_player_img, NULL);
        }
        ui_load_scr_animation(&guider_ui,
            &guider_ui.video_list, guider_ui.video_list_del,
            &guider_ui.video_player_del, setup_scr_video_list,
            LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
    }
}

void events_init_video_player(lv_ui *ui)
{
    lv_obj_add_event_cb(ui->video_player_btn_touch, video_player_touch_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->video_player_btn_close, video_player_close_handler, LV_EVENT_ALL, ui);
}
