#include "esp_log.h"
#include "esp_err.h"
#include "st7735s.h"
#include "ui.h"

static const char *TAG = "main";

void app_main(void)
{
    St7735s_cfg_t st7735s_cfg = {
        .host = SPI2_HOST,
        .mosi = 13,
        .miso = -1,
        .sclk = 14,
        .cs = 10,
        .dc = GPIO_NUM_9,
        .rst = GPIO_NUM_NC,
        .blk = GPIO_NUM_0,
        .spi_clock_hz = 10 * 1000 * 1000,
    };
    ESP_ERROR_CHECK(ST7735S_Init(&st7735s_cfg));
    ESP_LOGI(TAG, "ST7735S initialized");
    // ST7735S_FillScreen(0xf1f0);
    ESP_ERROR_CHECK(ui_init());
    ESP_LOGI(TAG, "UI (LVGL demo) started");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
