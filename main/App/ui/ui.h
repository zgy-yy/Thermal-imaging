#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 LVGL（显示移植 + 定时器 + lv_demo_music 例程）
 * @note 调用前须已完成 ST7735S_Init()
 */
esp_err_t ui_init(void);

#ifdef __cplusplus
}
#endif
