/**
 * @file ui.c
 * @brief LVGL + ST7735S 显示移植，运行 lv_demo_music 例程
 */

#include "ui.h"

#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lv_conf.h"
#include "st7735s.h"
#include "lvgl.h"

#if LV_USE_DEMO_MUSIC
#include "demos/music/lv_demo_music.h"
#endif

static const char *TAG = "ui";

#define LVGL_HOR_RES    ST7735S_WIDTH
#define LVGL_VER_RES    ST7735S_HEIGHT
#define LVGL_BUF_LINES  20U
#define LVGL_BUF_BYTES  (LVGL_HOR_RES * LVGL_BUF_LINES * 2U)

static lv_display_t *s_disp;
static uint8_t *s_buf1;
static uint8_t *s_buf2;
static esp_timer_handle_t s_tick_timer;
static TaskHandle_t s_lvgl_task;

static void lvgl_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(1);
}

static void disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint32_t w = (uint32_t)lv_area_get_width(area);
    uint32_t h = (uint32_t)lv_area_get_height(area);

    ST7735S_SetAddressWindow((uint16_t)area->x1, (uint16_t)area->y1, (uint16_t)w, (uint16_t)h);
    ST7735S_WritePixels565((uint16_t)w, (uint16_t)h, (const uint16_t *)px_map);
    lv_display_flush_ready(disp);
}

static void lvgl_task(void *arg)
{
    (void)arg;

    while (1) {
        lv_lock();
        uint32_t delay_ms = lv_timer_handler();
        lv_unlock();

        if (delay_ms < 5U) {
            delay_ms = 5U;
        } else if (delay_ms > 500U) {
            delay_ms = 500U;
        }
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

esp_err_t ui_init(void)
{
    s_buf1 = (uint8_t *)heap_caps_malloc(LVGL_BUF_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_buf2 = (uint8_t *)heap_caps_malloc(LVGL_BUF_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!s_buf1 || !s_buf2) {
        ESP_LOGE(TAG, "LVGL buffer alloc failed");
        free(s_buf1);
        free(s_buf2);
        return ESP_ERR_NO_MEM;
    }

    lv_init();

    s_disp = lv_display_create(LVGL_HOR_RES, LVGL_VER_RES);
    if (s_disp == NULL) {
        ESP_LOGE(TAG, "lv_display_create failed");
        return ESP_FAIL;
    }

    lv_display_set_flush_cb(s_disp, disp_flush_cb);
    /* ST7735 SPI 先发高字节；与 LVGL 默认 RGB565 小端序一致需 SWAPPED */
    lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
    lv_display_set_buffers(s_disp, s_buf1, s_buf2, LVGL_BUF_BYTES, LV_DISPLAY_RENDER_MODE_PARTIAL);

    const esp_timer_create_args_t tick_args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick",
    };
    esp_err_t err = esp_timer_create(&tick_args, &s_tick_timer);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_timer_start_periodic(s_tick_timer, 1000);
    if (err != ESP_OK) {
        return err;
    }

    lv_lock();
#if LV_USE_DEMO_MUSIC
    lv_demo_music();
    ESP_LOGI(TAG, "lv_demo_music started");
#else
    ESP_LOGE(TAG, "LV_USE_DEMO_MUSIC disabled: enable in menuconfig or main/lv_conf.h");
#endif
    lv_unlock();

    if (xTaskCreatePinnedToCore(lvgl_task, "lvgl", 16 * 1024, NULL, 4, &s_lvgl_task, 1) != pdPASS) {
        ESP_LOGE(TAG, "lvgl task create failed");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}
