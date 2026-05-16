#ifndef __ST7735S_H__
#define __ST7735S_H__

#include <stdint.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ST7735S_WIDTH   128U
#define ST7735S_HEIGHT  160U

/** 接线：SPI MOSI/SCLK/CS 由 spi_comm 管理；DC/RST/BLK 为 GPIO */
typedef struct {
    spi_host_device_t host;
    int mosi;
    int miso;       /* 无 MISO 填 -1 */
    int sclk;
    int cs;
    gpio_num_t dc;
    gpio_num_t rst;
    gpio_num_t blk; /* 不用背光控制填 GPIO_NUM_NC */
    uint32_t spi_clock_hz; /* 典型 10M~20M，ST7735 可先用 10MHz */
} St7735s_cfg_t;

/** 初始化 GPIO + SPI（调用 spi_comm_init）并下发 ST7735 寄存器序列 */
esp_err_t ST7735S_Init(const St7735s_cfg_t *cfg);

void ST7735S_SetAddressWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void ST7735S_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ST7735S_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7735S_FillScreen(uint16_t color);
/** 先调用 ST7735S_SetAddressWindow，再写 RGB565 块（内部按 4KB 分包 SPI） */
void ST7735S_WritePixels565(uint16_t w, uint16_t h, const uint16_t *rgb565);

#ifdef __cplusplus
}
#endif

#endif