#include "esp_log.h"
#include "esp_err.h"
#include "st7735s.h"
#include "uvc_camera.h"

static const char *TAG = "main";

void app_main(void)
{
    // ESP_ERROR_CHECK(uvc_camera_init());
    // ESP_LOGI(TAG, "UVC reader started");

    St7735s_cfg_t st7735s_cfg = {
        .host = SPI2_HOST,
        .mosi = 13,
        .miso = -1,
        .sclk = 14,
        .cs = 10,
        .dc = GPIO_NUM_9,
        .rst = GPIO_NUM_NC,   /* 有 RST 脚时改为实际 GPIO */
        .blk = GPIO_NUM_0,   /* 有背光脚时改为实际 GPIO */
        .spi_clock_hz = 10 * 1000 * 1000,
    };
    ESP_ERROR_CHECK(ST7735S_Init(&st7735s_cfg));

    ESP_LOGI(TAG, "ST7735S initialized");
    ST7735S_FillScreen(0xf010);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
