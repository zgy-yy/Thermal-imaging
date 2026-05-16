/**
 * @file uvc_camera.h
 * @brief USB UVC 热成像/相机 Host 侧接口
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 UVC Host
 *
 * 分配传输缓冲、配置 MJPEG 流、启动 USB Host 并阻塞等待设备连接。
 * 成功后 `camera_frame_cb` 会按帧率收到完整 MJPEG 数据（当前实现仅打日志）。
 *
 * @return ESP_OK 成功；ESP_ERR_NO_MEM / ESP_FAIL 等见日志
 * @note 须在 menuconfig 中关闭 USB_STREAM_QUICK_START，并启用 UVC_GET_CONFIG_DESC
 */
esp_err_t uvc_camera_init(void);

/**
 * @brief 停止 USB 流并释放 init 中申请的缓冲
 */
void uvc_camera_deinit(void);

#ifdef __cplusplus
}
#endif
