/*****************************************************************************
* | File      	:   LCD_1in14_V2_LVGL_test.c
* | Author      :   Waveshare team
* | Function    :   1.14inch LCD LVGL demo
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
#
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
#include "LCD_Test.h"
#include "LCD_1in14_V2.h"

static void Widgets_Init(lvgl_data_struct *dat);
static void LCD_1IN14_V2_KEY_Init(void);
static uint16_t LCD_1IN14_V2_Read_KEY(void);
extern int fade;
extern int sound_state[2];
/********************************************************************************
function:   Main function
parameter:
********************************************************************************/
int LCD_1in14_V2_test(void)
{
    if(DEV_Module_Init()!=0){
        return -1;
    } 

    /*KEY Init*/
    LCD_1IN14_V2_KEY_Init();

    /*LCD Init*/
    LCD_1IN14_V2_Init(HORIZONTAL);
    LCD_1IN14_V2_Clear(WHITE);

    /*Config parameters*/
    LCD_SetWindows = LCD_1IN14_V2_SetWindows;
    DISP_HOR_RES = LCD_1IN14_V2_HEIGHT;
    DISP_VER_RES = LCD_1IN14_V2_WIDTH;

    /*Init LVGL data structure*/    
    lvgl_data_struct *dat = (lvgl_data_struct *)malloc(sizeof(lvgl_data_struct));
    memset(dat->scr, 0, sizeof(dat->scr));
    dat->stage = 0;
    
    /*Init LVGL*/
    LVGL_Init();
    Widgets_Init(dat);

    while(1)
    {
        lv_task_handler();
        DEV_Delay_ms(5); 
 
        /*Read the key value and save it to the current key value*/
        dat->KEY_now = LCD_1IN14_V2_Read_KEY();

        /*Handling key press events*/
        handle_key_press(dat);

        /*Save the current key value as the previous key value*/
        dat->KEY_old = dat->KEY_now;
    }
    
    DEV_Module_Exit();
    return 0;
}

/********************************************************************************
function:   Initialize Widgets
parameter:
********************************************************************************/
static void Widgets_Init(lvgl_data_struct *dat)
{
    static lv_style_t style_label;
    lv_style_init(&style_label);
    lv_style_set_text_font(&style_label, &lv_font_montserrat_16);
    
    static lv_style_t style_label2;
    lv_style_init(&style_label2);
    lv_style_set_text_font(&style_label2, &lv_font_montserrat_20);    
    
    static lv_style_t food_left;
    lv_style_init(&food_left);
    lv_style_set_text_font(&food_left, &lv_font_montserrat_8);     

    /*Screen1: Just a picture*/
    dat->scr[0] = lv_obj_create(NULL);
    
    /*Declare and load the image resource*/
    LV_IMG_DECLARE(LCD_1inch14);
    lv_obj_t *img1 = lv_img_create(dat->scr[0]);
    lv_img_set_src(img1, &LCD_1inch14);

    /*Align the image to the center of the screen*/
    lv_obj_align(img1, LV_ALIGN_CENTER, 0, 0);

    /*Load the first screen as the current active screen*/
    lv_scr_load(dat->scr[0]);

    /*Screen2: User Interface*/
    dat->scr[1] = lv_obj_create(NULL);

    /*Create a 110x35 pixel button and add it to the center of the second screen*/
    dat->btn = lv_btn_create(dat->scr[1]);     
    lv_obj_set_size(dat->btn, 110, 35);                
    lv_obj_align(dat->btn, LV_ALIGN_CENTER, 0, 0);

    /*Create a label on the button and set its initial text to "Click:0"*/
    dat->label = lv_label_create(dat->btn);          
    lv_label_set_text(dat->label, "Stage:1");               
    lv_obj_center(dat->label);

    /*Add style to the label and set the font size to 16*/
    lv_obj_add_style(dat->label,&style_label,0);
    
    dat->design = lv_label_create(dat->scr[1]);          
    lv_label_set_text(dat->design, "Designed by Minkai-Shi");               
 //   lv_obj_center(dat->design);
    lv_obj_add_style(dat->design,&style_label,0);
    lv_obj_align(dat->design,LV_ALIGN_BOTTOM_RIGHT,0,0);
    

    /*Create an icon and add it to the second screen and set the image source of the icon to the GPS symbol*/
    dat->cur = lv_img_create(dat->scr[1]);
    lv_img_set_src(dat->cur, LV_SYMBOL_GPS);
    
    /*Screen3: Game*/
    dat->scr[2] = lv_obj_create(NULL);
        /*Declare and load the image resource*/
    LV_IMG_DECLARE(LCD_1inch14_b_g);
    lv_obj_t *img2 = lv_img_create(dat->scr[2]);
    lv_img_set_src(img2, &LCD_1inch14_b_g);

    /*Align the image to the center of the screen*/
    lv_obj_align(img2, LV_ALIGN_CENTER, 0, 0);
    dat->foodleft = lv_label_create(dat->scr[2]);          
    lv_label_set_text(dat->foodleft, "0");               
    lv_obj_align(dat->foodleft,LV_ALIGN_OUT_TOP_RIGHT,0,0); 

		dat->score = 0;
    snake_init(dat);

    /*Create an icon and add it to the second screen and set the image source of the icon to the GPS symbol*/
    dat->snake[0] = lv_img_create(dat->scr[2]);
    lv_img_set_src(dat->snake[0], LV_SYMBOL_EYE_OPEN);

    dat->snake[1] = lv_img_create(dat->scr[2]);
    lv_img_set_src(dat->snake[1], LV_SYMBOL_EYE_OPEN);

    dat->snake[2] = lv_img_create(dat->scr[2]);
    lv_img_set_src(dat->snake[2], LV_SYMBOL_EYE_OPEN);   

    dat->food[0] = lv_img_create(dat->scr[2]);
    lv_img_set_src(dat->food[0], LV_SYMBOL_DRIVE);
    
    dat->scr[3] = lv_obj_create(NULL);
    
 		/*Create a label */
    dat->label2 = lv_label_create(dat->scr[3]);          
//    lv_label_set_text(dat->label2, "Stage:1");               
    lv_obj_center(dat->label2);

    /*Add style to the label and set the font size to 32*/
    lv_obj_add_style(dat->label2,&style_label2,0);
}

