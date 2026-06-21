#ifndef __NTP_TIME_H_
#define __NTP_TIME_H_

#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"
#include "freertos/semphr.h"

void ntp_time_init(void);
void sntp_time_task(void *param);
void sntp_interval_task(void *param);
void trigger_ntp_sync(void);
void ntp_refresh_time_cache(void);

struct tm* get_time(void);

extern SemaphoreHandle_t sntp_trigger_sem;

#endif
