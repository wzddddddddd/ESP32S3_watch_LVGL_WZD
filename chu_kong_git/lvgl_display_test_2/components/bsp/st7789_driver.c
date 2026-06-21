#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include "st7789_driver.h"

#define LCD_SPI_HOST    SPI2_HOST

static const char* TAG = "st7789";

//lcd操作句柄
static esp_lcd_panel_io_handle_t lcd_io_handle = NULL;

//刷新完成回调函数
static lcd_flush_done_cb    s_flush_done_cb = NULL;

//背光GPIO
static gpio_num_t   s_bl_gpio = 45;
static bool s_bl_pwm_inited = false;
static uint8_t s_bl_percent = 30;
static bool s_bl_loaded_from_nvs = false;

#define BL_PWM_MODE         LEDC_LOW_SPEED_MODE
#define BL_PWM_TIMER        LEDC_TIMER_0
#define BL_PWM_CHANNEL      LEDC_CHANNEL_0
#define BL_PWM_DUTY_RES     LEDC_TIMER_13_BIT
#define BL_PWM_FREQ_HZ      1000
#define BL_PWM_MAX_DUTY     ((1 << 13) - 1)
#define BL_NVS_NS           "display"
#define BL_NVS_KEY          "bl_pct"

static uint8_t clamp_brightness(uint8_t percent)
{
    if (percent < 10) return 10;
    if (percent > 100) return 100;
    return percent;
}

static void backlight_apply_output(uint8_t percent)
{
    if (percent > 100) percent = 100;

    if (!s_bl_pwm_inited) {
        if (percent == 0) gpio_set_level(s_bl_gpio, 0);
        else gpio_set_level(s_bl_gpio, 1);
        return;
    }

    uint32_t duty = (uint32_t)((BL_PWM_MAX_DUTY * (uint32_t)percent) / 100U);
    ledc_set_duty(BL_PWM_MODE, BL_PWM_CHANNEL, duty);
    ledc_update_duty(BL_PWM_MODE, BL_PWM_CHANNEL);
}

static void backlight_load_from_nvs_once(void)
{
    if (s_bl_loaded_from_nvs) return;
    s_bl_loaded_from_nvs = true;

    nvs_handle_t nvs = 0;
    esp_err_t err = nvs_open(BL_NVS_NS, NVS_READONLY, &nvs);
    if (err != ESP_OK) {
        return;
    }

    uint8_t stored = 0;
    err = nvs_get_u8(nvs, BL_NVS_KEY, &stored);
    nvs_close(nvs);
    if (err == ESP_OK) {
        s_bl_percent = clamp_brightness(stored);
    }
}

static void backlight_save_to_nvs(uint8_t percent)
{
    nvs_handle_t nvs = 0;
    esp_err_t err = nvs_open(BL_NVS_NS, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "brightness nvs open failed: %s", esp_err_to_name(err));
        return;
    }

    err = nvs_set_u8(nvs, BL_NVS_KEY, percent);
    if (err == ESP_OK) {
        err = nvs_commit(nvs);
    }
    nvs_close(nvs);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "brightness nvs save failed: %s", esp_err_to_name(err));
    }
}

static bool notify_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    if(s_flush_done_cb)
        s_flush_done_cb(user_ctx);
    return false;
}


