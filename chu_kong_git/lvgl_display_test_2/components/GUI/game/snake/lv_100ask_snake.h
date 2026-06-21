#ifndef LV_100ASK_SNAKE_H
#define LV_100ASK_SNAKE_H

#include "../../lv_lib_100ask.h"

#if LV_USE_100ASK_SNAKE

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *lv_100ask_snake_create(lv_obj_t *parent);
void lv_100ask_snake_new_game(lv_obj_t *obj);
void lv_100ask_snake_set_dir(lv_obj_t *obj, lv_dir_t dir);
uint16_t lv_100ask_snake_get_score(lv_obj_t *obj);
bool lv_100ask_snake_is_over(lv_obj_t *obj);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_USE_100ASK_SNAKE */

#endif /* LV_100ASK_SNAKE_H */
