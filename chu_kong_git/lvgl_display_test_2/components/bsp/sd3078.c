#include "sd3078.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "i2c_bus_lock.h"

#define SD3078_TAG "sd3078"

#define SD3078_I2C_ADDR            0x32 /* 7-bit address */
#define SD3078_I2C_TIMEOUT_MS      100

#define SD3078_REG_SEC             0x00
#define SD3078_REG_MIN             0x01
#define SD3078_REG_HOUR            0x02
#define SD3078_REG_WEEK            0x03
#define SD3078_REG_DAY             0x04
#define SD3078_REG_MONTH           0x05
#define SD3078_REG_YEAR            0x06

#define SD3078_REG_CTR1            0x0F
#define SD3078_REG_CTR2            0x10

#define SD3078_BIT_WRTC3           (1U << 7) /* CTR1 bit7 */
#define SD3078_BIT_WRTC2           (1U << 2) /* CTR1 bit2 */
#define SD3078_BIT_WRTC1           (1U << 7) /* CTR2 bit7 */
#define SD3078_SLEEP_DISABLE_OUTPUT_MASK 0x0FU

static i2c_port_t s_i2c_port = I2C_NUM_0;
static bool s_inited = false;

static bool is_leap_year(uint16_t year)
{
    return ((year % 4U) == 0U) && (((year % 100U) != 0U) || ((year % 400U) == 0U));
}

static uint8_t days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t k_days[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    if (month < 1U || month > 12U) {
        return 0U;
    }

    uint8_t days = k_days[month - 1U];
    if (month == 2U && is_leap_year(year)) {
        days = 29U;
    }
    return days;
}

static uint8_t bin_to_bcd(uint8_t value)
{
    return (uint8_t)(((value / 10U) << 4) | (value % 10U));
}

static uint8_t bcd_to_bin(uint8_t value)
{
    return (uint8_t)(((value >> 4) * 10U) + (value & 0x0FU));
}

