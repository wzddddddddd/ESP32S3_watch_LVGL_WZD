#ifndef SD3078_H
#define SD3078_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "driver/i2c.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t year;  /* 2000-2099 */
    uint8_t month;  /* 1-12 */
    uint8_t day;    /* 1-31 */
    uint8_t week;   /* 0-6, 0=Sunday */
    uint8_t hour;   /* 0-23 */
    uint8_t minute; /* 0-59 */
    uint8_t second; /* 0-59 */
} sd3078_time_t;

esp_err_t sd3078_init(i2c_port_t i2c_port);
bool sd3078_time_is_valid(const sd3078_time_t *time);

esp_err_t sd3078_get_time(sd3078_time_t *time);
esp_err_t sd3078_set_time(const sd3078_time_t *time);
esp_err_t sd3078_prepare_for_sleep(void);

esp_err_t sd3078_get_unix_time(time_t *unix_time);
esp_err_t sd3078_set_unix_time(time_t unix_time);

#ifdef __cplusplus
}
#endif

#endif /* SD3078_H */