/** st7789初始�?
 * @param st7789_cfg_t  接口参数
 * @return 成功或失�?
*/
esp_err_t st7789_driver_hw_init(st7789_cfg_t* cfg)
{
    //初始化SPI
    spi_bus_config_t buscfg = {
        .sclk_io_num = cfg->clk,        //SCLK引脚
        .mosi_io_num = cfg->mosi,       //MOSI引脚
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .flags = SPICOMMON_BUSFLAG_MASTER , //SPI主模�?
        .max_transfer_sz = cfg->width * cfg->height * sizeof(uint16_t) / 2,  // �?半屏大小
     };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    s_flush_done_cb = cfg->done_cb; //设置刷新完成回调函数

    s_bl_gpio = cfg->bl;    //设置背光GPIO
    backlight_load_from_nvs_once();
    //初始化GPIO(BL)
    gpio_config_t bl_gpio_cfg = 
    {
        .pull_up_en = GPIO_PULLUP_DISABLE,          //禁止上拉
        .pull_down_en = GPIO_PULLDOWN_DISABLE,      //禁止下拉
        .mode = GPIO_MODE_OUTPUT,                   //输出模式
        .intr_type = GPIO_INTR_DISABLE,             //禁止中断
        .pin_bit_mask = (1ULL<<cfg->bl)             //GPIO45背光
    };
    gpio_config(&bl_gpio_cfg);

    ledc_timer_config_t ledc_timer = {
        .speed_mode = BL_PWM_MODE,
        .duty_resolution = BL_PWM_DUTY_RES,
        .timer_num = BL_PWM_TIMER,
        .freq_hz = BL_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_channel_config_t ledc_channel = {
        .gpio_num = cfg->bl,
        .speed_mode = BL_PWM_MODE,
        .channel = BL_PWM_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = BL_PWM_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    if (ledc_timer_config(&ledc_timer) == ESP_OK &&
        ledc_channel_config(&ledc_channel) == ESP_OK) {
        s_bl_pwm_inited = true;
        ESP_LOGI(TAG, "Backlight PWM init ok, gpio=%d, freq=%dHz", (int)cfg->bl, BL_PWM_FREQ_HZ);
        backlight_apply_output(s_bl_percent);
    } else {
        s_bl_pwm_inited = false;
        ESP_LOGW(TAG, "Backlight PWM init failed, fallback digital on");
        backlight_apply_output(s_bl_percent);
    }


    //初始化复位脚
    if(cfg->rst > 0)
    {
        gpio_config_t rst_gpio_cfg = 
        {
            .pull_up_en = GPIO_PULLUP_DISABLE,          //禁止上拉
            .pull_down_en = GPIO_PULLDOWN_DISABLE,      //禁止下拉
            .mode = GPIO_MODE_OUTPUT,                   //输出模式
            .intr_type = GPIO_INTR_DISABLE,             //禁止中断
            .pin_bit_mask = BIT(cfg->rst) //GPIO�?
        };
        gpio_config(&rst_gpio_cfg);
    }

    //创建基于spi的lcd操作句柄
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = cfg->dc,         //DC引脚，低电平表示发送命令，高电平表示传输数�?
        .cs_gpio_num = cfg->cs,         //CS引脚
        .pclk_hz = cfg->spi_fre,        //SPI时钟频率
        .lcd_cmd_bits = 8,              //命令长度
        .lcd_param_bits = 8,            //参数长度
        .spi_mode = 0,                  //使用SPI0模式
        .trans_queue_depth = 20,        //表示可以缓存的spi传输事务队列深度
        .on_color_trans_done = notify_flush_ready,   //刷新完成回调函数
        .user_ctx = cfg->cb_param,                                    //回调函数参数
        .flags = {    // 以下�?SPI 时序的相关参数，需根据 LCD 驱动 IC 的数据手册以及硬件的配置确定
            .sio_mode = 0,    // 通过一根数据线（MOSI）读写数据，0: Interface I 型，1: Interface II �?
        },
    };
    // Attach the LCD to the SPI bus
    ESP_LOGI(TAG,"create esp_lcd_new_panel");
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_config, &lcd_io_handle));
    
    //硬件复位
    if(cfg->rst > 0)
    {
        gpio_set_level(cfg->rst,0);
        vTaskDelay(pdMS_TO_TICKS(20));
        gpio_set_level(cfg->rst,1);
        vTaskDelay(pdMS_TO_TICKS(20));
    }    /* LCD init commands */
    esp_lcd_panel_io_tx_param(lcd_io_handle,LCD_CMD_SWRESET,NULL,0);    //软件复位
    vTaskDelay(pdMS_TO_TICKS(150));
    esp_lcd_panel_io_tx_param(lcd_io_handle,LCD_CMD_SLPOUT,NULL,0);     //退出休眠模�?
    vTaskDelay(pdMS_TO_TICKS(200));
 // 色彩格式 RGB565
    esp_lcd_panel_io_tx_param(lcd_io_handle, LCD_CMD_COLMOD,
                              (uint8_t[]){0x55}, 1);

    // ★★�?显示偏移设置�?40x284面板�?20内存中的偏移�?★★�?
    esp_lcd_panel_io_tx_param(lcd_io_handle, 0xB0,
                              (uint8_t[]){0x00, 0xF0}, 2);

    esp_lcd_panel_io_tx_param(lcd_io_handle,LCD_CMD_INVON,NULL,0);     //颜色翻转
    esp_lcd_panel_io_tx_param(lcd_io_handle,LCD_CMD_NORON,NULL,0);     //普通显示模�?
    uint8_t spin_type = 0;
    switch(cfg->spin)
    {
        case 0:
            spin_type = 0x00;   //不旋�?
            break;
        case 1:
            spin_type = 0x60;   //顺时�?0
            break;
        case 2:
            spin_type = 0xC0;   //180
            break;
        case 3:
            spin_type = 0xA0;   //顺时�?70,（逆时�?0�?
            break;
        default:break;
    }
    esp_lcd_panel_io_tx_param(lcd_io_handle,LCD_CMD_MADCTL,(uint8_t[]) {spin_type,}, 1);   //屏旋转方�?
    vTaskDelay(pdMS_TO_TICKS(150));
    esp_lcd_panel_io_tx_param(lcd_io_handle,LCD_CMD_DISPON,NULL,0);    //打开显示
    vTaskDelay(pdMS_TO_TICKS(300));
    return ESP_OK;
}

