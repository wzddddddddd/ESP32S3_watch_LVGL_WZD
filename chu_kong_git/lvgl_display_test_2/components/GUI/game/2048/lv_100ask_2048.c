/**
 * @file lv_100ask_2048.c
 *
 * LVGL8-compatible 2048 widget shim.
 */

#include "lv_100ask_2048.h"

#if LV_USE_100ASK_2048 != 0

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gui_guider.h>

#define CELL_COUNT MATRIX_SIZE
#define MAX_GAMES 4

typedef struct {
    bool used;
    lv_obj_t *root;
    lv_obj_t *cell_obj[CELL_COUNT][CELL_COUNT];
    lv_obj_t *cell_label[CELL_COUNT][CELL_COUNT];
    uint16_t board[CELL_COUNT][CELL_COUNT];
    uint32_t score;
    bool over;
    lv_point_t press_point;
    bool pressed;
    bool moved_in_press;
} game_ctx_t;

static game_ctx_t s_games[MAX_GAMES];
static bool s_rand_seeded = false;

static lv_color_t color_for_value(uint16_t v)
{
    switch (v) {
        case 0:    return lv_color_hex(0xc7b9ac);
        case 2:    return lv_color_hex(0xeee4da);
        case 4:    return lv_color_hex(0xede0c8);
        case 8:    return lv_color_hex(0xf2b179);
        case 16:   return lv_color_hex(0xf59563);
        case 32:   return lv_color_hex(0xf67c5f);
        case 64:   return lv_color_hex(0xf75f3b);
        case 128:  return lv_color_hex(0xedcf72);
        case 256:  return lv_color_hex(0xedcc61);
        case 512:  return lv_color_hex(0xedc850);
        case 1024: return lv_color_hex(0xedc53f);
        default:   return lv_color_hex(0xedc22e);
    }
}

static lv_color_t text_color_for_value(uint16_t v)
{
    return (v <= 4) ? lv_color_hex(0x6c635b) : lv_color_hex(0xf8f5f0);
}

static game_ctx_t *ctx_from_obj(lv_obj_t *obj)
{
    for (int i = 0; i < MAX_GAMES; i++) {
        if (s_games[i].used && s_games[i].root == obj) {
            return &s_games[i];
        }
    }
    return NULL;
}

static game_ctx_t *alloc_ctx(lv_obj_t *obj)
{
    for (int i = 0; i < MAX_GAMES; i++) {
        if (!s_games[i].used) {
            memset(&s_games[i], 0, sizeof(s_games[i]));
            s_games[i].used = true;
            s_games[i].root = obj;
            return &s_games[i];
        }
    }
    return NULL;
}

static void free_ctx(lv_obj_t *obj)
{
    for (int i = 0; i < MAX_GAMES; i++) {
        if (s_games[i].used && s_games[i].root == obj) {
            memset(&s_games[i], 0, sizeof(s_games[i]));
            return;
        }
    }
}

static void ensure_seeded(void)
{
    if (s_rand_seeded) {
        return;
    }
    s_rand_seeded = true;
    srand((unsigned)(time(NULL) ^ lv_tick_get()));
}

static void update_view(game_ctx_t *ctx)
{
    for (int r = 0; r < CELL_COUNT; r++) {
        for (int c = 0; c < CELL_COUNT; c++) {
            uint16_t v = ctx->board[r][c];
            lv_obj_set_style_bg_color(ctx->cell_obj[r][c], color_for_value(v), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ctx->cell_label[r][c], text_color_for_value(v), LV_PART_MAIN | LV_STATE_DEFAULT);
            if (v == 0) {
                lv_label_set_text(ctx->cell_label[r][c], "");
            } else {
                lv_label_set_text_fmt(ctx->cell_label[r][c], "%u", v);
            }
        }
    }
}

static void clear_board(game_ctx_t *ctx)
{
    memset(ctx->board, 0, sizeof(ctx->board));
    ctx->score = 0;
    ctx->over = false;
}

