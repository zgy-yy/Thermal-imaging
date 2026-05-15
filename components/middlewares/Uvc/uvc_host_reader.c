/*
 * USB Host UVC - based on ESP-IDF examples/peripherals/usb/host/uvc
 */

#include <assert.h>
#include <stdbool.h>

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_heap_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "usb/usb_host.h"
#include "usb/uvc_host.h"

#include "uvc_host_reader.h"

#define USB_HOST_TASK_PRIO     (15)
#define RECORDING_LENGTH_S     (5)

static const char *TAG = "uvc_host_reader";

static bool frame_callback(const uvc_host_frame_t *frame, void *user_ctx);
static void stream_callback(const uvc_host_stream_event_data_t *event, void *user_ctx);

static QueueHandle_t s_frame_q;
static bool s_dev_connected;

static const uvc_host_stream_config_t s_stream_cfg = {
    .event_cb = stream_callback,
    .frame_cb = frame_callback,
    .user_ctx = &s_frame_q,
    .usb = {
        .vid = UVC_HOST_ANY_VID,
        .pid = UVC_HOST_ANY_PID,
        .uvc_stream_index = 0,
    },
    .vs_format = {
#if CONFIG_SPIRAM
        .h_res = 640,
        .v_res = 480,
#else
        .h_res = 320,
        .v_res = 240,
#endif
        .fps = 15,
        .format = UVC_VS_FORMAT_MJPEG,
    },
    .advanced = {
        .frame_size = 0,
#if CONFIG_SPIRAM
        .number_of_frame_buffers = 3,
        .number_of_urbs = 3,
        .urb_size = 10 * 1024,
        .frame_heap_caps = MALLOC_CAP_SPIRAM,
#else
        .number_of_frame_buffers = 2,
        .number_of_urbs = 2,
        .urb_size = 2 * 1024,
#endif
    },
};

static bool frame_callback(const uvc_host_frame_t *frame, void *user_ctx)
{
    assert(frame && user_ctx);
    QueueHandle_t q = *(QueueHandle_t *)user_ctx;
    if (xQueueSendToBack(q, &frame, 0) != pdPASS) {
        ESP_LOGW(TAG, "frame queue full");
        return true;
    }
    return false;
}

static void stream_callback(const uvc_host_stream_event_data_t *event, void *user_ctx)
{
    (void)user_ctx;
    switch (event->type) {
    case UVC_HOST_TRANSFER_ERROR:
        ESP_LOGE(TAG, "USB err %d", event->transfer_error.error);
        break;
    case UVC_HOST_DEVICE_DISCONNECTED:
        ESP_LOGI(TAG, "UVC disconnected");
        s_dev_connected = false;
        ESP_ERROR_CHECK(uvc_host_stream_close(event->device_disconnected.stream_hdl));
        break;
    case UVC_HOST_FRAME_BUFFER_OVERFLOW:
        ESP_LOGW(TAG, "frame overflow");
        break;
    case UVC_HOST_FRAME_BUFFER_UNDERFLOW:
        ESP_LOGW(TAG, "frame underflow");
        break;
    default:
        ESP_LOGW(TAG, "stream event %d", event->type);
        break;
    }
}

static void usb_lib_task(void *arg)
{
    (void)arg;
    for (;;) {
        uint32_t flags;
        usb_host_lib_handle_events(portMAX_DELAY, &flags);
        if (flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            usb_host_device_free_all();
        }
    }
}

static void frame_handling_task(void *arg)
{
    const uvc_host_stream_config_t *cfg = (const uvc_host_stream_config_t *)arg;
    QueueHandle_t q = *(QueueHandle_t *)cfg->user_ctx;

    for (;;) {
        uvc_host_stream_hdl_t stream = NULL;
        ESP_LOGI(TAG, "open %dx%d@%.1f MJPEG",
                 cfg->vs_format.h_res, cfg->vs_format.v_res, cfg->vs_format.fps);

        esp_err_t err = uvc_host_stream_open(cfg, pdMS_TO_TICKS(5000), &stream);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "open failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        s_dev_connected = true;
        ESP_LOGI(TAG, "UVC opened");
        vTaskDelay(pdMS_TO_TICKS(100));

        unsigned iter = 0;
        while (s_dev_connected) {
            ESP_LOGI(TAG, "stream start iter %u", iter++);
            uvc_host_stream_start(stream);

            TickType_t window = pdMS_TO_TICKS(RECORDING_LENGTH_S * 1000);
            TimeOut_t tmo;
            vTaskSetTimeOutState(&tmo);

            do {
                uvc_host_frame_t *frame = NULL;
                if (xQueueReceive(q, &frame, pdMS_TO_TICKS(5000)) != pdPASS) {
                    ESP_LOGW(TAG, "no frame 5s");
                    break;
                }
                ESP_LOGI(TAG, "frame len %u", (unsigned)frame->data_len);
                uvc_host_frame_return(stream, frame);
            } while (xTaskCheckForTimeOut(&tmo, &window) == pdFALSE);

            if (s_dev_connected) {
                ESP_LOGI(TAG, "stream stop");
                uvc_host_stream_stop(stream);
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
        }
    }
}

esp_err_t uvc_host_reader_start(void)
{
    s_frame_q = xQueueCreate(3, sizeof(uvc_host_frame_t *));
    if (!s_frame_q) {
        return ESP_ERR_NO_MEM;
    }

    const usb_host_config_t host_cfg = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LOWMED,
    };
    esp_err_t err = usb_host_install(&host_cfg);
    if (err != ESP_OK) {
        return err;
    }

    BaseType_t ok = xTaskCreatePinnedToCore(usb_lib_task, "usb_lib", 4096, NULL, USB_HOST_TASK_PRIO, NULL, tskNO_AFFINITY);
    if (ok != pdTRUE) {
        return ESP_FAIL;
    }

    const uvc_host_driver_config_t uvc_cfg = {
        .driver_task_stack_size = 4 * 1024,
        .driver_task_priority = USB_HOST_TASK_PRIO + 1,
        .xCoreID = tskNO_AFFINITY,
        .create_background_task = true,
    };
    err = uvc_host_install(&uvc_cfg);
    if (err != ESP_OK) {
        return err;
    }

    ok = xTaskCreatePinnedToCore(frame_handling_task, "frame_hdl", 8192, (void *)&s_stream_cfg, USB_HOST_TASK_PRIO - 2, NULL, tskNO_AFFINITY);
    if (ok != pdTRUE) {
        return ESP_FAIL;
    }

    return ESP_OK;
}
