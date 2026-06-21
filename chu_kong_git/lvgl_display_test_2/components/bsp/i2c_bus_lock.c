#include "i2c_bus_lock.h"

#include "freertos/semphr.h"

static SemaphoreHandle_t s_i2c_bus_mutex = NULL;

esp_err_t bsp_i2c_bus_lock_init(void)
{
    if (s_i2c_bus_mutex != NULL) {
        return ESP_OK;
    }

    s_i2c_bus_mutex = xSemaphoreCreateRecursiveMutex();
    if (s_i2c_bus_mutex == NULL) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

bool bsp_i2c_bus_lock_take(TickType_t timeout_ticks)
{
    if (s_i2c_bus_mutex == NULL) {
        if (bsp_i2c_bus_lock_init() != ESP_OK) {
            return false;
        }
    }
    return (xSemaphoreTakeRecursive(s_i2c_bus_mutex, timeout_ticks) == pdTRUE);
}

void bsp_i2c_bus_lock_give(void)
{
    if (s_i2c_bus_mutex == NULL) {
        return;
    }
    xSemaphoreGiveRecursive(s_i2c_bus_mutex);
}