static void add_random_tile(game_ctx_t *ctx)
{
    int empty[16][2];
    int n = 0;

    for (int r = 0; r < CELL_COUNT; r++) {
        for (int c = 0; c < CELL_COUNT; c++) {
            if (ctx->board[r][c] == 0) {
                empty[n][0] = r;
                empty[n][1] = c;
                n++;
            }
        }
    }

    if (n == 0) {
        return;
    }

    int pick = rand() % n;
    int rr = empty[pick][0];
    int cc = empty[pick][1];
    ctx->board[rr][cc] = ((rand() % 10) == 0) ? 4 : 2;
}

static bool slide_line_left(uint16_t line[CELL_COUNT], uint32_t *score)
{
    uint16_t old[CELL_COUNT];
    uint16_t packed[CELL_COUNT] = {0};
    int idx = 0;
    bool changed = false;

    memcpy(old, line, sizeof(old));

    for (int i = 0; i < CELL_COUNT; i++) {
        if (line[i] != 0) {
            packed[idx++] = line[i];
        }
    }

    for (int i = 0; i < CELL_COUNT - 1; i++) {
        if (packed[i] != 0 && packed[i] == packed[i + 1]) {
            packed[i] = (uint16_t)(packed[i] * 2U);
            *score += packed[i];
            for (int j = i + 1; j < CELL_COUNT - 1; j++) {
                packed[j] = packed[j + 1];
            }
            packed[CELL_COUNT - 1] = 0;
        }
    }

    memcpy(line, packed, sizeof(packed));
    if (memcmp(old, line, sizeof(old)) != 0) {
        changed = true;
    }
    return changed;
}

static bool move_left(game_ctx_t *ctx)
{
    bool changed = false;
    for (int r = 0; r < CELL_COUNT; r++) {
        uint16_t line[CELL_COUNT];
        for (int c = 0; c < CELL_COUNT; c++) line[c] = ctx->board[r][c];
        if (slide_line_left(line, &ctx->score)) changed = true;
        for (int c = 0; c < CELL_COUNT; c++) ctx->board[r][c] = line[c];
    }
    return changed;
}

static bool move_right(game_ctx_t *ctx)
{
    bool changed = false;
    for (int r = 0; r < CELL_COUNT; r++) {
        uint16_t line[CELL_COUNT];
        for (int c = 0; c < CELL_COUNT; c++) line[c] = ctx->board[r][CELL_COUNT - 1 - c];
        if (slide_line_left(line, &ctx->score)) changed = true;
        for (int c = 0; c < CELL_COUNT; c++) ctx->board[r][CELL_COUNT - 1 - c] = line[c];
    }
    return changed;
}

static bool move_up(game_ctx_t *ctx)
{
    bool changed = false;
    for (int c = 0; c < CELL_COUNT; c++) {
        uint16_t line[CELL_COUNT];
        for (int r = 0; r < CELL_COUNT; r++) line[r] = ctx->board[r][c];
        if (slide_line_left(line, &ctx->score)) changed = true;
        for (int r = 0; r < CELL_COUNT; r++) ctx->board[r][c] = line[r];
    }
    return changed;
}

static bool move_down(game_ctx_t *ctx)
{
    bool changed = false;
    for (int c = 0; c < CELL_COUNT; c++) {
        uint16_t line[CELL_COUNT];
        for (int r = 0; r < CELL_COUNT; r++) line[r] = ctx->board[CELL_COUNT - 1 - r][c];
        if (slide_line_left(line, &ctx->score)) changed = true;
        for (int r = 0; r < CELL_COUNT; r++) ctx->board[CELL_COUNT - 1 - r][c] = line[r];
    }
    return changed;
}

static bool is_game_over(const game_ctx_t *ctx)
{
    for (int r = 0; r < CELL_COUNT; r++) {
        for (int c = 0; c < CELL_COUNT; c++) {
            uint16_t v = ctx->board[r][c];
            if (v == 0) return false;
            if (r + 1 < CELL_COUNT && ctx->board[r + 1][c] == v) return false;
            if (c + 1 < CELL_COUNT && ctx->board[r][c + 1] == v) return false;
        }
    }
    return true;
}

