#ifndef I2C_BUS_LOCK_H
#define I2C_BUS_LOCK_H

#include <stdbool.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t bsp_i2c_bus_lock_init(void);
bool bsp_i2c_bus_lock_take(TickType_t timeout_ticks);
void bsp_i2c_bus_lock_give(void);

#ifdef __cplusplus
}
#endif

#endif /* I2C_BUS_LOCK_H */

