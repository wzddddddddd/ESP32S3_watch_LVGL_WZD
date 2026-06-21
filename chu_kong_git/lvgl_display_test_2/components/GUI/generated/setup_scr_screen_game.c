#include "lvgl.h"
#include <stdio.h>

#include "events_init.h"
#include "gui_guider.h"
#include "widgets_init.h"

void setup_scr_screen_game(lv_ui *ui)
{
    ui->screen_game = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_game, 240, 284);
    lv_obj_set_scrollbar_mode(ui->screen_game, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(ui->screen_game, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_game, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    ui->screen_game_list_1 = lv_list_create(ui->screen_game);
    lv_obj_set_pos(ui->screen_game_list_1, 0, 0);
    lv_obj_set_size(ui->screen_game_list_1, 240, 284);
    lv_obj_set_scrollbar_mode(ui->screen_game_list_1, LV_SCROLLBAR_MODE_OFF);

    for (int i = 0; i < _LIST_NUMBER; i++) {
        ui->screen_game_list_1_item[i] = NULL;
    }

    ui->screen_game_list_1_item[0] = lv_list_add_btn(ui->screen_game_list_1, &game, "2048");
    ui->screen_game_list_1_item[1] = lv_list_add_btn(ui->screen_game_list_1, &game, "Memory Game");
    ui->screen_game_list_1_item[2] = lv_list_add_btn(ui->screen_game_list_1, &game, "Snake");
    ui->screen_game_list_1_item[3] = lv_list_add_btn(ui->screen_game_list_1, &game, "Flappy Bird");

    static lv_style_t style_list_main;
    ui_init_style(&style_list_main);
    lv_style_set_pad_top(&style_list_main, 5);
    lv_style_set_pad_left(&style_list_main, 5);
    lv_style_set_pad_right(&style_list_main, 5);
    lv_style_set_pad_bottom(&style_list_main, 5);
    lv_style_set_bg_opa(&style_list_main, 255);
    lv_style_set_bg_color(&style_list_main, lv_color_hex(0xFFFFFF));
    lv_style_set_border_width(&style_list_main, 2);
    lv_style_set_border_opa(&style_list_main, 255);
    lv_style_set_border_color(&style_list_main, lv_color_hex(0xFAFAFA));
    lv_style_set_border_side(&style_list_main, LV_BORDER_SIDE_FULL);
    lv_style_set_radius(&style_list_main, 3);
    lv_obj_add_style(ui->screen_game_list_1, &style_list_main, LV_PART_MAIN | LV_STATE_DEFAULT);

    static lv_style_t style_list_btn;
    ui_init_style(&style_list_btn);
    lv_style_set_pad_top(&style_list_btn, 6);
    lv_style_set_pad_left(&style_list_btn, 6);
    lv_style_set_pad_right(&style_list_btn, 6);
    lv_style_set_pad_bottom(&style_list_btn, 6);
    lv_style_set_border_width(&style_list_btn, 0);
    lv_style_set_text_color(&style_list_btn, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_list_btn, &songti_font_16);
    lv_style_set_text_opa(&style_list_btn, 255);
    lv_style_set_radius(&style_list_btn, 3);
    lv_style_set_bg_opa(&style_list_btn, 255);
    lv_style_set_bg_color(&style_list_btn, lv_color_hex(0xFFFFFF));
    lv_style_set_bg_grad_dir(&style_list_btn, LV_GRAD_DIR_NONE);

    for (int i = 0; i < _LIST_NUMBER; i++) {
        if (ui->screen_game_list_1_item[i] == NULL) {
            continue;
        }
        lv_obj_add_style(ui->screen_game_list_1_item[i], &style_list_btn, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    lv_obj_update_layout(ui->screen_game);
    events_init_screen_game(ui);
}
