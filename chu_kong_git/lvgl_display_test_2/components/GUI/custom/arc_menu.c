#include "arc_menu.h"
#include "gui_guider.h"

#include <dirent.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_heap_caps.h"

static const char *TAG = "ARC_MENU";
#define ARC_MENU_DEBUG_VISUAL 0

static lv_obj_t *s_arc_cont = NULL;
static lv_obj_t *s_arc_labels[MAX_WALLPAPERS];
static char s_wallpaper_full_paths[MAX_WALLPAPERS][512];
static char s_wallpaper_lvgl_paths[MAX_WALLPAPERS][512];
static char s_wallpaper_names[MAX_WALLPAPERS][96];
static int s_wallpaper_count = 0;
static float s_scroll_offset = 0.0f;
static bool s_is_open = false;
static bool s_dragging = false;
static int s_drag_accum = 0;
static uint32_t s_block_click_tick = 0;
static bool s_scanned_once = false;

static lv_img_dsc_t s_cached_wallpaper_dsc;
static uint8_t *s_cached_wallpaper_buf = NULL;
static char s_cached_wallpaper_src[512] = {0};
static char s_fallback_wallpaper_src[512] = {0};

#define WALL_STEP_LINES 12
#define WALL_STEP_PERIOD_MS 5

typedef struct {
    bool active;
    bool dec_opened;
    lv_timer_t *timer;
    lv_img_decoder_dsc_t dec;
    uint16_t w;
    uint16_t h;
    uint16_t cur_y;
    uint8_t *buf;
    char src[512];
    uint32_t start_tick;
} wallpaper_job_t;

static wallpaper_job_t s_job;

static void wallpaper_job_step_cb(lv_timer_t *t);

