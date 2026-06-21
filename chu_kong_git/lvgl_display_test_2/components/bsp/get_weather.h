#ifndef __GET_WEATHER_H_
#define __GET_WEATHER_H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define FORECAST_DAY 3

// 天气字符最大为"雷阵雨伴有冰雹" 共计21, 某尾加\0 -> 22
typedef struct weather {
    char    text_day[FORECAST_DAY][22];
    char    text_night[FORECAST_DAY][22];
    uint8_t code_day[FORECAST_DAY];
    uint8_t code_night[FORECAST_DAY];
    int8_t  degree_high[FORECAST_DAY];
    int8_t  degree_low[FORECAST_DAY];
    uint8_t humidity[FORECAST_DAY];
    char    city[22];
} weather_t;

typedef enum {
    WEATHER_OK = 0,
    WEATHER_SYNCING,
    WEATHER_NO_WIFI,
    WEATHER_HTTP_FAIL,
    WEATHER_PARSE_FAIL,
} weather_status_t;

typedef struct {
    bool             valid;
    char             city[22];
    char             condition[22];
    int8_t           high;
    int8_t           low;
    uint8_t          humidity;
    time_t           last_update;
    weather_status_t status;
} weather_snapshot_t;

void weather_service_init(void);
void weather_request_sync(void);
bool weather_get_snapshot(weather_snapshot_t *out);

#endif