/********************************************************************************
function:   Initialize all keys
parameter:
********************************************************************************/
static void LCD_1IN14_V2_KEY_Init(void)
{
    DEV_KEY_Config(LCD_KEY_A);
    DEV_KEY_Config(LCD_KEY_B);
    DEV_KEY_Config(LCD_KEY_UP);
    DEV_KEY_Config(LCD_KEY_DOWN);
    DEV_KEY_Config(LCD_KEY_LEFT);
    DEV_KEY_Config(LCD_KEY_RIGHT);
    DEV_KEY_Config(LCD_KEY_CTRL);
}

/********************************************************************************
function:   Read the status of all keys. If a key is pressed, the data
            position corresponding to the key is 1, otherwise it is 0
parameter:
********************************************************************************/
static uint16_t LCD_1IN14_V2_Read_KEY(void) 
{
    uint16_t KEY_Value = 0;

    if (DEV_Digital_Read(LCD_KEY_A)  == 0)          KEY_Value |= KEY_A;
    if (DEV_Digital_Read(LCD_KEY_B)  == 0)          KEY_Value |= KEY_B;

    if (DEV_Digital_Read(LCD_KEY_UP) == 0)          KEY_Value |= KEY_UP;
    else if (DEV_Digital_Read(LCD_KEY_DOWN)  == 0)  KEY_Value |= KEY_DOWN;
    else if (DEV_Digital_Read(LCD_KEY_LEFT)  == 0)  KEY_Value |= KEY_LEFT;
    else if (DEV_Digital_Read(LCD_KEY_RIGHT) == 0)  KEY_Value |= KEY_RIGHT;
    else if (DEV_Digital_Read(LCD_KEY_CTRL)  == 0)  KEY_Value |= KEY_CTRL;

    return KEY_Value;
}

