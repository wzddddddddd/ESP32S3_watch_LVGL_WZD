#include <stdbool.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "esp_log.h"

#include "My_timer.h"
#include "power_sleep.h"

#define TAG "MY_Timer"

#define TIMER_GROUP_ID       TIMER_GROUP_0
#define TIMER_INDEX_ID       TIMER_0
#define TIMER_DIVIDER        80
#define TIMER_INTERVAL_US    1000

#define KEY1_GPIO              GPIO_NUM_5
#define KEY2_GPIO              GPIO_NUM_6
#define KEY3_GPIO              GPIO_NUM_7
#define KEY_TICK_MAX           20
#define KEY_SCAN_PERIOD_MS     KEY_TICK_MAX
#define KEY3_LONG_PRESS_MS     3000U
#define KEY3_LONG_PRESS_COUNT  ((KEY3_LONG_PRESS_MS + KEY_SCAN_PERIOD_MS - 1U) / KEY_SCAN_PERIOD_MS)

static volatile uint8_t Key_Num = 0;

static bool timer_isr_callback(void *arg)
{
    (void)arg;
    Key_Tick();
    return false;
}

esp_err_t my_timer_init(void)
{
    timer_config_t timer_config = {
        .alarm_en = TIMER_ALARM_EN,
        .counter_en = TIMER_PAUSE,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = TIMER_AUTORELOAD_EN,
        .divider = TIMER_DIVIDER,
    };

    esp_err_t ret = timer_init(TIMER_GROUP_ID, TIMER_INDEX_ID, &timer_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "timer init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = timer_set_alarm_value(TIMER_GROUP_ID, TIMER_INDEX_ID, TIMER_INTERVAL_US);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "timer_set_alarm_value failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = timer_isr_callback_add(TIMER_GROUP_ID, TIMER_INDEX_ID, timer_isr_callback, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "timer_isr_callback_add failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = timer_start(TIMER_GROUP_ID, TIMER_INDEX_ID);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "timer_start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "key scan timer init ok");
    return ESP_OK;
}

void Key_Init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(KEY1_GPIO) | BIT64(KEY2_GPIO) | BIT64(KEY3_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "key GPIO init failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "key GPIO init ok (GPIO5/GPIO6/GPIO7)");
    }
}

uint8_t Key_GetNum(void)
{
    uint8_t temp = 0;

    if (Key_Num != 0) {
        temp = Key_Num;
        Key_Num = 0;
    }
    return temp;
}

uint8_t Key_GetState(void)
{
    if (gpio_get_level(KEY1_GPIO) == 1) {
        return 1;
    }
    if (gpio_get_level(KEY2_GPIO) == 1) {
        return 2;
    }
    if (gpio_get_level(KEY3_GPIO) == 1) {
        return 3;
    }
    return 0;
}

void Key_Tick(void)
{
    static uint8_t count = 0;
    static uint8_t curr_state = 0;
    static uint8_t prev_state = 0;
    static uint16_t key3_hold_count = 0;
    static bool key3_long_press_triggered = false;

    count++;
    if (count < KEY_TICK_MAX) {
        return;
    }
    count = 0;

    prev_state = curr_state;
    curr_state = Key_GetState();

    if (power_sleep_wake_key_guard_active()) {
        if (curr_state == 3) {
            key3_hold_count = 0;
            key3_long_press_triggered = false;
            return;
        }

        if (prev_state == 3) {
            power_sleep_clear_wake_key_guard();
            power_sleep_reset_timer();
        } else {
            power_sleep_clear_wake_key_guard();
        }
        key3_hold_count = 0;
        key3_long_press_triggered = false;
        return;
    }

    if (curr_state == 3) {
        if (key3_hold_count < KEY3_LONG_PRESS_COUNT) {
            key3_hold_count++;
        }
        if (!key3_long_press_triggered && key3_hold_count >= KEY3_LONG_PRESS_COUNT) {
            key3_long_press_triggered = true;
            power_sleep_request_deep_sleep();
        }
    } else {
        key3_hold_count = 0;
    }

    if (curr_state == 0 && prev_state != 0) {
        if (prev_state == 3 && key3_long_press_triggered) {
            key3_long_press_triggered = false;
            power_sleep_reset_timer();
            return;
        }

        Key_Num = prev_state;
        key3_long_press_triggered = false;
        power_sleep_reset_timer();
    }
}
