/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "events_init.h"
#include <stdio.h>
#include "lvgl.h"

#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "freemaster_client.h"
#endif


static void clock_screen_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        switch(dir) {
        case LV_DIR_LEFT:
        {
            lv_indev_wait_release(lv_indev_get_act());
            ui_load_scr_animation(&guider_ui, &guider_ui.menu_screen, guider_ui.menu_screen_del, &guider_ui.clock_screen_del, setup_scr_menu_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false, true);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

static void clock_screen_cont_2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_SCROLL_END:
    {
        break;
    }
    default:
        break;
    }
}

void events_init_clock_screen (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->clock_screen, clock_screen_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->clock_screen_cont_2, clock_screen_cont_2_event_handler, LV_EVENT_ALL, ui);
}

static void menu_screen_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        switch(dir) {
        case LV_DIR_RIGHT:
        {
            lv_indev_wait_release(lv_indev_get_act());
            ui_load_scr_animation(&guider_ui, &guider_ui.clock_screen, guider_ui.clock_screen_del, &guider_ui.menu_screen_del, setup_scr_clock_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 200, false, true);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

static void menu_screen_list_1_item0_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.novel_list, guider_ui.novel_list_del, &guider_ui.menu_screen_del, setup_scr_novel_list, LV_SCR_LOAD_ANIM_NONE, 200, 0, false, true);
        break;
    }
    default:
        break;
    }
}

static void menu_screen_list_1_item2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.setting_screen, guider_ui.setting_screen_del, &guider_ui.menu_screen_del, setup_scr_setting_screen, LV_SCR_LOAD_ANIM_NONE, 200, 0, false, true);
        break;
    }
    default:
        break;
    }
}

void events_init_menu_screen (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->menu_screen, menu_screen_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->menu_screen_list_1_item0, menu_screen_list_1_item0_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->menu_screen_list_1_item2, menu_screen_list_1_item2_event_handler, LV_EVENT_ALL, ui);
}

static void novel_display_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        switch(dir) {
        case LV_DIR_RIGHT:
        {
            lv_indev_wait_release(lv_indev_get_act());
            ui_load_scr_animation(&guider_ui, &guider_ui.clock_screen, guider_ui.clock_screen_del, &guider_ui.novel_display_del, setup_scr_clock_screen, LV_SCR_LOAD_ANIM_NONE, 200, 0, false, true);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

void events_init_novel_display (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->novel_display, novel_display_event_handler, LV_EVENT_ALL, ui);
}

static void novel_list_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        switch(dir) {
        case LV_DIR_RIGHT:
        {
            lv_indev_wait_release(lv_indev_get_act());
            ui_load_scr_animation(&guider_ui, &guider_ui.menu_screen, guider_ui.menu_screen_del, &guider_ui.novel_list_del, setup_scr_menu_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false, true);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

static void novel_list_list_1_item0_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.novel_display, guider_ui.novel_display_del, &guider_ui.novel_list_del, setup_scr_novel_display, LV_SCR_LOAD_ANIM_NONE, 200, 0, false, true);
        break;
    }
    default:
        break;
    }
}

void events_init_novel_list (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->novel_list, novel_list_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->novel_list_list_1_item0, novel_list_list_1_item0_event_handler, LV_EVENT_ALL, ui);
}

static void setting_screen_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        switch(dir) {
        case LV_DIR_RIGHT:
        {
            lv_indev_wait_release(lv_indev_get_act());
            ui_load_scr_animation(&guider_ui, &guider_ui.menu_screen, guider_ui.menu_screen_del, &guider_ui.setting_screen_del, setup_scr_menu_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false, true);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

void events_init_setting_screen (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->setting_screen, setting_screen_event_handler, LV_EVENT_ALL, ui);
}


void events_init(lv_ui *ui)
{

}
