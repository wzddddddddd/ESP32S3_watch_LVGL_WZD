#include "power_sleep.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/rtc_io.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"

#include "SD_card.h"
#include "cst816t_driver.h"
#include "lvgl_display.h"
#include "peripheral_sleep.h"
#include "st7789_driver.h"
#include "storage_worker.h"
#include "video_player.h"
#include "wifi_manager.h"

#define TAG "POWER_SLEEP"

#define SLEEP_TIMEOUT_SECONDS 120U
#define POWER_SLEEP_POLL_MS   200U

#define GPIO_KEY_DOWN   GPIO_NUM_5
#define GPIO_KEY_UP     GPIO_NUM_6
#define GPIO_KEY_OK     GPIO_NUM_7
#define GPIO_TOUCH_INT  GPIO_NUM_16
#define GPIO_I2C_SDA    GPIO_NUM_17
#define GPIO_I2C_SCL    GPIO_NUM_18
#define GPIO_LCD_DC     GPIO_NUM_10
#define GPIO_LCD_MOSI   GPIO_NUM_11
#define GPIO_LCD_CLK    GPIO_NUM_12
#define GPIO_LCD_CS     GPIO_NUM_13
#define GPIO_LCD_RST    GPIO_NUM_14
#define GPIO_SD_CLK     GPIO_NUM_39
#define GPIO_SD_MOSI    GPIO_NUM_40
#define GPIO_SD_MISO    GPIO_NUM_41
#define GPIO_SD_CS      GPIO_NUM_42
#define GPIO_LCD_BL     GPIO_NUM_45

static const gpio_num_t s_used_gpio_whitelist[] = {
    GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_7,
    GPIO_NUM_10, GPIO_NUM_11, GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14,
    GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_18,
    GPIO_NUM_39, GPIO_NUM_40, GPIO_NUM_41, GPIO_NUM_42,
    GPIO_NUM_45,
};

static const gpio_num_t s_protected_gpio_list[] = {
    GPIO_NUM_0, GPIO_NUM_3,
    GPIO_NUM_19, GPIO_NUM_20,
    GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_28, GPIO_NUM_29, GPIO_NUM_30, GPIO_NUM_31,
    GPIO_NUM_32, GPIO_NUM_33, GPIO_NUM_34, GPIO_NUM_35, GPIO_NUM_36, GPIO_NUM_37,
    GPIO_NUM_43, GPIO_NUM_44, GPIO_NUM_46,
};

typedef struct {
    gpio_num_t pin;
    uint32_t level;
} held_gpio_cfg_t;

static const held_gpio_cfg_t s_hold_low_power_pins[] = {
    {GPIO_LCD_DC, 0},
    {GPIO_LCD_MOSI, 0},
    {GPIO_LCD_CLK, 0},
    {GPIO_LCD_CS, 1},
    {GPIO_LCD_RST, 0},
    {GPIO_SD_CLK, 0},
    {GPIO_SD_MOSI, 0},
    {GPIO_SD_CS, 1},
    {GPIO_LCD_BL, 0},
};

static volatile uint32_t s_last_activity_time = 0;
static volatile bool s_force_sleep = false;
static volatile bool s_force_deep_sleep = false;
static bool s_sleep_initialized = false;
static bool s_boot_initialized = false;
static bool s_need_return_video_list = false;
static bool s_wifi_resume_after_light_sleep = false;
static volatile bool s_wake_key_guard_active = false;
static int32_t s_last_logged_remain = -1;

extern void st7789_hw_init(void);
extern volatile bool g_is_sleeping;
extern volatile uint32_t g_wake_tick;

static inline uint32_t power_sleep_now_seconds(void)
{
    TickType_t tick = xPortInIsrContext() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();
    return (uint32_t)((tick * portTICK_PERIOD_MS) / 1000U);
}

static bool power_sleep_gpio_in_list(gpio_num_t pin, const gpio_num_t *list, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i) {
        if (list[i] == pin) {
            return true;
        }
    }
    return false;
}

static bool power_sleep_gpio_is_used_or_protected(gpio_num_t pin)
{
    if (power_sleep_gpio_in_list(pin, s_used_gpio_whitelist,
                                 sizeof(s_used_gpio_whitelist) / sizeof(s_used_gpio_whitelist[0]))) {
        return true;
    }
    if (power_sleep_gpio_in_list(pin, s_protected_gpio_list,
                                 sizeof(s_protected_gpio_list) / sizeof(s_protected_gpio_list[0]))) {
        return true;
    }
    return false;
}

static void power_sleep_release_boot_holds(void)
{
    size_t i;

    gpio_deep_sleep_hold_dis();
    rtc_gpio_deinit(GPIO_KEY_OK);
    for (i = 0; i < (sizeof(s_hold_low_power_pins) / sizeof(s_hold_low_power_pins[0])); ++i) {
        gpio_hold_dis(s_hold_low_power_pins[i].pin);
    }
}

