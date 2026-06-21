#ifndef LV_LIB_100ASK_H
#define LV_LIB_100ASK_H

#include "lvgl.h"

/*
 * Local compatibility config for the imported 100ask 2048 widget.
 * Keep defaults overridable from compiler flags or other config headers.
 */
#ifndef LV_USE_100ASK_2048
#define LV_USE_100ASK_2048 1
#endif

#ifndef LV_100ASK_2048_SIMPLE_TEST
#define LV_100ASK_2048_SIMPLE_TEST 0
#endif

#ifndef LV_USE_100ASK_MEMORY_GAME
#define LV_USE_100ASK_MEMORY_GAME 1
#endif

#ifndef LV_100ASK_MEMORY_GAME_SIMPLE_TEST
#define LV_100ASK_MEMORY_GAME_SIMPLE_TEST 0
#endif

#ifndef LV_100ASK_MEMORY_GAME_DEFAULT_ROW
#define LV_100ASK_MEMORY_GAME_DEFAULT_ROW 4
#endif

#ifndef LV_100ASK_MEMORY_GAME_DEFAULT_COL
#define LV_100ASK_MEMORY_GAME_DEFAULT_COL 4
#endif

#ifndef LV_USE_100ASK_SNAKE
#define LV_USE_100ASK_SNAKE 1
#endif

#endif /* LV_LIB_100ASK_H */
