#include "lv_100ask_snake.h"

#if LV_USE_100ASK_SNAKE

#include <stdlib.h>

#define SNAKE_GRID_W 14
#define SNAKE_GRID_H 14
#define SNAKE_MAX_LEN (SNAKE_GRID_W * SNAKE_GRID_H)
#define SNAKE_MOVE_MS 240

typedef struct {
    int16_t x;
    int16_t y;
} snake_pt_t;

typedef struct {
    lv_obj_t *root;
    lv_obj_t *board;
    lv_obj_t *food_obj;
    lv_obj_t *body_obj[SNAKE_MAX_LEN];
    lv_timer_t *timer;
    snake_pt_t body[SNAKE_MAX_LEN];
    snake_pt_t food;
    lv_dir_t dir;
    uint16_t len;
    uint16_t score;
    uint16_t cell;
    bool game_over;
} snake_game_t;

static snake_game_t *s_game = NULL;
static bool s_rand_seeded = false;

static bool snake_is_opposite_dir(lv_dir_t a, lv_dir_t b)
{
    return (a == LV_DIR_LEFT && b == LV_DIR_RIGHT) ||
           (a == LV_DIR_RIGHT && b == LV_DIR_LEFT) ||
           (a == LV_DIR_TOP && b == LV_DIR_BOTTOM) ||
           (a == LV_DIR_BOTTOM && b == LV_DIR_TOP);
}

static bool snake_on_body(const snake_game_t *g, int16_t x, int16_t y)
{
    uint16_t i;
    for (i = 0; i < g->len; i++) {
        if (g->body[i].x == x && g->body[i].y == y) {
            return true;
        }
    }
    return false;
}

static void snake_place_food(snake_game_t *g)
{
    uint16_t tries;
    for (tries = 0; tries < 200; tries++) {
        int16_t x = (int16_t)(rand() % SNAKE_GRID_W);
        int16_t y = (int16_t)(rand() % SNAKE_GRID_H);
        if (!snake_on_body(g, x, y)) {
            g->food.x = x;
            g->food.y = y;
            return;
        }
    }

    g->food.x = 0;
    g->food.y = 0;
}