void snake_init(lvgl_data_struct *dat){
		uint16_t i ;
		for(i = 0; i < SNAKE_BODY_ALL_LEN; i++){
				dat->My_snake.my_snake_body[i].color = BLUE;
		}
		
		if((dat->My_snake.snake_body_len>3)&&(dat->My_snake.snake_body_len<100))
		{
			for(i=3;i<dat->My_snake.snake_body_len;i++)	
			{
				lv_obj_del(dat->snake[i]);
			}
		}		
		
		dat->My_snake.snake_body_len = 3;
		dat->My_snake.snake_body_width = 10;
	
		dat->My_snake.my_snake_body[0].iX = 70;
		dat->My_snake.my_snake_body[0].iY = 40;
	
		dat->My_snake.my_snake_body[1].iX = 60;
		dat->My_snake.my_snake_body[1].iY = 40;
	
		dat->My_snake.my_snake_body[2].iX = 50;
		dat->My_snake.my_snake_body[2].iY = 40;
		
		dat->My_food.food.iX  = 50;
		dat->My_food.food.iY  = 50;
		dat->My_food.food.color = RED;
		dat->My_food.width = 10;
		dat->dir = 4;
		switch(dat->stage)
		{
			case 1:
				dat->speed = 45;
				dat->food_num = 8;
			break;
			case 2:
				dat->speed = 35;
				dat->food_num = 10;
			break;
			case 3:
				dat->speed = 30;
				dat->food_num = 12;
			break;
			case 4:
				dat->speed = 25;
				dat->food_num = 12;
			break;
			case 5:
				dat->speed = 22;
				dat->food_num = 16;
			break;
			case 6:
				dat->speed = 20;
				dat->food_num = 16;
			break;
			case 7:
				dat->speed = 16;
				dat->food_num = 16;
			break;
			default:
				dat->speed = 60;
				dat->food_num = 2;				 
		}

}
void draw_snake_food(lvgl_data_struct *dat){
		lv_obj_set_pos(dat->food[0], dat->My_food.food.iX, dat->My_food.food.iY);
}
void draw_snake(lvgl_data_struct *dat){
		uint16_t i;

		for(i = 0; i < dat->My_snake.snake_body_len; i++){
				lv_obj_set_pos(dat->snake[i], dat->My_snake.my_snake_body[i].iX, dat->My_snake.my_snake_body[i].iY);
		}

}

void snake_move(lvgl_data_struct *dat){
		uint16_t i;
		//eat food
		if((dat->My_snake.my_snake_body[0].iX == dat->My_food.food.iX)&&
			(dat->My_snake.my_snake_body[0].iY == dat->My_food.food.iY)&&
			dat->My_snake.snake_body_len < 100)
			{				
				dat->snake[dat->My_snake.snake_body_len] = lv_img_create(dat->scr[2]);
    		lv_img_set_src(dat->snake[dat->My_snake.snake_body_len], LV_SYMBOL_EYE_OPEN);   
				dat->My_snake.snake_body_len++;
				dat->score += 10;							
 			   // random food
 			  if(dat->food_num>1)
 			  {
 			  	dat->My_food.food.iX = (rand() % 24) *10;
    			dat->My_food.food.iY = (rand() % 13) *10;	
    			dat->food_num--;	
    			sound_state[1]=1;	  	
 			  }
 			  else
 			  {
 			  	DEV_Delay_ms(500); 			  	
 			  	dat->game_status = 1;
 			  	dat->stage++;
 			  	dat->stage &= 7; 			  	
 			  	lv_scr_load(dat->scr[3]);
 			  	dat->cnt_screen = 0;
 			  	sound_state[1]=2;
 			  }				
			}
		//body move	
		for( i = dat->My_snake.snake_body_len - 1; i > 0; i--){
				dat->My_snake.my_snake_body[i].iX = dat->My_snake.my_snake_body[i-1].iX;
				dat->My_snake.my_snake_body[i].iY = dat->My_snake.my_snake_body[i-1].iY;				
		}		
    //head move
		dat->dir_old = dat->dir;
		switch(dat->dir) 
		{
			case 1:// Joystick up
				if(dat->My_snake.my_snake_body[0].iY < dat->My_snake.snake_body_width)
					if(dat->stage > 0)
						game_over(dat);
					else
						dat->My_snake.my_snake_body[0].iY = DISP_VER_RES-15;
				else						
					dat->My_snake.my_snake_body[0].iY -= dat->My_snake.snake_body_width;	
				break;	
			case 2:// Joystick down
				if(dat->My_snake.my_snake_body[0].iY >= DISP_VER_RES-20)
					if(dat->stage > 0)
						game_over(dat);
					else					
						dat->My_snake.my_snake_body[0].iY = dat->My_snake.snake_body_width;
				else						
					dat->My_snake.my_snake_body[0].iY += dat->My_snake.snake_body_width;	
				break;
	    		
			case 3:// Joystick left
				if(dat->My_snake.my_snake_body[0].iX <= dat->My_snake.snake_body_width)
					if(dat->stage > 0)
						game_over(dat);
					else
						dat->My_snake.my_snake_body[0].iX = DISP_HOR_RES-20;
				else						
					dat->My_snake.my_snake_body[0].iX -= dat->My_snake.snake_body_width;		   
				break; 		
			default:// Joystick right
				if(dat->My_snake.my_snake_body[0].iX >= DISP_HOR_RES-20)
					if(dat->stage > 0)
						game_over(dat);
					else
						dat->My_snake.my_snake_body[0].iX = dat->My_snake.snake_body_width;
				else						
					dat->My_snake.my_snake_body[0].iX += dat->My_snake.snake_body_width;		   
		}	
		//Check if the head touches the body
		for( i = dat->My_snake.snake_body_len - 1; i > 0; i--){
				if((dat->My_snake.my_snake_body[i].iX == dat->My_snake.my_snake_body[0].iX)&&
				(dat->My_snake.my_snake_body[i].iY == dat->My_snake.my_snake_body[0].iY)&&
				(dat->stage > 1))		
					game_over(dat);	
		}			
}

