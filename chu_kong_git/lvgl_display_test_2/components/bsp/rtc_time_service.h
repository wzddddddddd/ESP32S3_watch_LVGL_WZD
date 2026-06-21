#ifndef RTC_TIME_SERVICE_H
#define RTC_TIME_SERVICE_H

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t rtc_time_service_init_shared_i2c(void);
bool rtc_time_service_is_ready(void);

esp_err_t rtc_time_service_boot_sync_system_from_rtc(void);
esp_err_t rtc_time_service_sync_rtc_from_system(void);
esp_err_t rtc_time_service_prepare_for_sleep(void);
esp_err_t rtc_time_service_set_manual_and_sync(int year, int month, int day,
                                               int hour, int minute, int second);
esp_err_t rtc_time_service_hourly_sync_system_from_rtc(void);

#ifdef __cplusplus
}
#endif

#endif /* RTC_TIME_SERVICE_H */
