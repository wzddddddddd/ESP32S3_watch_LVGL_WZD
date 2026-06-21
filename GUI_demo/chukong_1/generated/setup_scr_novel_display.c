/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"



void setup_scr_novel_display(lv_ui *ui)
{
    //Write codes novel_display
    ui->novel_display = lv_obj_create(NULL);
    lv_obj_set_size(ui->novel_display, 240, 284);
    lv_obj_set_scrollbar_mode(ui->novel_display, LV_SCROLLBAR_MODE_OFF);

    //Write style for novel_display, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->novel_display, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->novel_display, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->novel_display, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes novel_display_label_1
    ui->novel_display_label_1 = lv_label_create(ui->novel_display);
    lv_label_set_text(ui->novel_display_label_1, "　这个理由无法让雷蒙德信服：\n　　“可你编故事的时候也经常会用‘一百多年前’‘几百年前’‘很久以前’来让大家无法证实。”\n　　“所以才要找你爸爸确认啊！”卢米安一脸“这下知道我为什么要找你爸爸”的表情。\n　　“也是……”雷蒙德接受了这个解释，可总觉得有哪里不对。\n　　两人离开广场，往村庄深处走去时，雷蒙德终于醒悟过来：\n　　“可你为什么要确认这么一个传说是真的还是假的？”\n　　“巫师啊，那可是巫师啊！我们要是能确认他曾经住在哪栋房屋内，后来被埋葬在了哪里，说不定可以发现他的秘密，让自己也获得超越普通人的神奇力量。”卢米安说着像是谎言的实话。\n　　雷蒙德果然露出了一脸“你不要骗我”的表情：\n　　“那些故事大部分都是编来吓小孩的，怎么可能是真的？\n　　“而且，追寻巫师的力量可是会被投进裁判所的！”\n　　因蒂斯共和国位于这个世界的北大陆，处于正统地位的神灵是“永恒烈阳”和“蒸汽与机械之神”，两者的教会瓜分了几乎所有民众的信仰，并且不允许同处北大陆的鲁恩王国的“黑夜女神”教会、“风暴之主”教会，费内波特王国的“大地母神”教会，伦堡等中南诸国的“知识与智慧之神”教会，弗萨克帝国的“战神”教会进来传教。\n　　而“永恒烈阳”教会的宗教裁判所一向让民众们畏惧，不知多少异端、异教徒被关了进去，遭受残酷的对待。\n　　卢米安哈哈笑了起来：\n　　“你现在担心这个干什么？你自己也说了，那些传说绝大部分都是编的，找到巫师遗留的可能几乎没有。\n　　“再说，就算真找到了巫师的遗留，我们也不是一定要继承那种禁忌的力量，完全可以交给教会，换取他们的奖赏，嗯，作为一个巫师，陪葬品里肯定有不少财宝。”\n　　卢米安口中的教会指的是“永恒烈阳”教会，因为他们所在的科尔杜村没有“蒸汽与机械之神”教会——这往往集中在各个大城市和有工厂的地方。\n　　见雷蒙德听得怦然心动，卢米安暗自“啧”了一声，补了一句：\n　　“难道你真的想去当牧羊人？”\n");
    lv_label_set_long_mode(ui->novel_display_label_1, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->novel_display_label_1, 0, 0);
    lv_obj_set_size(ui->novel_display_label_1, 240, 265);
    lv_obj_add_flag(ui->novel_display_label_1, LV_OBJ_FLAG_SCROLLABLE);

    //Write style for novel_display_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->novel_display_label_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->novel_display_label_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->novel_display_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->novel_display_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->novel_display_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes novel_display_spangroup_1
    ui->novel_display_spangroup_1 = lv_spangroup_create(ui->novel_display);
    lv_spangroup_set_align(ui->novel_display_spangroup_1, LV_TEXT_ALIGN_LEFT);
    lv_spangroup_set_overflow(ui->novel_display_spangroup_1, LV_SPAN_OVERFLOW_CLIP);
    lv_spangroup_set_mode(ui->novel_display_spangroup_1, LV_SPAN_MODE_BREAK);
    //create span
    ui->novel_display_spangroup_1_span = lv_spangroup_new_span(ui->novel_display_spangroup_1);
    lv_span_set_text(ui->novel_display_spangroup_1_span, "19");
    lv_style_set_text_color(&ui->novel_display_spangroup_1_span->style, lv_color_hex(0xffffff));
    lv_style_set_text_decor(&ui->novel_display_spangroup_1_span->style, LV_TEXT_DECOR_NONE);
    lv_style_set_text_font(&ui->novel_display_spangroup_1_span->style, &lv_font_Amiko_Regular_12);
    ui->novel_display_spangroup_1_span = lv_spangroup_new_span(ui->novel_display_spangroup_1);
    lv_span_set_text(ui->novel_display_spangroup_1_span, ":");
    lv_style_set_text_color(&ui->novel_display_spangroup_1_span->style, lv_color_hex(0xfdfdfd));
    lv_style_set_text_decor(&ui->novel_display_spangroup_1_span->style, LV_TEXT_DECOR_NONE);
    lv_style_set_text_font(&ui->novel_display_spangroup_1_span->style, &lv_font_montserratMedium_12);
    ui->novel_display_spangroup_1_span = lv_spangroup_new_span(ui->novel_display_spangroup_1);
    lv_span_set_text(ui->novel_display_spangroup_1_span, "45");
    lv_style_set_text_color(&ui->novel_display_spangroup_1_span->style, lv_color_hex(0xffffff));
    lv_style_set_text_decor(&ui->novel_display_spangroup_1_span->style, LV_TEXT_DECOR_NONE);
    lv_style_set_text_font(&ui->novel_display_spangroup_1_span->style, &lv_font_Amiko_Regular_12);
    lv_obj_set_pos(ui->novel_display_spangroup_1, 198, 269);
    lv_obj_set_size(ui->novel_display_spangroup_1, 39, 8);

    //Write style state: LV_STATE_DEFAULT for &style_novel_display_spangroup_1_main_main_default
    static lv_style_t style_novel_display_spangroup_1_main_main_default;
    ui_init_style(&style_novel_display_spangroup_1_main_main_default);

    lv_style_set_border_width(&style_novel_display_spangroup_1_main_main_default, 0);
    lv_style_set_radius(&style_novel_display_spangroup_1_main_main_default, 0);
    lv_style_set_bg_opa(&style_novel_display_spangroup_1_main_main_default, 0);
    lv_style_set_pad_top(&style_novel_display_spangroup_1_main_main_default, 0);
    lv_style_set_pad_right(&style_novel_display_spangroup_1_main_main_default, 0);
    lv_style_set_pad_bottom(&style_novel_display_spangroup_1_main_main_default, 0);
    lv_style_set_pad_left(&style_novel_display_spangroup_1_main_main_default, 0);
    lv_style_set_shadow_width(&style_novel_display_spangroup_1_main_main_default, 0);
    lv_obj_add_style(ui->novel_display_spangroup_1, &style_novel_display_spangroup_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_spangroup_refr_mode(ui->novel_display_spangroup_1);

    //The custom code of novel_display.


    //Update current screen layout.
    lv_obj_update_layout(ui->novel_display);

    //Init events for screen.
    events_init_novel_display(ui);
}
