#ifndef ARC_MENU_H
#define ARC_MENU_H

#include "lvgl.h"

#define ARC_CONT_WIDTH  100
#define ARC_CONT_HEIGHT 284

#define ARC_CENTER_X    (-120)
#define ARC_CENTER_Y    142
#define ARC_RADIUS      180

#define PI              3.14159265f
#define DEG2RAD(a)      ((a) * PI / 180.0f)
#define MAX_WALLPAPERS  20

void arc_menu_init(lv_obj_t *parent);
void arc_menu_open(void);
void arc_menu_close(void);
bool arc_menu_is_open(void);

#endif
