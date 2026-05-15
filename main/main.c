#include "esp_log.h"
#include "esp_err.h"

#include "uvc_host_reader.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_ERROR_CHECK(uvc_host_reader_start());
    ESP_LOGI(TAG, "UVC reader started");
}
