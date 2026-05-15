#include "esp_log.h"
#include "esp_err.h"
#include "st7735s.h"
#include "uvc_host_reader.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_ERROR_CHECK(uvc_host_reader_start());
    ESP_LOGI(TAG, "UVC reader started");
    const st7735s_cfg_t lcd = {
        .host = SPI2_HOST,
        .mosi = 11, .miso = -1, .sclk = 12, .cs = 10,
        .dc = GPIO_NUM_13, .rst = GPIO_NUM_14, .blk = GPIO_NUM_15,
        .spi_clock_hz = 10 * 1000 * 1000,
    };
    ESP_ERROR_CHECK(ST7735S_Init(&lcd));
    ST7735S_FillScreen(0xF800);  /* 红 */
}