static void game_root_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *owner = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *obj = (owner != NULL) ? owner : target;
    game_ctx_t *ctx = ctx_from_obj(obj);

    if (ctx == NULL) {
        return;
    }

    if (code == LV_EVENT_DELETE) {
        if (target == obj) {
            free_ctx(obj);
        }
        return;
    }

    if (code == LV_EVENT_PRESSED) {
        lv_indev_t *indev_pressed = lv_event_get_indev(e);
        if (indev_pressed == NULL) {
            indev_pressed = lv_indev_get_act();
        }
        if (indev_pressed != NULL) {
            lv_indev_get_point(indev_pressed, &ctx->press_point);
            ctx->pressed = true;
            ctx->moved_in_press = false;
        }
        return;
    }

    if (code == LV_EVENT_PRESSING) {
        if (!ctx->pressed || ctx->moved_in_press || ctx->over) {
            return;
        }
        lv_indev_t *indev_pressing = lv_event_get_indev(e);
        if (indev_pressing == NULL) {
            indev_pressing = lv_indev_get_act();
        }
        if (indev_pressing == NULL) {
            return;
        }
        lv_point_t cur = {0};
        lv_indev_get_point(indev_pressing, &cur);
        int dx = cur.x - ctx->press_point.x;
        int dy = cur.y - ctx->press_point.y;
        int ax = LV_ABS(dx);
        int ay = LV_ABS(dy);
        if (ax < 8 && ay < 8) {
            return;
        }

        bool moved_now = false;
        if (ax >= ay) {
            moved_now = (dx > 0) ? move_right(ctx) : move_left(ctx);
        } else {
            moved_now = (dy > 0) ? move_down(ctx) : move_up(ctx);
        }

        ctx->moved_in_press = true;
        if (moved_now) {
            add_random_tile(ctx);
            ctx->over = is_game_over(ctx);
            update_view(ctx);
            lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
        }
        return;
    }

    if (code != LV_EVENT_GESTURE && code != LV_EVENT_RELEASED) {
        return;
    }

    lv_indev_t *indev = lv_event_get_indev(e);
    if (indev == NULL) {
        indev = lv_indev_get_act();
    }
    if (indev == NULL || ctx->over) {
        return;
    }

    if (code == LV_EVENT_RELEASED) {
        if (ctx->moved_in_press) {
            ctx->pressed = false;
            ctx->moved_in_press = false;
            return;
        }
    }

    bool moved = false;
    lv_dir_t dir = LV_DIR_NONE;

    if (code == LV_EVENT_GESTURE) {
        dir = lv_indev_get_gesture_dir(indev);
    } else {
        /* Robust fallback: use press->release displacement. */
        if (!ctx->pressed) {
            return;
        }
        lv_point_t release_point = {0};
        lv_indev_get_point(indev, &release_point);
        int dx = release_point.x - ctx->press_point.x;
        int dy = release_point.y - ctx->press_point.y;
        int ax = LV_ABS(dx);
        int ay = LV_ABS(dy);
        ctx->pressed = false;
        ctx->moved_in_press = false;
        if (ax < 8 && ay < 8) {
            return;
        }
        if (ax >= ay) {
            dir = (dx > 0) ? LV_DIR_RIGHT : LV_DIR_LEFT;
        } else {
            dir = (dy > 0) ? LV_DIR_BOTTOM : LV_DIR_TOP;
        }
    }

    if (dir == LV_DIR_LEFT) {
        moved = move_left(ctx);
    } else if (dir == LV_DIR_RIGHT) {
        moved = move_right(ctx);
    } else if (dir == LV_DIR_TOP) {
        moved = move_up(ctx);
    } else if (dir == LV_DIR_BOTTOM) {
        moved = move_down(ctx);
    }

    if (moved) {
        add_random_tile(ctx);
        ctx->over = is_game_over(ctx);
        update_view(ctx);
        lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
    }
}

