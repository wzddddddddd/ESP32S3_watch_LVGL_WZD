#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "st7789_driver.h"
#include "cst816t_driver.h"
#include "rtc_time_service.h"
#include "driver/gpio.h"
#include "lv_port.h"
#include <string.h>

#define TAG "lv_port"

#define LCD_WIDTH  240
#define LCD_HEIGHT 284

#define FLUSH_WAIT_TIMEOUT_MS 150U

static lv_disp_drv_t disp_drv;

/* Set by power_sleep_wakeup(), used to debounce touch right after wakeup. */
extern volatile uint32_t g_wake_tick;

static lv_color_t *s_bounce_buf = NULL;
static uint16_t s_bounce_lines = 0;
static volatile bool s_flush_done = true;
static SemaphoreHandle_t s_flush_sem = NULL;
static uint32_t s_last_no_bounce_log_ms = 0;
static uint32_t s_last_bounce_retry_ms = 0;

static portMUX_TYPE s_flush_state_lock = portMUX_INITIALIZER_UNLOCKED;
static display_flush_state_t s_flush_state = {0};

static inline bool is_internal_ram(const void *p)
{
    if (p == NULL) return false;
    return !esp_ptr_external_ram(p);
}

static inline uint32_t now_ms_from_tick(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

static void flush_state_mark_start(bool use_bounce)
{
    portENTER_CRITICAL(&s_flush_state_lock);
    s_flush_state.flush_in_progress = true;
    s_flush_state.flush_count++;
    s_flush_state.last_flush_start_ms = now_ms_from_tick();
    s_flush_state.draw_buf_internal = !use_bounce;
    s_flush_state.bounce_available = (s_bounce_buf != NULL);
    s_flush_state.bounce_lines = s_bounce_lines;
    portEXIT_CRITICAL(&s_flush_state_lock);
}

static void flush_state_mark_done(void)
{
    portENTER_CRITICAL(&s_flush_state_lock);
    s_flush_state.flush_in_progress = false;
    s_flush_state.last_flush_end_ms = now_ms_from_tick();
    portEXIT_CRITICAL(&s_flush_state_lock);
}

static void flush_state_mark_timeout(void)
{
    portENTER_CRITICAL(&s_flush_state_lock);
    s_flush_state.flush_wait_timeouts++;
    portEXIT_CRITICAL(&s_flush_state_lock);
}

void lv_port_get_flush_state(display_flush_state_t *out_state)
{
    if (out_state == NULL) return;
    portENTER_CRITICAL(&s_flush_state_lock);
    *out_state = s_flush_state;
    portEXIT_CRITICAL(&s_flush_state_lock);
}

void lv_flush_done_cb(void *param)
{
    (void)param;
    s_flush_done = true;

    BaseType_t higher_woken = pdFALSE;
    if (s_flush_sem != NULL) {
        xSemaphoreGiveFromISR(s_flush_sem, &higher_woken);
        if (higher_woken == pdTRUE) {
            portYIELD_FROM_ISR();
        }
    }
}

static bool wait_flush_done(TickType_t timeout_ticks)
{
    if (s_flush_sem != NULL) {
        if (xSemaphoreTake(s_flush_sem, timeout_ticks) == pdTRUE) {
            return true;
        }
        flush_state_mark_timeout();
        return false;
    }

    TickType_t start_tick = xTaskGetTickCount();
    while (!s_flush_done) {
        if ((xTaskGetTickCount() - start_tick) > timeout_ticks) {
            flush_state_mark_timeout();
            s_flush_done = true;
            return false;
        }
        vTaskDelay(1);
    }
    return true;
}

static bool flush_transfer_once(int x1, int x2, int y1, int y2, lv_color_t *src)
{
    if (x2 <= x1 || y2 <= y1 || src == NULL) {
        return true;
    }

    if (s_flush_sem != NULL) {
        while (xSemaphoreTake(s_flush_sem, 0) == pdTRUE) {
        }
    }

    s_flush_done = false;
    st7789_flush(x1, x2, y1, y2, src);
    return wait_flush_done(pdMS_TO_TICKS(FLUSH_WAIT_TIMEOUT_MS));
}

static bool alloc_bounce_buffer(void)
{
    const uint16_t line_candidates[] = {40, 20, 10, 8, 6, 4, 2, 1};

    if (s_bounce_buf != NULL) {
        heap_caps_free(s_bounce_buf);
        s_bounce_buf = NULL;
        s_bounce_lines = 0;
    }

    for (size_t i = 0; i < sizeof(line_candidates) / sizeof(line_candidates[0]); ++i) {
        uint16_t lines = line_candidates[i];
        size_t px_count = (size_t)LCD_WIDTH * lines;
        lv_color_t *tmp = heap_caps_malloc(px_count * sizeof(lv_color_t),
                                           MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        if (tmp != NULL) {
            s_bounce_buf = tmp;
            s_bounce_lines = lines;
            ESP_LOGI(TAG, "bounce buffer ready: %u lines, %u bytes",
                     (unsigned int)lines,
                     (unsigned int)(px_count * sizeof(lv_color_t)));
            return true;
        }
    }

    ESP_LOGE(TAG, "bounce buffer alloc failed");
    return false;
}

void disp_flush(struct _lv_disp_drv_t *drv,
                const lv_area_t *area, lv_color_t *color_p)
{
    int x1 = area->x1;
    int x2 = area->x2 + 1;
    int y1 = area->y1;
    int y2 = area->y2 + 1;
    bool use_bounce = !is_internal_ram(color_p);

    flush_state_mark_start(use_bounce);

    if (!use_bounce) {
        if (!flush_transfer_once(x1, x2, y1, y2, color_p)) {
            ESP_LOGW(TAG, "direct flush timeout");
        }
        flush_state_mark_done();
        lv_disp_flush_ready(drv);
        return;
    }

    if (s_bounce_buf == NULL || s_bounce_lines == 0) {
        uint32_t now = now_ms_from_tick();
        if ((now - s_last_bounce_retry_ms) > 1000U) {
            s_last_bounce_retry_ms = now;
            if (alloc_bounce_buffer()) {
                portENTER_CRITICAL(&s_flush_state_lock);
                s_flush_state.bounce_available = true;
                s_flush_state.bounce_lines = s_bounce_lines;
                portEXIT_CRITICAL(&s_flush_state_lock);
            }
        }
        if (s_bounce_buf == NULL || s_bounce_lines == 0) {
            if ((now - s_last_no_bounce_log_ms) > 2000U) {
                s_last_no_bounce_log_ms = now;
                ESP_LOGE(TAG, "PSRAM flush skipped: no bounce buffer");
            }
            flush_state_mark_done();
            lv_disp_flush_ready(drv);
            return;
        }
    }

    if (s_bounce_buf == NULL || s_bounce_lines == 0) {
        flush_state_mark_done();
        lv_disp_flush_ready(drv);
        return;
    }

    int w = x2 - x1;
    int total_lines = y2 - y1;
    int sent_lines = 0;
    bool flush_ok = true;

    while (sent_lines < total_lines) {
        int chunk = s_bounce_lines;
        if (chunk > total_lines - sent_lines) {
            chunk = total_lines - sent_lines;
        }

        int pixels = w * chunk;
        memcpy(s_bounce_buf, color_p + (sent_lines * w), pixels * sizeof(lv_color_t));

        if (!flush_transfer_once(x1, x2,
                                 y1 + sent_lines, y1 + sent_lines + chunk,
                                 s_bounce_buf)) {
            flush_ok = false;
            break;
        }

        sent_lines += chunk;
    }

    if (!flush_ok) {
        ESP_LOGW(TAG, "bounce flush timeout, continue");
        s_flush_done = true;
    }

    flush_state_mark_done();
    lv_disp_flush_ready(drv);
}

void lv_disp_init(void)
{
    static lv_disp_draw_buf_t disp_buf;

    ESP_LOGI(TAG, "=== display init begin ===");
    ESP_LOGI(TAG, "internal free: %d KB",
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024);
    ESP_LOGI(TAG, "internal DMA free: %d KB",
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA) / 1024);
    ESP_LOGI(TAG, "PSRAM free: %d KB",
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);

    lv_color_t *disp1 = NULL;
    lv_color_t *disp2 = NULL;
    size_t buf_size = 0;
    bool use_psram_draw = false;

    /* Prefer internal DMA draw buffers first, degrade line count progressively. */
    const uint16_t internal_lines[] = {40, 20, 12, 10, 8, 6, 4, 2, 1};
    for (size_t i = 0; i < sizeof(internal_lines) / sizeof(internal_lines[0]); ++i) {
        uint16_t lines = internal_lines[i];
        buf_size = LCD_WIDTH * lines;
        disp1 = heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        disp2 = heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        if (disp1 != NULL && disp2 != NULL) {
            ESP_LOGI(TAG, "internal DMA draw buffer: %u-line double", (unsigned int)lines);
            break;
        }

        if (disp1 != NULL) { heap_caps_free(disp1); disp1 = NULL; }
        if (disp2 != NULL) { heap_caps_free(disp2); disp2 = NULL; }

        disp1 = heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        disp2 = NULL;
        if (disp1 != NULL) {
            ESP_LOGW(TAG, "fallback to internal %u-line single buffer", (unsigned int)lines);
            break;
        }
    }

    /* Last resort: PSRAM draw buffer + bounce copy for DMA transfer. */
    if (disp1 == NULL) {
        const uint16_t psram_lines[] = {40, 20, 10, 4, 2};
        for (size_t i = 0; i < sizeof(psram_lines) / sizeof(psram_lines[0]); ++i) {
            uint16_t lines = psram_lines[i];
            buf_size = LCD_WIDTH * lines;
            disp1 = heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            disp2 = heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (disp1 != NULL && disp2 != NULL) {
                use_psram_draw = true;
                ESP_LOGW(TAG, "fallback to PSRAM %u-line double buffer", (unsigned int)lines);
                break;
            }

            if (disp1 != NULL) { heap_caps_free(disp1); disp1 = NULL; }
            if (disp2 != NULL) { heap_caps_free(disp2); disp2 = NULL; }

            disp1 = heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            disp2 = NULL;
            if (disp1 != NULL) {
                use_psram_draw = true;
                ESP_LOGW(TAG, "fallback to PSRAM %u-line single buffer", (unsigned int)lines);
                break;
            }
        }
    }

    if (!disp1) {
        ESP_LOGE(TAG, "draw buffer alloc failed");
        return;
    }

    if (use_psram_draw) {
        if (!alloc_bounce_buffer()) {
            ESP_LOGW(TAG, "PSRAM draw buffer active without bounce; flush will be skipped until retry succeeds");
        }
    }

    lv_disp_draw_buf_init(&disp_buf, disp1, disp2, buf_size);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = disp_flush;
    disp_drv.rotated = LV_DISP_ROT_NONE;
    lv_disp_drv_register(&disp_drv);

    portENTER_CRITICAL(&s_flush_state_lock);
    s_flush_state.draw_buf_internal = is_internal_ram(disp1);
    s_flush_state.bounce_available = (s_bounce_buf != NULL);
    s_flush_state.bounce_lines = s_bounce_lines;
    portEXIT_CRITICAL(&s_flush_state_lock);

    ESP_LOGI(TAG, "draw buffer: %u px, %s buffer, %s memory",
             (unsigned int)buf_size,
             disp2 ? "double" : "single",
             is_internal_ram(disp1) ? "internal" : "PSRAM");
}

void indev_read(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    (void)indev_drv;

    /* Ignore touch for 500ms after wakeup. */
    if (g_wake_tick != 0 &&
        (xTaskGetTickCount() - g_wake_tick) < pdMS_TO_TICKS(500)) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    int16_t x, y;
    int state;
    cst816t_read(&x, &y, &state);

    x = LCD_WIDTH - 1 - x;

    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= LCD_WIDTH) x = LCD_WIDTH - 1;
    if (y >= LCD_HEIGHT) y = LCD_HEIGHT - 1;

    y -= 20;
    if (y < 0) y = 0;
    if (y >= LCD_HEIGHT) y = LCD_HEIGHT - 1;

    data->point.x = x;
    data->point.y = y;
    data->state = state;
}