static esp_err_t sd3078_i2c_write(uint8_t reg, const uint8_t *data, size_t len)
{
    if (!bsp_i2c_bus_lock_take(pdMS_TO_TICKS(SD3078_I2C_TIMEOUT_MS))) {
        return ESP_ERR_TIMEOUT;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        bsp_i2c_bus_lock_give();
        return ESP_ERR_NO_MEM;
    }

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SD3078_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    if (len > 0U && data != NULL) {
        i2c_master_write(cmd, (uint8_t *)data, len, true);
    }
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(SD3078_I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    bsp_i2c_bus_lock_give();
    return ret;
}

static esp_err_t sd3078_i2c_read(uint8_t reg, uint8_t *data, size_t len)
{
    if (data == NULL || len == 0U) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!bsp_i2c_bus_lock_take(pdMS_TO_TICKS(SD3078_I2C_TIMEOUT_MS))) {
        return ESP_ERR_TIMEOUT;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        bsp_i2c_bus_lock_give();
        return ESP_ERR_NO_MEM;
    }

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SD3078_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SD3078_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    if (len > 1U) {
        i2c_master_read(cmd, data, len - 1U, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, &data[len - 1U], I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(SD3078_I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    bsp_i2c_bus_lock_give();
    return ret;
}

static esp_err_t sd3078_write_enable(void)
{
    uint8_t ctr2 = 0U;
    uint8_t ctr1 = 0U;

    esp_err_t ret = sd3078_i2c_read(SD3078_REG_CTR2, &ctr2, 1U);
    if (ret != ESP_OK) {
        return ret;
    }
    ctr2 |= SD3078_BIT_WRTC1;
    ret = sd3078_i2c_write(SD3078_REG_CTR2, &ctr2, 1U);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = sd3078_i2c_read(SD3078_REG_CTR1, &ctr1, 1U);
    if (ret != ESP_OK) {
        return ret;
    }
    ctr1 |= (SD3078_BIT_WRTC2 | SD3078_BIT_WRTC3);
    return sd3078_i2c_write(SD3078_REG_CTR1, &ctr1, 1U);
}

static esp_err_t sd3078_write_disable(void)
{
    uint8_t ctr1 = 0U;
    uint8_t ctr2 = 0U;

    esp_err_t ret = sd3078_i2c_read(SD3078_REG_CTR1, &ctr1, 1U);
    if (ret != ESP_OK) {
        return ret;
    }
    ctr1 &= (uint8_t)~(SD3078_BIT_WRTC2 | SD3078_BIT_WRTC3);
    ret = sd3078_i2c_write(SD3078_REG_CTR1, &ctr1, 1U);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = sd3078_i2c_read(SD3078_REG_CTR2, &ctr2, 1U);
    if (ret != ESP_OK) {
        return ret;
    }
    ctr2 &= (uint8_t)~SD3078_BIT_WRTC1;
    return sd3078_i2c_write(SD3078_REG_CTR2, &ctr2, 1U);
}

bool sd3078_time_is_valid(const sd3078_time_t *time)
{
    if (time == NULL) {
        return false;
    }
    if (time->year < 2000U || time->year > 2099U) return false;
    if (time->month < 1U || time->month > 12U) return false;
    uint8_t max_day = days_in_month(time->year, time->month);
    if (time->day < 1U || time->day > max_day) return false;
    if (time->week > 6U) return false;
    if (time->hour > 23U) return false;
    if (time->minute > 59U) return false;
    if (time->second > 59U) return false;
    return true;
}

esp_err_t sd3078_init(i2c_port_t i2c_port)
{
    esp_err_t ret = bsp_i2c_bus_lock_init();
    if (ret != ESP_OK) {
        return ret;
    }

    s_i2c_port = i2c_port;
    uint8_t sec = 0U;
    ret = sd3078_i2c_read(SD3078_REG_SEC, &sec, 1U);
    if (ret == ESP_OK) {
        s_inited = true;
        ESP_LOGI(SD3078_TAG, "SD3078 online, sec reg=0x%02X", sec);
    }
    return ret;
}

esp_err_t sd3078_get_time(sd3078_time_t *time)
{
    if (!s_inited) {
        return ESP_ERR_INVALID_STATE;
    }
    if (time == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buf[7] = {0};
    esp_err_t ret = sd3078_i2c_read(SD3078_REG_SEC, buf, sizeof(buf));
    if (ret != ESP_OK) {
        return ret;
    }

    time->second = bcd_to_bin(buf[0] & 0x7FU);
    time->minute = bcd_to_bin(buf[1] & 0x7FU);
    time->hour = bcd_to_bin(buf[2] & 0x3FU);
    time->week = bcd_to_bin(buf[3] & 0x07U);
    time->day = bcd_to_bin(buf[4] & 0x3FU);
    time->month = bcd_to_bin(buf[5] & 0x1FU);
    time->year = (uint16_t)(2000U + bcd_to_bin(buf[6]));
    return ESP_OK;
}

esp_err_t sd3078_set_time(const sd3078_time_t *time)
{
    if (!s_inited) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!sd3078_time_is_valid(time)) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buf[7] = {0};
    buf[0] = bin_to_bcd(time->second);
    buf[1] = bin_to_bcd(time->minute);
    buf[2] = (uint8_t)(bin_to_bcd(time->hour) | 0x80U); /* 24h mode */
    buf[3] = (uint8_t)(bin_to_bcd(time->week) & 0x07U);
    buf[4] = bin_to_bcd(time->day);
    buf[5] = bin_to_bcd(time->month);
    buf[6] = bin_to_bcd((uint8_t)(time->year % 100U));

    esp_err_t ret = sd3078_write_enable();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = sd3078_i2c_write(SD3078_REG_SEC, buf, sizeof(buf));
    esp_err_t ret_dis = sd3078_write_disable();
    if (ret == ESP_OK) {
        ret = ret_dis;
    }
    return ret;
}

esp_err_t sd3078_prepare_for_sleep(void)
{
    if (!s_inited) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t ctr2 = 0U;
    esp_err_t ret = sd3078_i2c_read(SD3078_REG_CTR2, &ctr2, 1U);
    if (ret != ESP_OK) {
        return ret;
    }

    uint8_t ctr2_new = (uint8_t)(ctr2 & (uint8_t)~SD3078_SLEEP_DISABLE_OUTPUT_MASK);
    if (ctr2_new == ctr2) {
        return ESP_OK;
    }

    ret = sd3078_write_enable();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = sd3078_i2c_write(SD3078_REG_CTR2, &ctr2_new, 1U);
    esp_err_t ret_dis = sd3078_write_disable();
    if (ret == ESP_OK) {
        ret = ret_dis;
    }

    if (ret == ESP_OK) {
        ESP_LOGI(SD3078_TAG, "RTC low-power prep done: CTR2 0x%02X -> 0x%02X", ctr2, ctr2_new);
    }
    return ret;
}

esp_err_t sd3078_get_unix_time(time_t *unix_time)
{
    if (unix_time == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    sd3078_time_t rtc_time = {0};
    esp_err_t ret = sd3078_get_time(&rtc_time);
    if (ret != ESP_OK) {
        return ret;
    }

    struct tm tm_value = {0};
    tm_value.tm_year = (int)rtc_time.year - 1900;
    tm_value.tm_mon = (int)rtc_time.month - 1;
    tm_value.tm_mday = (int)rtc_time.day;
    tm_value.tm_wday = (int)rtc_time.week;
    tm_value.tm_hour = (int)rtc_time.hour;
    tm_value.tm_min = (int)rtc_time.minute;
    tm_value.tm_sec = (int)rtc_time.second;
    tm_value.tm_isdst = -1;

    time_t ts = mktime(&tm_value);
    if (ts == (time_t)-1) {
        return ESP_FAIL;
    }
    *unix_time = ts;
    return ESP_OK;
}

esp_err_t sd3078_set_unix_time(time_t unix_time)
{
    struct tm tm_value = {0};
    if (localtime_r(&unix_time, &tm_value) == NULL) {
        return ESP_FAIL;
    }

    sd3078_time_t rtc_time = {
        .year = (uint16_t)(tm_value.tm_year + 1900),
        .month = (uint8_t)(tm_value.tm_mon + 1),
        .day = (uint8_t)tm_value.tm_mday,
        .week = (uint8_t)tm_value.tm_wday,
        .hour = (uint8_t)tm_value.tm_hour,
        .minute = (uint8_t)tm_value.tm_min,
        .second = (uint8_t)tm_value.tm_sec,
    };
    return sd3078_set_time(&rtc_time);
}