void snake_run(lvgl_data_struct *dat){
		char label_text[128];
		if((dat->KEY_now & KEY_UP) && (dat->dir_old > 2)) // Joystick up
    {
        dat->dir = 1;
    }
    else if((dat->KEY_now & KEY_DOWN) && (dat->dir_old > 2)) // Joystick down
    {
        dat->dir = 2;
    }
    else if((dat->KEY_now & KEY_LEFT) && (dat->dir_old < 3)) // Joystick left
    {
        dat->dir = 3;
    }
    else if((dat->KEY_now & KEY_RIGHT) && (dat->dir_old < 3)) // Joystick right
    {
        dat->dir = 4;
    }  	
		//food
		sprintf(label_text, "%d ",dat->score);    
   	lv_label_set_text(dat->foodleft, label_text);  // Update label	 
		
		dat->move_cnt++;
		if(dat->move_cnt > dat->speed)
		{
			snake_move(dat);
			dat->move_cnt = 0;
		}	
	
		draw_snake(dat);
			
		draw_snake_food(dat);						
}

void snake_swich(lvgl_data_struct *dat){
		char label_text[128];
		uint16_t i;
		if(dat->game_status == 1)
		{
    	sprintf(label_text, "Stage %d cleared!\n  Score:%d",dat->stage,dat->score);    
    	lv_label_set_text(dat->label2, label_text);  // Update label	    	
    	snake_init(dat);
    	if(dat->cnt_screen>200)
    	{
    		dat->game_status = 0;
    		dat->cnt_screen = 0;
    		lv_scr_load(dat->scr[2]);    		
    	}
    	dat->cnt_screen++;  	
  	}
  	else	if(dat->game_status == 2)
		{
    	sprintf(label_text, "GAME OVER!\nScore:%d",dat->score);
    	lv_label_set_text(dat->label2, label_text);  // Update label
    	if(dat->cnt_screen>200)
    	{
    		dat->stage = 0;
    		dat->score = 0;    	
    		snake_init(dat);
    		dat->game_status = 0;
    		dat->cnt_screen = 0;
    	 	lv_scr_load(dat->scr[1]);
    	}
    	dat->cnt_screen++;  	
  	}
  	else
  	{
  		sprintf(label_text, "stage%d\ndel_num:%d\ndel_num2:%d\nfade:%d",
  		dat->stage,
  		dat->del_num,
  		fade,
  		dat->My_snake.snake_body_len);
  		lv_label_set_text(dat->label2, label_text);  // Update label
  	} 
  				
}

void game_over(lvgl_data_struct *dat){
		sound_state[1]=3;
		DEV_Delay_ms(500);		
		dat->game_status = 2;		
		dat->cnt_screen = 0;
		lv_scr_load(dat->scr[3]);	
}