void lv_indev_init(void)
{
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = indev_read;
    lv_indev_drv_register(&indev_drv);
}

void st7789_hw_init(void)
{
    st7789_cfg_t st7789_config = {
        .bl = GPIO_NUM_45,
        .clk = GPIO_NUM_12,
        .cs = GPIO_NUM_13,
        .dc = GPIO_NUM_10,
        .mosi = GPIO_NUM_11,
        .rst = GPIO_NUM_14,
        .spi_fre = 80 * 1000 * 1000,
        .height = LCD_HEIGHT,
        .width = LCD_WIDTH,
        .spin = 2,
        .done_cb = lv_flush_done_cb,
        .cb_param = &disp_drv,
    };

    st7789_driver_hw_init(&st7789_config);
}

void cst816t_hw_init(void)
{
    cst816t_cfg_t cst816t_config = {
        .scl = GPIO_NUM_18,
        .sda = GPIO_NUM_17,
        .fre = 400 * 1000,
        .x_limit = LCD_WIDTH,
        .y_limit = LCD_HEIGHT,
    };
    cst816t_init(&cst816t_config);
}

void lv_timer_cb(void *arg)
{
    uint32_t tick_interval = *((uint32_t *)arg);
    lv_tick_inc(tick_interval);
}

void lv_tick_init(void)
{
    static uint32_t tick_interval = 5;
    const esp_timer_create_args_t arg = {
        .arg = &tick_interval,
        .callback = lv_timer_cb,
        .name = "lv_tick",
        .dispatch_method = ESP_TIMER_TASK,
        .skip_unhandled_events = true,
    };

    esp_timer_handle_t timer_handle;
    esp_timer_create(&arg, &timer_handle);
    esp_timer_start_periodic(timer_handle, tick_interval * 1000);
}

void lv_port_init(void)
{
    if (s_flush_sem == NULL) {
        s_flush_sem = xSemaphoreCreateBinary();
        if (s_flush_sem == NULL) {
            ESP_LOGE(TAG, "flush semaphore create failed");
        }
    }

    lv_init();
    st7789_hw_init();
    cst816t_hw_init();
    esp_err_t rtc_ret = rtc_time_service_init_shared_i2c();
    if (rtc_ret == ESP_OK) {
        rtc_ret = rtc_time_service_boot_sync_system_from_rtc();
    }
    if (rtc_ret != ESP_OK) {
        ESP_LOGW(TAG, "rtc init/sync skipped: %s", esp_err_to_name(rtc_ret));
    }
    lv_disp_init();
    lv_indev_init();
    lv_tick_init();
}
