/*
 * setup_scr_screen_weather.c
 * Weather detail screen (generated-style screen kept in generated/ per project convention).
 */

#include "lvgl.h"
#include "gui_guider.h"
#include "events_init.h"

void setup_scr_screen_weather(lv_ui *ui)
{
    ui->screen_weather = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_weather, 240, 284);
    lv_obj_set_scrollbar_mode(ui->screen_weather, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(ui->screen_weather, lv_color_hex(0x0B1220), 0);
    lv_obj_set_style_bg_opa(ui->screen_weather, 255, 0);

    lv_obj_t *title = lv_label_create(ui->screen_weather);
    lv_label_set_text(title, "Weather");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &songti_font_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    ui->screen_weather_label_city = lv_label_create(ui->screen_weather);
    lv_label_set_text(ui->screen_weather_label_city, "City: --");
    lv_obj_set_style_text_color(ui->screen_weather_label_city, lv_color_hex(0xC8CDD6), 0);
    lv_obj_set_style_text_font(ui->screen_weather_label_city, &songti_font_16, 0);
    lv_obj_align(ui->screen_weather_label_city, LV_ALIGN_TOP_LEFT, 14, 48);

    ui->screen_weather_label_condition = lv_label_create(ui->screen_weather);
    lv_label_set_text(ui->screen_weather_label_condition, "Weather: --");
    lv_obj_set_style_text_color(ui->screen_weather_label_condition, lv_color_hex(0xC8CDD6), 0);
    lv_obj_set_style_text_font(ui->screen_weather_label_condition, &songti_font_16, 0);
    lv_obj_align(ui->screen_weather_label_condition, LV_ALIGN_TOP_LEFT, 14, 78);

    ui->screen_weather_label_temp = lv_label_create(ui->screen_weather);
    lv_label_set_text(ui->screen_weather_label_temp, "Temp: --");
    lv_obj_set_style_text_color(ui->screen_weather_label_temp, lv_color_hex(0xC8CDD6), 0);
    lv_obj_set_style_text_font(ui->screen_weather_label_temp, &songti_font_16, 0);
    lv_obj_align(ui->screen_weather_label_temp, LV_ALIGN_TOP_LEFT, 14, 108);

    ui->screen_weather_label_humidity = lv_label_create(ui->screen_weather);
    lv_label_set_text(ui->screen_weather_label_humidity, "Humidity: --");
    lv_obj_set_style_text_color(ui->screen_weather_label_humidity, lv_color_hex(0xC8CDD6), 0);
    lv_obj_set_style_text_font(ui->screen_weather_label_humidity, &songti_font_16, 0);
    lv_obj_align(ui->screen_weather_label_humidity, LV_ALIGN_TOP_LEFT, 14, 138);

    ui->screen_weather_label_update = lv_label_create(ui->screen_weather);
    lv_label_set_text(ui->screen_weather_label_update, "Updated: --");
    lv_obj_set_style_text_color(ui->screen_weather_label_update, lv_color_hex(0xC8CDD6), 0);
    lv_obj_set_style_text_font(ui->screen_weather_label_update, &songti_font_16, 0);
    lv_obj_align(ui->screen_weather_label_update, LV_ALIGN_TOP_LEFT, 14, 168);

    ui->screen_weather_label_status = lv_label_create(ui->screen_weather);
    lv_label_set_text(ui->screen_weather_label_status, "");
    lv_obj_set_style_text_color(ui->screen_weather_label_status, lv_color_hex(0xC8CDD6), 0);
    lv_obj_set_style_text_font(ui->screen_weather_label_status, &songti_font_16, 0);
    lv_obj_set_width(ui->screen_weather_label_status, 240);
    lv_obj_set_style_text_align(ui->screen_weather_label_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ui->screen_weather_label_status, LV_ALIGN_BOTTOM_MID, 0, -18);

    lv_obj_update_layout(ui->screen_weather);
    events_init_screen_weather(ui);
    ui->screen_weather_del = false;
}