static void snake_render(snake_game_t *g)
{
    uint16_t i;
    for (i = 0; i < SNAKE_MAX_LEN; i++) {
        if (i < g->len) {
            lv_obj_set_pos(g->body_obj[i], g->body[i].x * g->cell, g->body[i].y * g->cell);
            lv_obj_clear_flag(g->body_obj[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(
                g->body_obj[i],
                (i == 0) ? lv_color_hex(0x2E7D32) : lv_color_hex(0x43A047),
                LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            lv_obj_add_flag(g->body_obj[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    lv_obj_set_pos(g->food_obj, g->food.x * g->cell, g->food.y * g->cell);
}

static void snake_timer_cb(lv_timer_t *timer)
{
    snake_game_t *g = (snake_game_t *)timer->user_data;
    if (g == NULL || g->game_over || g->len == 0) {
        return;
    }

    snake_pt_t next = g->body[0];
    if (g->dir == LV_DIR_LEFT) next.x--;
    else if (g->dir == LV_DIR_RIGHT) next.x++;
    else if (g->dir == LV_DIR_TOP) next.y--;
    else if (g->dir == LV_DIR_BOTTOM) next.y++;

    if (next.x < 0 || next.x >= SNAKE_GRID_W || next.y < 0 || next.y >= SNAKE_GRID_H) {
        g->game_over = true;
        lv_event_send(g->root, LV_EVENT_VALUE_CHANGED, NULL);
        return;
    }

    {
        uint16_t i;
        for (i = 0; i < g->len; i++) {
            if (g->body[i].x == next.x && g->body[i].y == next.y) {
                g->game_over = true;
                lv_event_send(g->root, LV_EVENT_VALUE_CHANGED, NULL);
                return;
            }
        }
    }

    {
        bool ate = (next.x == g->food.x && next.y == g->food.y);
        uint16_t old_len = g->len;
        uint16_t i;

        if (ate && g->len < SNAKE_MAX_LEN) {
            g->len++;
        }

        for (i = g->len; i > 1; i--) {
            g->body[i - 1] = g->body[i - 2];
        }
        g->body[0] = next;

        if (ate) {
            g->score += 10;
            if (old_len == SNAKE_MAX_LEN) {
                g->game_over = true;
            } else {
                snake_place_food(g);
            }
        }
    }

    snake_render(g);
    lv_event_send(g->root, LV_EVENT_VALUE_CHANGED, NULL);
}

static void snake_delete_cb(lv_event_t *e)
{
    snake_game_t *g = (snake_game_t *)lv_event_get_user_data(e);
    if (g == NULL) {
        return;
    }

    if (g->timer != NULL) {
        lv_timer_del(g->timer);
        g->timer = NULL;
    }

    if (s_game == g) {
        s_game = NULL;
    }

    lv_mem_free(g);
}

lv_obj_t *lv_100ask_snake_create(lv_obj_t *parent)
{
    uint16_t i;
    lv_coord_t w;
    lv_coord_t h;

    if (!s_rand_seeded) {
        srand((unsigned)lv_tick_get());
        s_rand_seeded = true;
    }

    if (s_game != NULL && s_game->root != NULL && lv_obj_is_valid(s_game->root)) {
        lv_obj_del(s_game->root);
    }

    s_game = (snake_game_t *)lv_mem_alloc(sizeof(snake_game_t));
    if (s_game == NULL) {
        return NULL;
    }
    lv_memset_00(s_game, sizeof(snake_game_t));

    s_game->root = lv_obj_create(parent);
    lv_obj_add_flag(s_game->root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_game->root, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(s_game->root, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_clear_flag(s_game->root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(s_game->root, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_game->root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_game->root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_game->board = lv_obj_create(s_game->root);
    lv_obj_add_flag(s_game->board, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_game->board, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(s_game->board, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_clear_flag(s_game->board, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_game->board, lv_color_hex(0xD7CCC8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_game->board, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_game->board, lv_color_hex(0x8D6E63), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_game->board, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_game->board, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    w = lv_obj_get_width(s_game->root);
    h = lv_obj_get_height(s_game->root);
    s_game->cell = (uint16_t)LV_MIN((w > 0 ? w : 224) / SNAKE_GRID_W, (h > 0 ? h : 224) / SNAKE_GRID_H);
    if (s_game->cell < 8) {
        s_game->cell = 8;
    }
    lv_obj_set_size(s_game->board, s_game->cell * SNAKE_GRID_W, s_game->cell * SNAKE_GRID_H);
    lv_obj_center(s_game->board);

    for (i = 0; i < SNAKE_MAX_LEN; i++) {
        s_game->body_obj[i] = lv_obj_create(s_game->board);
        lv_obj_clear_flag(s_game->body_obj[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(s_game->body_obj[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_size(s_game->body_obj[i], s_game->cell - 1, s_game->cell - 1);
        lv_obj_set_style_radius(s_game->body_obj[i], 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(s_game->body_obj[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_flag(s_game->body_obj[i], LV_OBJ_FLAG_HIDDEN);
    }

    s_game->food_obj = lv_obj_create(s_game->board);
    lv_obj_clear_flag(s_game->food_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(s_game->food_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(s_game->food_obj, s_game->cell - 1, s_game->cell - 1);
    lv_obj_set_style_radius(s_game->food_obj, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_game->food_obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_game->food_obj, lv_color_hex(0xE53935), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(s_game->root, snake_delete_cb, LV_EVENT_DELETE, s_game);
    s_game->timer = lv_timer_create(snake_timer_cb, SNAKE_MOVE_MS, s_game);

    lv_100ask_snake_new_game(s_game->root);
    return s_game->root;
}

void lv_100ask_snake_new_game(lv_obj_t *obj)
{
    int16_t cx;
    int16_t cy;

    if (s_game == NULL || obj == NULL || obj != s_game->root) {
        return;
    }

    cx = SNAKE_GRID_W / 2;
    cy = SNAKE_GRID_H / 2;

    s_game->len = 3;
    s_game->dir = LV_DIR_RIGHT;
    s_game->score = 0;
    s_game->game_over = false;

    s_game->body[0].x = cx;
    s_game->body[0].y = cy;
    s_game->body[1].x = cx - 1;
    s_game->body[1].y = cy;
    s_game->body[2].x = cx - 2;
    s_game->body[2].y = cy;

    snake_place_food(s_game);
    snake_render(s_game);
    lv_event_send(s_game->root, LV_EVENT_VALUE_CHANGED, NULL);
}

void lv_100ask_snake_set_dir(lv_obj_t *obj, lv_dir_t dir)
{
    if (s_game == NULL || obj == NULL || obj != s_game->root || s_game->game_over) {
        return;
    }

    if (dir != LV_DIR_LEFT && dir != LV_DIR_RIGHT && dir != LV_DIR_TOP && dir != LV_DIR_BOTTOM) {
        return;
    }

    if (snake_is_opposite_dir(s_game->dir, dir)) {
        return;
    }

    s_game->dir = dir;
}

uint16_t lv_100ask_snake_get_score(lv_obj_t *obj)
{
    if (s_game == NULL || obj == NULL || obj != s_game->root) {
        return 0;
    }
    return s_game->score;
}

bool lv_100ask_snake_is_over(lv_obj_t *obj)
{
    if (s_game == NULL || obj == NULL || obj != s_game->root) {
        return true;
    }
    return s_game->game_over;
}

#endif /* LV_USE_100ASK_SNAKE */