static float clampf_range(float val, float min_val, float max_val)
{
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

static float get_scroll_limit(void)
{
    const int item_gap = 36;
    const int item_h = 24;
    const int start_y = 12;
    int content_h = 0;
    if (s_wallpaper_count > 0) {
        content_h = start_y + ((s_wallpaper_count - 1) * item_gap) + item_h;
    }
    int overflow = content_h - ARC_CONT_HEIGHT;
    if (overflow < 0) overflow = 0;
    return (float)overflow;
}

static void extract_name_without_ext(const char *file_name, char *out, size_t out_len)
{
    if (out_len == 0) return;
    out[0] = '\0';
    if (file_name == NULL) return;

    const char *dot = strrchr(file_name, '.');
    size_t n = dot ? (size_t)(dot - file_name) : strlen(file_name);
    if (n >= out_len) n = out_len - 1;
    memcpy(out, file_name, n);
    out[n] = '\0';
}

static void scan_wallpapers(void)
{
    DIR *dir = opendir("/sdcard/wallpapers");
    if (dir == NULL) {
        ESP_LOGW(TAG, "failed to open /sdcard/wallpapers");
        s_wallpaper_count = 0;
        return;
    }

    s_wallpaper_count = 0;
    struct dirent *entry = NULL;
    while ((entry = readdir(dir)) != NULL && s_wallpaper_count < MAX_WALLPAPERS) {
        if (entry->d_type != DT_REG) continue;

        const char *ext = strrchr(entry->d_name, '.');
        if (ext == NULL) continue;
        if (strcasecmp(ext, ".png") != 0 && strcasecmp(ext, ".jpg") != 0 &&
            strcasecmp(ext, ".jpeg") != 0) {
            continue;
        }

        snprintf(s_wallpaper_full_paths[s_wallpaper_count],
                 sizeof(s_wallpaper_full_paths[s_wallpaper_count]),
                 "/sdcard/wallpapers/%s",
                 entry->d_name);
        snprintf(s_wallpaper_lvgl_paths[s_wallpaper_count],
                 sizeof(s_wallpaper_lvgl_paths[s_wallpaper_count]),
                 "S:/wallpapers/%s",
                 entry->d_name);
        extract_name_without_ext(entry->d_name,
                                 s_wallpaper_names[s_wallpaper_count],
                                 sizeof(s_wallpaper_names[s_wallpaper_count]));
        s_wallpaper_count++;
    }
    closedir(dir);

    s_scanned_once = true;
    ESP_LOGI(TAG, "scan done, wallpapers=%d", s_wallpaper_count);
}

static void free_cached_wallpaper(void)
{
    if (s_cached_wallpaper_buf != NULL) {
        heap_caps_free(s_cached_wallpaper_buf);
        s_cached_wallpaper_buf = NULL;
    }
    memset(&s_cached_wallpaper_dsc, 0, sizeof(s_cached_wallpaper_dsc));
    s_cached_wallpaper_src[0] = '\0';
}

static void wallpaper_job_cancel(void)
{
    if (!s_job.active) return;

    if (s_job.timer != NULL) {
        lv_timer_del(s_job.timer);
        s_job.timer = NULL;
    }
    if (s_job.dec_opened) {
        lv_img_decoder_close(&s_job.dec);
        s_job.dec_opened = false;
    }
    if (s_job.buf != NULL) {
        heap_caps_free(s_job.buf);
        s_job.buf = NULL;
    }

    memset(&s_job, 0, sizeof(s_job));
}

static void wallpaper_job_finish_apply(void)
{
    if (!s_job.active || s_job.buf == NULL || s_job.w == 0 || s_job.h == 0) {
        wallpaper_job_cancel();
        return;
    }

    size_t px_size = (size_t)s_job.w * (size_t)s_job.h * sizeof(lv_color_t);

    free_cached_wallpaper();
    s_cached_wallpaper_buf = s_job.buf;
    s_job.buf = NULL;

    memset(&s_cached_wallpaper_dsc, 0, sizeof(s_cached_wallpaper_dsc));
    s_cached_wallpaper_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    s_cached_wallpaper_dsc.header.w = s_job.w;
    s_cached_wallpaper_dsc.header.h = s_job.h;
    s_cached_wallpaper_dsc.data_size = px_size;
    s_cached_wallpaper_dsc.data = s_cached_wallpaper_buf;
    snprintf(s_cached_wallpaper_src, sizeof(s_cached_wallpaper_src), "%s", s_job.src);

    if (guider_ui.clock_screen_cont_2 != NULL) {
        lv_obj_set_style_bg_img_src(guider_ui.clock_screen_cont_2, &s_cached_wallpaper_dsc, 0);
    }

    ESP_LOGI(TAG, "wallpaper applied: %s (%ux%u) cost=%lu ms",
             s_job.src,
             (unsigned)s_job.w,
             (unsigned)s_job.h,
             (unsigned long)lv_tick_elaps(s_job.start_tick));

    wallpaper_job_cancel();
}

static void wallpaper_job_fail_fallback(void)
{
    if (guider_ui.clock_screen_cont_2 != NULL && s_job.src[0] != '\0') {
        /* Keep the file path in a persistent buffer. LVGL keeps a pointer to the string. */
        snprintf(s_fallback_wallpaper_src, sizeof(s_fallback_wallpaper_src), "%s", s_job.src);
        lv_obj_set_style_bg_img_src(guider_ui.clock_screen_cont_2, s_fallback_wallpaper_src, 0);
    }
    wallpaper_job_cancel();
}

static bool wallpaper_job_start(const char *lvgl_src)
{
    if (lvgl_src == NULL || lvgl_src[0] == '\0') return false;

    if (strcmp(s_cached_wallpaper_src, lvgl_src) == 0 && s_cached_wallpaper_buf != NULL) {
        if (guider_ui.clock_screen_cont_2 != NULL) {
            lv_obj_set_style_bg_img_src(guider_ui.clock_screen_cont_2, &s_cached_wallpaper_dsc, 0);
        }
        return true;
    }

    wallpaper_job_cancel();

    memset(&s_job, 0, sizeof(s_job));
    snprintf(s_job.src, sizeof(s_job.src), "%s", lvgl_src);
    s_job.start_tick = lv_tick_get();

    lv_res_t ret = lv_img_decoder_open(&s_job.dec, lvgl_src, lv_color_black(), 0);
    if (ret != LV_RES_OK) {
        ESP_LOGW(TAG, "decoder open failed: %s", lvgl_src);
        wallpaper_job_cancel();
        return false;
    }
    s_job.dec_opened = true;
    s_job.w = s_job.dec.header.w;
    s_job.h = s_job.dec.header.h;
    if (s_job.w == 0 || s_job.h == 0) {
        wallpaper_job_cancel();
        return false;
    }

    size_t px_size = (size_t)s_job.w * (size_t)s_job.h * sizeof(lv_color_t);
    if (px_size > (size_t)512 * 1024) {
        ESP_LOGW(TAG, "wallpaper too large for cache (%u bytes), fallback: %s",
                 (unsigned)px_size, lvgl_src);
        wallpaper_job_fail_fallback();
        return false;
    }

    s_job.buf = (uint8_t *)heap_caps_malloc(px_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_job.buf == NULL) {
        s_job.buf = (uint8_t *)heap_caps_malloc(px_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    if (s_job.buf == NULL) {
        ESP_LOGW(TAG, "cache alloc failed, size=%u", (unsigned)px_size);
        wallpaper_job_fail_fallback();
        return false;
    }

    s_job.cur_y = 0;
    s_job.active = true;
    s_job.timer = lv_timer_create(wallpaper_job_step_cb, WALL_STEP_PERIOD_MS, NULL);
    if (s_job.timer == NULL) {
        ESP_LOGW(TAG, "lv_timer_create failed");
        wallpaper_job_fail_fallback();
        return false;
    }

    return true;
}

static void update_arc_item_positions(void)
{
    if (s_arc_cont == NULL || s_wallpaper_count == 0) return;

    const int item_gap = 36;   /* bigger spacing */
    const int item_h = 24;
    const int start_y = 12;
    for (int i = 0; i < s_wallpaper_count; i++) {
        int y = start_y + (i * item_gap) + (int)s_scroll_offset;
        if (y < -item_h || y > (ARC_CONT_HEIGHT - item_h)) {
            lv_obj_add_flag(s_arc_labels[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_clear_flag(s_arc_labels[i], LV_OBJ_FLAG_HIDDEN);

        int dy = y - ARC_CENTER_Y;
        int rr = ARC_RADIUS * ARC_RADIUS - dy * dy;
        int x = 6;
        if (rr > 0) {
            float x_arc = ARC_CENTER_X + sqrtf((float)rr);
            x = (int)x_arc;
        }
        const int label_w = 92;
        if (x < 2) x = 2;
        if (x > ARC_CONT_WIDTH - label_w - 2) x = ARC_CONT_WIDTH - label_w - 2;
        lv_obj_set_pos(s_arc_labels[i], x, y);
    }
}

static void arc_item_click_cb(lv_event_t *e)
{
    if (s_dragging || lv_tick_elaps(s_block_click_tick) < 180) {
        return;
    }

    lv_obj_t *label = lv_event_get_target(e);
    int idx = (int)(uintptr_t)lv_obj_get_user_data(label);
    if (idx < 0 || idx >= s_wallpaper_count) return;

    if (strlen(s_wallpaper_full_paths[idx]) > 0) {
        const char *src = s_wallpaper_lvgl_paths[idx];
        arc_menu_close();
        if (!wallpaper_job_start(src)) {
            lv_obj_set_style_bg_img_src(guider_ui.clock_screen_cont_2, src, 0);
        }
        return;
    }

    arc_menu_close();
}

static void wallpaper_job_step_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    if (!s_job.active || !s_job.dec_opened || s_job.buf == NULL) {
        wallpaper_job_cancel();
        return;
    }

    for (int i = 0; i < WALL_STEP_LINES; i++) {
        if (s_job.cur_y >= s_job.h) {
            wallpaper_job_finish_apply();
            return;
        }

        uint8_t *line = (uint8_t *)(s_job.buf + ((size_t)s_job.cur_y * s_job.w * sizeof(lv_color_t)));
        if (lv_img_decoder_read_line(&s_job.dec, 0, s_job.cur_y, s_job.w, line) != LV_RES_OK) {
            ESP_LOGW(TAG, "decoder read_line failed y=%u src=%s", (unsigned)s_job.cur_y, s_job.src);
            wallpaper_job_fail_fallback();
            return;
        }
        s_job.cur_y++;
    }
}

static void arc_scroll_event_cb(lv_event_t *e)
{
    static int16_t last_x = 0;
    static int16_t last_y = 0;
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        lv_point_t p;
        lv_indev_get_point(lv_indev_get_act(), &p);
        last_x = p.x;
        last_y = p.y;
        s_dragging = false;
        s_drag_accum = 0;
        return;
    }

    if (code == LV_EVENT_RELEASED) {
        if (s_dragging) {
            // Ignore the click generated right after a drag gesture.
            s_block_click_tick = lv_tick_get();
        }
        s_dragging = false;
        s_drag_accum = 0;
        return;
    }

    if (code != LV_EVENT_PRESSING) return;

    lv_point_t p;
    lv_indev_get_point(lv_indev_get_act(), &p);

    int diff_x = p.x - last_x;
    int diff_y = p.y - last_y;
    last_x = p.x;
    last_y = p.y;

    s_drag_accum += LV_ABS(diff_y) + (LV_ABS(diff_x) / 2);
    if (s_drag_accum > 8) {
        s_dragging = true;
    }

    if (diff_x < -50 && LV_ABS(diff_x) > (LV_ABS(diff_y) + 8)) {
        arc_menu_close();
        return;
    }

    if (LV_ABS(diff_y) >= 1) {
        s_scroll_offset += diff_y * 1.35f;
        s_scroll_offset = clampf_range(s_scroll_offset, -get_scroll_limit(), 0.0f);
        update_arc_item_positions();
    }
}

static void anim_x_cb(void *var, int32_t v)
{
    lv_obj_set_x((lv_obj_t *)var, v);
}

void arc_menu_open(void)
{
    if (s_arc_cont == NULL || s_is_open) return;
    s_is_open = true;
    lv_obj_move_foreground(s_arc_cont);

    if (!s_scanned_once) {
        scan_wallpapers();
    }
    s_scroll_offset = 0.0f;
    for (int i = 0; i < MAX_WALLPAPERS; i++) {
        if (i < s_wallpaper_count) {
            lv_label_set_text(s_arc_labels[i], s_wallpaper_names[i]);
            lv_obj_clear_flag(s_arc_labels[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_label_set_text(s_arc_labels[i], "");
            lv_obj_add_flag(s_arc_labels[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    update_arc_item_positions();

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_arc_cont);
    lv_anim_set_values(&a, -ARC_CONT_WIDTH, 0);
    lv_anim_set_time(&a, 260);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)anim_x_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

void arc_menu_close(void)
{
    if (s_arc_cont == NULL || !s_is_open) return;
    s_is_open = false;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_arc_cont);
    lv_anim_set_values(&a, 0, -ARC_CONT_WIDTH);
    lv_anim_set_time(&a, 220);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)anim_x_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_start(&a);
}

bool arc_menu_is_open(void)
{
    return s_is_open;
}

void arc_menu_init(lv_obj_t *parent)
{
    s_arc_cont = lv_obj_create(parent);
    lv_obj_set_size(s_arc_cont, ARC_CONT_WIDTH, ARC_CONT_HEIGHT);
    lv_obj_set_pos(s_arc_cont, -ARC_CONT_WIDTH, 0);
    lv_obj_set_scrollbar_mode(s_arc_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(s_arc_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_arc_cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_arc_cont, LV_OBJ_FLAG_GESTURE_BUBBLE);

    /* Debug first: make drawer visible to confirm render path. */
#if ARC_MENU_DEBUG_VISUAL
    lv_obj_set_style_bg_opa(s_arc_cont, LV_OPA_60, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_arc_cont, lv_color_hex(0x202020), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_arc_cont, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_arc_cont, lv_color_hex(0x66CCFF), LV_PART_MAIN | LV_STATE_DEFAULT);
#else
    lv_obj_set_style_bg_opa(s_arc_cont, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_arc_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
#endif
    lv_obj_set_style_pad_all(s_arc_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (!s_scanned_once) {
        scan_wallpapers();
    }

    /* Re-apply cached wallpaper when clock screen is recreated. */
    if (s_cached_wallpaper_buf != NULL && guider_ui.clock_screen_cont_2 != NULL) {
        lv_obj_set_style_bg_img_src(guider_ui.clock_screen_cont_2, &s_cached_wallpaper_dsc, 0);
    }

    for (int i = 0; i < MAX_WALLPAPERS; i++) {
        s_arc_labels[i] = lv_label_create(s_arc_cont);
        lv_obj_set_size(s_arc_labels[i], 92, 20);
        lv_obj_set_style_text_align(s_arc_labels[i], LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(s_arc_labels[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(s_arc_labels[i], LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(s_arc_labels[i], LV_FONT_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(s_arc_labels[i], LV_OPA_60, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(s_arc_labels[i], lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(s_arc_labels[i], 4, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_transform_zoom(s_arc_labels[i], 256, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_opa(s_arc_labels[i], LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_long_mode(s_arc_labels[i], LV_LABEL_LONG_CLIP);
        lv_label_set_text(s_arc_labels[i], (i < s_wallpaper_count) ? s_wallpaper_names[i] : "");
        if (i >= s_wallpaper_count) {
            lv_obj_add_flag(s_arc_labels[i], LV_OBJ_FLAG_HIDDEN);
        }

        lv_obj_add_flag(s_arc_labels[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(s_arc_labels[i], LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_user_data(s_arc_labels[i], (void *)(uintptr_t)i);
        lv_obj_add_event_cb(s_arc_labels[i], arc_item_click_cb, LV_EVENT_CLICKED, NULL);
    }

    update_arc_item_positions();
    lv_obj_add_event_cb(s_arc_cont, arc_scroll_event_cb, LV_EVENT_ALL, NULL);
}