/** st7789写入显示数据
 * @param x1,x2,y1,y2:显示区域
 * @return �?
*/
void st7789_flush(int x1,int x2,int y1,int y2,void *data)
{

    // 【新增核心修复】：如果休眠导致句柄为空，直接模拟刷新完成，防止 LVGL 卡死等待
    if (lcd_io_handle == NULL)
    {
        if(s_flush_done_cb) {
            s_flush_done_cb(NULL);
        }
        return;
    }
    
    // define an area of frame memory where MCU can access
    if(x2 <= x1 || y2 <= y1)
    {
        if(s_flush_done_cb)
            s_flush_done_cb(NULL);
        return;
    }

    int y_offset = 36;
    y1 += y_offset;
    y2 += y_offset;

    esp_lcd_panel_io_tx_param(lcd_io_handle, LCD_CMD_CASET, (uint8_t[]) {
        (x1 >> 8) & 0xFF,
        x1 & 0xFF,
        ((x2 - 1) >> 8) & 0xFF,
        (x2 - 1) & 0xFF,
    }, 4);
    esp_lcd_panel_io_tx_param(lcd_io_handle, LCD_CMD_RASET, (uint8_t[]) {
        (y1 >> 8) & 0xFF,
        y1 & 0xFF,
        ((y2 - 1) >> 8) & 0xFF,
        (y2 - 1) & 0xFF,
    }, 4);
    // transfer frame buffer
    size_t len = (x2 - x1) * (y2 - y1) * 2;
    esp_lcd_panel_io_tx_color(lcd_io_handle, LCD_CMD_RAMWR, data, len);
    return ;
}

/** 控制背光
 * @param enable 是否使能背光
 * @return �?
*/
void st7789_lcd_backlight(bool enable)
{
    if(enable)
    {
        backlight_apply_output(s_bl_percent);
    }
    else
    {
        backlight_apply_output(0);
    }
}

void st7789_lcd_set_brightness(uint8_t percent)
{
    percent = clamp_brightness(percent);
    if (percent != s_bl_percent) {
        s_bl_percent = percent;
        backlight_save_to_nvs(s_bl_percent);
    }
    backlight_apply_output(s_bl_percent);
}

uint8_t st7789_lcd_get_brightness(void)
{
    return s_bl_percent;
}

/** st7789反初始化
 * @param sleep_mode true=用于Light Sleep唤醒（释放SPI总线），false=用于重启�?
 */
void st7789_driver_deinit(bool sleep_mode)
{
    // 关闭背光
    if (s_bl_gpio != -1) {
        backlight_apply_output(0);
    }

    if (lcd_io_handle != NULL) {
        // 发送ST7789关显示和休眠命令
        esp_lcd_panel_io_tx_param(lcd_io_handle, 0x28, NULL, 0);
        vTaskDelay(pdMS_TO_TICKS(20));
        esp_lcd_panel_io_tx_param(lcd_io_handle, 0x10, NULL, 0);
        vTaskDelay(pdMS_TO_TICKS(120)); // 必须延时，否则芯片还没睡死你就把总线关了

        if (sleep_mode) {
            
            // 2. 删除 IO 句柄
            esp_lcd_panel_io_del(lcd_io_handle);
            lcd_io_handle = NULL;

            // 3. 释放 SPI 总线
            spi_bus_free(LCD_SPI_HOST);
            
            ESP_LOGI(TAG, "ST7789 deinit done (All resources & SPI bus freed for sleep)");
        } else {
            ESP_LOGI(TAG, "ST7789 display off, waiting for esp_restart");
        }
    }
}