lv_obj_t *lv_100ask_2048_create(lv_obj_t *parent)
{
    ensure_seeded();

    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xb3a397), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);

    game_ctx_t *ctx = alloc_ctx(obj);
    if (ctx == NULL) {
        return obj;
    }

    int pad = 6;
    int cell_w = 46;
    int cell_h = 46;

    for (int r = 0; r < CELL_COUNT; r++) {
        for (int c = 0; c < CELL_COUNT; c++) {
            lv_obj_t *cell = lv_obj_create(obj);
            lv_obj_set_size(cell, cell_w, cell_h);
            lv_obj_set_pos(cell, c * (cell_w + pad), r * (cell_h + pad));
            lv_obj_set_style_radius(cell, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(cell, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(cell, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_flag(cell, LV_OBJ_FLAG_GESTURE_BUBBLE);

            lv_obj_t *label = lv_label_create(cell);
            lv_obj_set_style_text_font(label, &songti_font_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(label);
            lv_obj_add_flag(label, LV_OBJ_FLAG_GESTURE_BUBBLE);

            ctx->cell_obj[r][c] = cell;
            ctx->cell_label[r][c] = label;
        }
    }

    lv_obj_add_event_cb(obj, game_root_event_cb, LV_EVENT_PRESSED, obj);
    lv_obj_add_event_cb(obj, game_root_event_cb, LV_EVENT_PRESSING, obj);
    lv_obj_add_event_cb(obj, game_root_event_cb, LV_EVENT_GESTURE, obj);
    lv_obj_add_event_cb(obj, game_root_event_cb, LV_EVENT_RELEASED, obj);
    lv_obj_add_event_cb(obj, game_root_event_cb, LV_EVENT_DELETE, obj);

    /* Register same handlers on children to avoid missed touch events on some panels. */
    for (int r = 0; r < CELL_COUNT; r++) {
        for (int c = 0; c < CELL_COUNT; c++) {
            lv_obj_add_event_cb(ctx->cell_obj[r][c], game_root_event_cb, LV_EVENT_PRESSED, obj);
            lv_obj_add_event_cb(ctx->cell_obj[r][c], game_root_event_cb, LV_EVENT_PRESSING, obj);
            lv_obj_add_event_cb(ctx->cell_obj[r][c], game_root_event_cb, LV_EVENT_GESTURE, obj);
            lv_obj_add_event_cb(ctx->cell_obj[r][c], game_root_event_cb, LV_EVENT_RELEASED, obj);

            lv_obj_add_event_cb(ctx->cell_label[r][c], game_root_event_cb, LV_EVENT_PRESSED, obj);
            lv_obj_add_event_cb(ctx->cell_label[r][c], game_root_event_cb, LV_EVENT_PRESSING, obj);
            lv_obj_add_event_cb(ctx->cell_label[r][c], game_root_event_cb, LV_EVENT_GESTURE, obj);
            lv_obj_add_event_cb(ctx->cell_label[r][c], game_root_event_cb, LV_EVENT_RELEASED, obj);
        }
    }

    lv_100ask_2048_set_new_game(obj);
    return obj;
}

void lv_100ask_2048_set_new_game(lv_obj_t *obj)
{
    game_ctx_t *ctx = ctx_from_obj(obj);
    if (ctx == NULL) {
        return;
    }

    clear_board(ctx);
    add_random_tile(ctx);
    add_random_tile(ctx);
    update_view(ctx);
    lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
}

uint16_t lv_100ask_2048_get_best_tile(lv_obj_t *obj)
{
    game_ctx_t *ctx = ctx_from_obj(obj);
    if (ctx == NULL) {
        return 0;
    }

    uint16_t best = 0;
    for (int r = 0; r < CELL_COUNT; r++) {
        for (int c = 0; c < CELL_COUNT; c++) {
            if (ctx->board[r][c] > best) {
                best = ctx->board[r][c];
            }
        }
    }
    return best;
}

uint16_t lv_100ask_2048_get_score(lv_obj_t *obj)
{
    game_ctx_t *ctx = ctx_from_obj(obj);
    if (ctx == NULL) {
        return 0;
    }
    return (uint16_t)ctx->score;
}

bool lv_100ask_2048_get_status(lv_obj_t *obj)
{
    game_ctx_t *ctx = ctx_from_obj(obj);
    if (ctx == NULL) {
        return true;
    }
    return ctx->over;
}

#endif /* LV_USE_100ASK_2048 */
