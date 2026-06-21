#include <stddef.h>
#include <stdio.h>

#include "custom.h"
#include "lvgl.h"

extern lv_ui guider_ui;

static void custom_btn_novel_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    ui_load_scr_animation(&guider_ui, &guider_ui.novel_list, guider_ui.novel_list_del,
                          &guider_ui.menu_screen_del, setup_scr_novel_list,
                          LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
}

static void custom_btn_img_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    ui_load_scr_animation(&guider_ui, &guider_ui.screen_img_list, guider_ui.screen_img_list_del,
                          &guider_ui.menu_screen_del, setup_scr_screen_img_list,
                          LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
}

static void custom_btn_video_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    ui_load_scr_animation(&guider_ui, &guider_ui.video_list, guider_ui.video_list_del,
                          &guider_ui.menu_screen_del, setup_scr_video_list,
                          LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
}

static void custom_btn_setting_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if (lv_scr_act() != guider_ui.menu_screen) {
        return;
    }
    ui_load_scr_animation(&guider_ui, &guider_ui.setting_screen, guider_ui.setting_screen_del,
                          &guider_ui.menu_screen_del, setup_scr_setting_screen,
                          LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
}

static void custom_btn_game_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    ui_load_scr_animation(&guider_ui, &guider_ui.screen_game, guider_ui.screen_game_del,
                          &guider_ui.menu_screen_del, setup_scr_screen_game,
                          LV_SCR_LOAD_ANIM_NONE, 0, 0, false, true);
}

extern const lv_img_dsc_t novel;
extern const lv_img_dsc_t image;
extern const lv_img_dsc_t OTA;
extern const lv_img_dsc_t she_zhi;
extern const lv_img_dsc_t game;

#define MENU_ITEM_COUNT 5
static lv_obj_t *s_menu_imgs[MENU_ITEM_COUNT];

static void menu_scroll_event_cb(lv_event_t *e)
{
    lv_obj_t *cont = lv_event_get_target(e);
    lv_area_t cont_a;
    lv_obj_get_coords(cont, &cont_a);

    if (lv_area_get_width(&cont_a) == 0) {
        return;
    }

    lv_coord_t cont_x_center = cont_a.x1 + lv_area_get_width(&cont_a) / 2;

    uint32_t child_cnt = lv_obj_get_child_cnt(cont);
    for (uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t *child = lv_obj_get_child(cont, i);
        lv_area_t child_a;
        lv_obj_get_coords(child, &child_a);
        lv_coord_t child_x_center = child_a.x1 + lv_area_get_width(&child_a) / 2;
        lv_coord_t diff_x = LV_ABS(child_x_center - cont_x_center);

        uint16_t zoom = 300 - (diff_x * 100) / (lv_area_get_width(&cont_a) / 2);
        if (zoom < 200) zoom = 200;
        if (zoom > 300) zoom = 300;
        if (i < MENU_ITEM_COUNT && s_menu_imgs[i] != NULL) {
            lv_img_set_zoom(s_menu_imgs[i], zoom);
        }

        lv_opa_t opa = 255 - (diff_x * 135) / (lv_area_get_width(&cont_a) / 2);
        if (opa < 100) opa = 100;
        lv_obj_set_style_opa(child, opa, 0);
    }
}

static lv_obj_t *create_menu_item(lv_obj_t *parent, const void *img_src, const char *text, int idx)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 100, 120);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(cont, 0, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(cont, LV_DIR_NONE);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *img = lv_img_create(cont);
    lv_img_set_src(img, img_src);
    if (idx < MENU_ITEM_COUNT) {
        s_menu_imgs[idx] = img;
    }

    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &songti_font_16, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x0D3055), 0);

    return cont;
}

void create_swipeable_menu(lv_ui *ui)
{
    if (ui->menu_screen_list_1 != NULL) {
        lv_obj_del(ui->menu_screen_list_1);
        ui->menu_screen_list_1 = NULL;
    }

    lv_obj_t *cont = lv_obj_create(ui->menu_screen);
    lv_obj_set_size(cont, 240, 170);
    lv_obj_center(cont);

    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(cont, LV_DIR_HOR);
    lv_obj_set_scroll_snap_x(cont, LV_SCROLL_SNAP_CENTER);

    lv_obj_set_style_bg_opa(cont, 0, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_left(cont, 80, 0);
    lv_obj_set_style_pad_right(cont, 80, 0);
    lv_obj_set_style_pad_column(cont, 40, 0);

    struct menu_item_cfg {
        const void *img_src;
        const char *text;
        void (*cb)(lv_event_t *);
    };

    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        s_menu_imgs[i] = NULL;
    }

    static struct menu_item_cfg items[] = {
        {&novel, "小说", custom_btn_novel_cb},
        {&image, "图片", custom_btn_img_cb},
        {&OTA, "视频", custom_btn_video_cb},
        {&she_zhi, "设置", custom_btn_setting_cb},
        {&game, "游戏", custom_btn_game_cb},
    };

    for (size_t i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
        lv_obj_t *item = create_menu_item(cont, items[i].img_src, items[i].text, (int)i);
        lv_obj_add_event_cb(item, items[i].cb, LV_EVENT_CLICKED, ui);
    }

    ui->menu_screen_list_1 = cont;

    lv_obj_add_event_cb(cont, menu_scroll_event_cb, LV_EVENT_SCROLL, NULL);
    lv_obj_update_layout(cont);
    lv_event_send(cont, LV_EVENT_SCROLL, NULL);
    lv_obj_scroll_to_view(lv_obj_get_child(cont, 0), LV_ANIM_OFF);
}