static void power_sleep_hold_output(gpio_num_t pin, uint32_t level)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&io_conf) != ESP_OK) {
        return;
    }
    gpio_set_level(pin, level);
    gpio_hold_en(pin);
}

static void power_sleep_set_output_level(gpio_num_t pin, uint32_t level)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&io_conf) != ESP_OK) {
        return;
    }
    gpio_set_level(pin, level);
}

static void power_sleep_set_pin_disable_float(gpio_num_t pin)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(pin),
        .mode = GPIO_MODE_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&io_conf);
}

static void power_sleep_prepare_misc_inputs(void)
{
    gpio_config_t key_conf = {
        .pin_bit_mask = BIT64(GPIO_KEY_DOWN) | BIT64(GPIO_KEY_UP),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&key_conf);

    gpio_config_t touch_conf = {
        .pin_bit_mask = BIT64(GPIO_TOUCH_INT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&touch_conf);

    gpio_config_t i2c_conf = {
        .pin_bit_mask = BIT64(GPIO_I2C_SDA) | BIT64(GPIO_I2C_SCL),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&i2c_conf);

    gpio_config_t sd_miso_conf = {
        .pin_bit_mask = BIT64(GPIO_SD_MISO),
        .mode = GPIO_MODE_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&sd_miso_conf);
}

static void light_sleep_io_prepare_fast(void)
{
    gpio_config_t key_conf = {
        .pin_bit_mask = BIT64(GPIO_KEY_DOWN) | BIT64(GPIO_KEY_UP),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&key_conf);

    gpio_config_t touch_conf = {
        .pin_bit_mask = BIT64(GPIO_TOUCH_INT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&touch_conf);

    power_sleep_set_output_level(GPIO_LCD_BL, 0);
}

static void power_sleep_wait_wake_key_release(void)
{
    gpio_config_t key_ok_conf = {
        .pin_bit_mask = BIT64(GPIO_KEY_OK),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&key_ok_conf);

    while (gpio_get_level(GPIO_KEY_OK) == 1) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void power_sleep_prepare_ext0_wakeup(void)
{
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

    rtc_gpio_init(GPIO_KEY_OK);
    rtc_gpio_set_direction(GPIO_KEY_OK, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_dis(GPIO_KEY_OK);
    rtc_gpio_pulldown_en(GPIO_KEY_OK);

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_enable_ext0_wakeup(GPIO_KEY_OK, 1);
}

static void deep_sleep_io_prepare_full(void)
{
    size_t i;
    int pin_num;

    power_sleep_prepare_misc_inputs();

    for (pin_num = 0; pin_num < GPIO_NUM_MAX; ++pin_num) {
        gpio_num_t pin = (gpio_num_t)pin_num;

        if (!GPIO_IS_VALID_GPIO(pin)) {
            continue;
        }
        if (power_sleep_gpio_is_used_or_protected(pin)) {
            continue;
        }

        power_sleep_set_pin_disable_float(pin);
        if (RTC_GPIO_IS_VALID_GPIO(pin)) {
            rtc_gpio_isolate(pin);
        }
    }

    for (i = 0; i < (sizeof(s_hold_low_power_pins) / sizeof(s_hold_low_power_pins[0])); ++i) {
        power_sleep_hold_output(s_hold_low_power_pins[i].pin, s_hold_low_power_pins[i].level);
    }

    gpio_deep_sleep_hold_en();
    esp_sleep_config_gpio_isolate();
}

void power_sleep_boot_init(void)
{
    esp_sleep_wakeup_cause_t cause;

    if (s_boot_initialized) {
        return;
    }
    s_boot_initialized = true;

    power_sleep_release_boot_holds();

    cause = esp_sleep_get_wakeup_cause();
    s_wake_key_guard_active = (cause == ESP_SLEEP_WAKEUP_EXT0);
    if (s_wake_key_guard_active) {
        ESP_LOGI(TAG, "boot from deep sleep by GPIO7");
    }
}

void power_sleep_reset_timer(void)
{
    s_last_activity_time = power_sleep_now_seconds();
}

void power_sleep_request_sleep(void)
{
    s_force_sleep = true;
}

void power_sleep_request_deep_sleep(void)
{
    s_force_deep_sleep = true;
}

bool power_sleep_wake_key_guard_active(void)
{
    return s_wake_key_guard_active;
}

void power_sleep_clear_wake_key_guard(void)
{
    s_wake_key_guard_active = false;
}

static void power_sleep_wakeup_light_sleep(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    ESP_LOGI(TAG, "wake from light sleep");

    if (cause == ESP_SLEEP_WAKEUP_GPIO) {
        cst816t_reinit();
    }

    st7789_hw_init();
    st7789_lcd_backlight(1);

    vTaskDelay(pdMS_TO_TICKS(100));

    if (s_wifi_resume_after_light_sleep) {
        wifi_manager_start();
    }
    s_wifi_resume_after_light_sleep = false;

    g_wake_tick = xTaskGetTickCount();
    g_is_sleeping = false;
    lvgl_runtime_clear_pause_ack();

    if (s_need_return_video_list) {
        s_need_return_video_list = false;
        lvgl_msg_send_nonblocking(LVGL_MSG_RETURN_TO_VIDEO_LIST, 0, NULL);
    }

    power_sleep_reset_timer();
    system_diag_snapshot("wakeup");
}

static void power_sleep_enter_light_sleep(void)
{
    s_force_sleep = false;
    s_last_logged_remain = -1;

    ESP_LOGI(TAG, "enter light sleep");

    lvgl_runtime_clear_pause_ack();
    g_is_sleeping = true;
    if (!lvgl_wait_for_pause_ack(600)) {
        ESP_LOGW(TAG, "LVGL pause ACK timeout, continue sleep path");
        system_diag_snapshot("sleep-ack-timeout");
    }

    if (video_player_is_playing()) {
        ESP_LOGI(TAG, "stopping video player before light sleep");
        if (!video_player_stop_wait(3000)) {
            ESP_LOGW(TAG, "video stop wait timeout in sleep path");
        }
        s_need_return_video_list = true;
    }

    st7789_lcd_backlight(0);
    st7789_driver_deinit(true);

    s_wifi_resume_after_light_sleep = wifi_manager_is_enabled();
    wifi_manager_stop();
    peripheral_prepare_light_sleep();
    light_sleep_io_prepare_fast();

    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    gpio_wakeup_enable(GPIO_KEY_OK, GPIO_INTR_HIGH_LEVEL);
    gpio_wakeup_enable(GPIO_TOUCH_INT, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    ESP_LOGI(TAG, "start light sleep");
    esp_light_sleep_start();

    power_sleep_wakeup_light_sleep();
}

static void power_sleep_enter_deep_sleep(void)
{
    esp_err_t ret;

    s_force_deep_sleep = false;
    s_force_sleep = false;

    ESP_LOGI(TAG, "enter deep sleep");

    lvgl_runtime_clear_pause_ack();
    g_is_sleeping = true;
    if (!lvgl_wait_for_pause_ack(600)) {
        ESP_LOGW(TAG, "LVGL pause ACK timeout before deep sleep");
        system_diag_snapshot("deep-sleep-ack-timeout");
    }

    if (video_player_is_playing()) {
        ESP_LOGI(TAG, "stopping video player before deep sleep");
        if (!video_player_stop_wait(3000)) {
            ESP_LOGW(TAG, "video stop wait timeout before deep sleep");
        }
    }

    if (!storage_prepare_for_sleep(2500)) {
        ESP_LOGW(TAG, "storage prepare for sleep timed out");
    }

    st7789_lcd_backlight(0);
    st7789_driver_deinit(true);

    wifi_manager_stop();
    peripheral_prepare_deep_sleep();
    ret = i2c_driver_delete(I2C_NUM_0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "i2c_driver_delete failed: %s", esp_err_to_name(ret));
    }

    SD_card_deinit();

    power_sleep_wait_wake_key_release();
    power_sleep_prepare_ext0_wakeup();
    deep_sleep_io_prepare_full();

    system_diag_snapshot("deep-sleep");
    ESP_LOGI(TAG, "start deep sleep");
    esp_deep_sleep_start();
}

static void power_sleep_task(void *param)
{
    (void)param;

    while (1) {
        uint32_t current_time;
        uint32_t idle_time;
        uint32_t remain;

        vTaskDelay(pdMS_TO_TICKS(POWER_SLEEP_POLL_MS));

        current_time = power_sleep_now_seconds();
        idle_time = current_time - s_last_activity_time;
        remain = (idle_time >= SLEEP_TIMEOUT_SECONDS) ? 0U : (SLEEP_TIMEOUT_SECONDS - idle_time);

        if (!s_force_sleep && !s_force_deep_sleep && (int32_t)remain != s_last_logged_remain) {
            s_last_logged_remain = (int32_t)remain;
            ESP_LOGI(TAG, "sleep countdown: %lu s", (unsigned long)remain);
        }

        if (s_force_deep_sleep) {
            power_sleep_enter_deep_sleep();
        } else if (s_force_sleep || idle_time >= SLEEP_TIMEOUT_SECONDS) {
            power_sleep_enter_light_sleep();
        }
    }
}

void power_sleep_init(void)
{
    if (s_sleep_initialized) {
        return;
    }
    s_sleep_initialized = true;

    power_sleep_boot_init();
    power_sleep_reset_timer();

    xTaskCreate(power_sleep_task, "power_sleep_task", 4096, NULL, 3, NULL);

    ESP_LOGI(TAG, "power sleep ready: idle light sleep + GPIO7 deep sleep");
}
