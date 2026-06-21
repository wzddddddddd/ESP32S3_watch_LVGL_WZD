#include "peripheral_sleep.h"

#include "esp_err.h"
#include "esp_log.h"

#include "SD_card.h"
#include "cst816t_driver.h"
#include "rtc_time_service.h"

static const char *TAG = "periph_sleep";

static void log_prepare_result(const char *name, esp_err_t ret)
{
    if (ret == ESP_OK || ret == ESP_ERR_INVALID_STATE) {
        return;
    }
    ESP_LOGW(TAG, "%s prepare failed: %s", name, esp_err_to_name(ret));
}

void peripheral_prepare_light_sleep(void)
{
    esp_err_t rtc_ret = rtc_time_service_prepare_for_sleep();
    log_prepare_result("rtc", rtc_ret);
}

void peripheral_prepare_deep_sleep(void)
{
    esp_err_t rtc_ret = rtc_time_service_prepare_for_sleep();
    log_prepare_result("rtc", rtc_ret);

    cst816t_prepare_for_deep_sleep();
    SD_card_prepare_for_sleep();
}
