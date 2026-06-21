#include "cst816t_driver.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "power_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus_lock.h"

#define TOUCH_I2C_PORT      I2C_NUM_0
#define TOUCH_INT_GPIO      GPIO_NUM_16

#define CST816T_ADDR        0x15
#define CST816T_REG_SLEEP   0xA5
#define CST816T_SLEEP_VALUE 0x03

static const char *TAG = "cst816t";

static uint16_t s_usLimitX = 0;
static uint16_t s_usLimitY = 0;

static esp_err_t i2c_read(uint8_t slave_addr, uint8_t register_addr, uint8_t read_len, uint8_t *data_buf);
static esp_err_t i2c_read_batch(uint8_t slave_addr, uint8_t start_reg, uint8_t len, uint8_t *buf);
static esp_err_t i2c_write_byte(uint8_t slave_addr, uint8_t register_addr, uint8_t value);
static void touch_int_handler(void *arg);

static bool s_isr_service_installed = false;

esp_err_t cst816t_init(cst816t_cfg_t *cfg)
{
    int i2c_master_port = TOUCH_I2C_PORT;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = cfg->sda,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = cfg->scl,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = cfg->fre,
    };

    s_usLimitX = cfg->x_limit;
    s_usLimitY = cfg->y_limit;

    esp_err_t ret = bsp_i2c_bus_lock_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c lock init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    if (!bsp_i2c_bus_lock_take(pdMS_TO_TICKS(200))) {
        return ESP_ERR_TIMEOUT;
    }

    ret = i2c_param_config(i2c_master_port, &conf);
    if (ret == ESP_OK) {
        ret = i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
        if (ret == ESP_ERR_INVALID_STATE) {
            ret = ESP_OK;
        }
    }

    bsp_i2c_bus_lock_give();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(TOUCH_INT_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&io_conf);

    if (!s_isr_service_installed) {
        gpio_install_isr_service(0);
        s_isr_service_installed = true;
    }
    gpio_isr_handler_remove(TOUCH_INT_GPIO);
    gpio_isr_handler_add(TOUCH_INT_GPIO, touch_int_handler, NULL);

    uint8_t data_buf = 0;
    i2c_read(CST816T_ADDR, 0xA7, 1, &data_buf);
    ESP_LOGI(TAG, "\tChip ID: 0x%02x", data_buf);

    i2c_read(CST816T_ADDR, 0xA9, 1, &data_buf);
    ESP_LOGI(TAG, "\tFirmware version: 0x%02x", data_buf);

    i2c_read(CST816T_ADDR, 0xAA, 1, &data_buf);
    ESP_LOGI(TAG, "\tFactory ID: 0x%02x", data_buf);

    return ESP_OK;
}

void cst816t_read(int16_t *x, int16_t *y, int *state)
{
    static int16_t last_x = 0;
    static int16_t last_y = 0;
    static TickType_t last_pr_tick = 0;

    uint8_t buf[5] = {0};
    esp_err_t ret = i2c_read_batch(CST816T_ADDR, 0x02, 5, buf);

    if (ret != ESP_OK || buf[0] != 1) {
        TickType_t now = xTaskGetTickCount();
        if (last_pr_tick != 0 && (now - last_pr_tick) <= pdMS_TO_TICKS(35)) {
            *x = last_x;
            *y = last_y;
            *state = 1;
            return;
        }

        *x = last_x;
        *y = last_y;
        *state = 0;
        return;
    }

    int16_t current_x = ((buf[1] & 0x0F) << 8) | buf[2];
    int16_t current_y = ((buf[3] & 0x0F) << 8) | buf[4];

    if (current_x >= s_usLimitX) current_x = s_usLimitX - 1;
    if (current_y >= s_usLimitY) current_y = s_usLimitY - 1;
    if (current_x < 0) current_x = 0;
    if (current_y < 0) current_y = 0;

    last_x = current_x;
    last_y = current_y;
    last_pr_tick = xTaskGetTickCount();

    *x = last_x;
    *y = last_y;
    *state = 1;
}

static esp_err_t i2c_read_batch(uint8_t slave_addr, uint8_t start_reg, uint8_t len, uint8_t *buf)
{
    if (!bsp_i2c_bus_lock_take(pdMS_TO_TICKS(80))) {
        return ESP_ERR_TIMEOUT;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) {
        bsp_i2c_bus_lock_give();
        return ESP_FAIL;
    }

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, start_reg, I2C_MASTER_ACK);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, buf, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, &buf[len - 1], I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(TOUCH_I2C_PORT, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
    bsp_i2c_bus_lock_give();
    return ret;
}

static esp_err_t i2c_read(uint8_t slave_addr, uint8_t register_addr,
                           uint8_t read_len, uint8_t *data_buf)
{
    return i2c_read_batch(slave_addr, register_addr, read_len, data_buf);
}

static esp_err_t i2c_write_byte(uint8_t slave_addr, uint8_t register_addr, uint8_t value)
{
    if (!bsp_i2c_bus_lock_take(pdMS_TO_TICKS(80))) {
        return ESP_ERR_TIMEOUT;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) {
        bsp_i2c_bus_lock_give();
        return ESP_FAIL;
    }

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, register_addr, true);
    i2c_master_write_byte(cmd, value, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(TOUCH_I2C_PORT, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
    bsp_i2c_bus_lock_give();
    return ret;
}

static void touch_int_handler(void *arg)
{
    (void)arg;

    static uint32_t last_reset_tick = 0;
    uint32_t now = xTaskGetTickCount();

    if (now - last_reset_tick > pdMS_TO_TICKS(500)) {
        last_reset_tick = now;
        power_sleep_reset_timer();
    }
}

void cst816t_prepare_for_sleep(void)
{
    gpio_intr_disable(TOUCH_INT_GPIO);
    if (s_isr_service_installed) {
        gpio_isr_handler_remove(TOUCH_INT_GPIO);
    }
}

void cst816t_prepare_for_deep_sleep(void)
{
    cst816t_prepare_for_sleep();

    esp_err_t ret = i2c_write_byte(CST816T_ADDR, CST816T_REG_SLEEP, CST816T_SLEEP_VALUE);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "set deep sleep mode failed: %s", esp_err_to_name(ret));
    }
}

void cst816t_reinit(void)
{
    if (!bsp_i2c_bus_lock_take(pdMS_TO_TICKS(200))) {
        ESP_LOGW(TAG, "reinit lock timeout");
        return;
    }

    i2c_driver_delete(TOUCH_I2C_PORT);

    cst816t_cfg_t cst816t_config = {
        .scl = GPIO_NUM_18,
        .sda = GPIO_NUM_17,
        .fre = 400 * 1000,
        .x_limit = 240,
        .y_limit = 284,
    };

    esp_err_t ret = cst816t_init(&cst816t_config);
    bsp_i2c_bus_lock_give();

    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "reinit failed: %s", esp_err_to_name(ret));
        return;
    }

    gpio_intr_enable(TOUCH_INT_GPIO);
}
