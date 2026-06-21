#ifndef FLAPPY_BIRD_UI_PORT_COMPAT_H
#define FLAPPY_BIRD_UI_PORT_COMPAT_H

#include "lvgl.h"

/* SquareLine project is generated for LVGL9.
 * This adapter keeps it buildable on LVGL8 used by current firmware. */
#if LVGL_VERSION_MAJOR < 9
#define lv_image_dsc_t lv_img_dsc_t
#define lv_image_create lv_img_create
#define lv_image_set_src lv_img_set_src
#define lv_image_set_scale lv_img_set_zoom
#define lv_image_get_scale lv_img_get_zoom
#define lv_image_set_rotation lv_img_set_angle
#define lv_image_get_rotation lv_img_get_angle
#define lv_button_create lv_btn_create

#define lv_screen_load_anim_t lv_scr_load_anim_t
#define lv_screen_load_anim lv_scr_load_anim
#define lv_display_get_default lv_disp_get_default
#define lv_disp_load_scr lv_scr_load

#define lv_obj_get_x_aligned lv_obj_get_x
#define lv_obj_get_y_aligned lv_obj_get_y

/* LVGL9 -> LVGL8 API rename compatibility */
#define lv_obj_send_event lv_event_send
#define lv_obj_remove_flag lv_obj_clear_flag
#define lv_obj_remove_state lv_obj_clear_state
#define lv_malloc lv_mem_alloc
#define lv_free lv_mem_free

/* Fallback to an enabled nearby font size in LVGL8 config */
#define lv_font_montserrat_22 lv_font_montserrat_20
#endif

#endif
