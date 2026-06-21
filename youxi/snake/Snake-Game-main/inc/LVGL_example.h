/*****************************************************************************
* | File        :   LCD_Test.h
* | Author      :   Waveshare team
* | Function    :   test Demo
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2024-07-08
* | Info        :   
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#ifndef _LVGL_EXAMPLE_H_
#define _LVGL_EXAMPLE_H_


#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
//#include "hardware/rtc.h"
#include "pico/util/datetime.h"
#include "DEV_Config.h"
#include "lvgl.h"

#define SNAKE_BODY_ALL_LEN 100

typedef struct snake_body{
		uint16_t iX;
		uint16_t iY;
		uint16_t color;
}snake_body_t;

typedef struct {
		snake_body_t my_snake_body[SNAKE_BODY_ALL_LEN];
		uint16_t snake_body_len;
		uint16_t snake_body_width;
		uint16_t snake_speed;
		
}my_eat_snake;

typedef struct {
		snake_body_t food;		
		uint16_t width;		
}my_snake_food;



/*LVGL objects*/
typedef struct {
    lv_obj_t *scr[6];     // 0,Welcome£»1£¬Setting£»2£¬Game£»3£¬Gameover£»4£¬Clearance
    lv_obj_t *cur;        // Cursor
    lv_obj_t *btn;        // Button
    lv_obj_t *btn2;        // Button
    lv_obj_t *label;      // Label
    lv_obj_t *label2;      // Label
    lv_obj_t *foodleft;      // Label
    lv_obj_t *design;
    lv_obj_t *snake[SNAKE_BODY_ALL_LEN];       // snake body
    lv_obj_t *food[10];       // food
    lv_obj_t *sw_1;       // Switch 2
    uint16_t stage;   // Number of button clicks
    uint16_t speed;   // move speed
    uint16_t KEY_now;     // Current state of the button
    uint16_t KEY_old;     // Button Previous State
    uint16_t dir;         // snack direction
    uint16_t dir_old;         // snack direction
    uint16_t move_cnt;    // control move speed
    uint16_t s_hwCounter;
    uint16_t score;
    uint16_t game_status;
    uint16_t food_num;
    uint16_t del_num;
    uint16_t del_num2;
    uint16_t cnt_screen;
    my_eat_snake My_snake;
    my_snake_food My_food;
} lvgl_data_struct;

typedef void (*LCD_SetWindowsFunc)(uint16_t, uint16_t, uint16_t, uint16_t);

void LVGL_Init(void);
void handle_key_press(lvgl_data_struct *dat);
void switch_to_next_screen(lv_obj_t *scr[]);
void snake_init(lvgl_data_struct *dat);
void draw_snake(lvgl_data_struct *dat);
void snake_move(lvgl_data_struct *dat);
void game_over(lvgl_data_struct *dat);
extern void snake_run(lvgl_data_struct *dat);
extern void snake_swich(lvgl_data_struct *dat);

#endif
