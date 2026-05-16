/**
 * @file lv_conf.h
 * @brief LVGL 配置（ST7735 128x160 RGB565，lv_demo_music）
 */
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH              16
#define LV_DRAW_SW_SUPPORT_RGB565_SWAPPED  1
#define LV_MEM_SIZE                 (64U * 1024U)

#define LV_USE_OS                   LV_OS_FREERTOS

#define LV_FONT_MONTSERRAT_14       1
#define LV_FONT_DEFAULT             &lv_font_montserrat_14

#define LV_BUILD_DEMOS              1
#define LV_USE_DEMO_MUSIC           1

#endif /* LV_CONF_H */
