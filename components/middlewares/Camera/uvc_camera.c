/**
 * @file uvc_camera.c
 * @brief ESP32 USB Host UVC 相机封装（基于 espressif/usb_stream，仅视频无 UAC）
 *
 * 流程：分配缓冲 → uvc_streaming_config → 注册状态回调 → 启动并等待设备连接
 * 依赖：main/idf_component.yml 中 espressif/usb_stream；sdkconfig 勿开启 USB_STREAM_QUICK_START
 */

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "usb_stream.h"
#include "uvc_camera.h"


static const char *TAG = "uvc_camera";

/* 1=自动匹配模组支持的分辨率；0=使用下方固定宽高（如 120x160） */
#define UVC_FRAME_RESOLUTION_ANY    (0)

#if UVC_FRAME_RESOLUTION_ANY
#define UVC_FRAME_WIDTH             FRAME_RESOLUTION_ANY
#define UVC_FRAME_HEIGHT            FRAME_RESOLUTION_ANY
#else
#define UVC_FRAME_WIDTH             (640)
#define UVC_FRAME_HEIGHT            (360)
#endif

/* USB 传输双缓冲 + 单帧 MJPEG 缓冲，须大于单帧最大体积 */
#ifdef CONFIG_IDF_TARGET_ESP32S2
#define UVC_XFER_BUFFER_SIZE        (45 * 1024)
#else
#define UVC_XFER_BUFFER_SIZE        (55 * 1024)
#endif

static uint8_t *s_xfer_buffer_a;   /**< Bulk/Iso 传输缓冲 A */
static uint8_t *s_xfer_buffer_b;   /**< Bulk/Iso 传输缓冲 B */
static uint8_t *s_frame_buffer;    /**< 组装后的单帧 MJPEG 拷贝目标 */
static bool s_started;

/**
 * @brief 每收到一帧完整 MJPEG 时由 usb_stream 在独立任务中调用（勿长时间阻塞）
 */
static void camera_frame_cb(uvc_frame_t *frame, void *ptr)
{
    ESP_LOGI(TAG,
             "uvc callback: fmt=%d seq=%" PRIu32 " %ux%u len=%u",
             frame->frame_format,
             frame->sequence,
             frame->width,
             frame->height,
             frame->data_bytes);
    (void)ptr;
}

/**
 * @brief USB 设备插拔状态；连接时可查询模组支持的分辨率列表
 */
static void stream_state_changed_cb(usb_stream_state_t event, void *arg)
{
    (void)arg;

    switch (event) {
    case STREAM_CONNECTED: {
        size_t frame_size = 0;
        size_t frame_index = 0;

        uvc_frame_size_list_get(NULL, &frame_size, &frame_index);
        if (frame_size) {
            ESP_LOGI(TAG, "UVC: frame list size=%u, current=%u", frame_size, frame_index);
            uvc_frame_size_t *list =
                (uvc_frame_size_t *)malloc(frame_size * sizeof(uvc_frame_size_t));
            if (list) {
                uvc_frame_size_list_get(list, NULL, NULL);
                for (size_t i = 0; i < frame_size; i++) {
                    unsigned fps = list[i].interval ? (unsigned)(10000000U / list[i].interval) : 0;
                    ESP_LOGI(TAG, "  frame[%u] = %ux%u interval=%" PRIu32 " (~%u fps)",
                             i, list[i].width, list[i].height, list[i].interval, fps);
                }
                free(list);
            }
        } else {
            ESP_LOGW(TAG, "UVC: empty frame list");
        }
        ESP_LOGI(TAG, "device connected");
        break;
    }
    case STREAM_DISCONNECTED:
        ESP_LOGI(TAG, "device disconnected");
        break;
    default:
        ESP_LOGE(TAG, "unknown stream event %d", event);
        break;
    }
}

esp_err_t uvc_camera_init(void)
{
    esp_err_t ret;

    if (s_started) {
        return ESP_OK;
    }
    s_xfer_buffer_a = (uint8_t *)malloc(UVC_XFER_BUFFER_SIZE);
    s_xfer_buffer_b = (uint8_t *)malloc(UVC_XFER_BUFFER_SIZE);
    s_frame_buffer = (uint8_t *)malloc(UVC_XFER_BUFFER_SIZE);
    if (!s_xfer_buffer_a || !s_xfer_buffer_b || !s_frame_buffer) {
        ESP_LOGE(TAG, "buffer alloc failed");
        uvc_camera_deinit();
        return ESP_ERR_NO_MEM;
    }

    uvc_config_t uvc_config = {
        .frame_width = UVC_FRAME_WIDTH,
        .frame_height = UVC_FRAME_HEIGHT,
        .frame_interval = FPS2INTERVAL(30),       /* 目标约 15fps */
        .xfer_buffer_size = UVC_XFER_BUFFER_SIZE,
        .xfer_buffer_a = s_xfer_buffer_a,
        .xfer_buffer_b = s_xfer_buffer_b,
        .frame_buffer_size = UVC_XFER_BUFFER_SIZE,
        .frame_buffer = s_frame_buffer,
        .frame_cb = camera_frame_cb,
        .frame_cb_arg = NULL,
    };

    ret = uvc_streaming_config(&uvc_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uvc_streaming_config failed");
        uvc_camera_deinit();
        return ret;
    }

    ret = usb_streaming_state_register(stream_state_changed_cb, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "usb_streaming_state_register failed");
        uvc_camera_deinit();
        return ret;
    }

    ret = usb_streaming_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "usb_streaming_start failed");
        uvc_camera_deinit();
        return ret;
    }

    /* 阻塞直到 UVC 设备枚举并开流完成 */
    ret = usb_streaming_connect_wait(portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "usb_streaming_connect_wait failed");
        uvc_camera_deinit();
        return ret;
    }

    /* 部分热成像模组连接后需短暂稳定再持续出帧 */
    vTaskDelay(pdMS_TO_TICKS(500));
    s_started = true;
    ESP_LOGI(TAG, "UVC camera ready");
    return ESP_OK;
}

void uvc_camera_deinit(void)
{
    if (s_started) {
        usb_streaming_stop();
        s_started = false;
    }
    free(s_xfer_buffer_a);
    free(s_xfer_buffer_b);
    free(s_frame_buffer);
    s_xfer_buffer_a = NULL;
    s_xfer_buffer_b = NULL;
    s_frame_buffer = NULL;
}
