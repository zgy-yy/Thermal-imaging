#include "st7735s.h"
#include "spi_comm.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static uint16_t s_width = ST7735S_WIDTH;
static uint16_t s_height = ST7735S_HEIGHT;
static uint8_t s_x_offset = 2U;
static uint8_t s_y_offset = 1U;

static gpio_num_t s_dc = GPIO_NUM_NC;
static gpio_num_t s_rst = GPIO_NUM_NC;
static gpio_num_t s_blk = GPIO_NUM_NC;
static bool s_ready;

#define ST7735S_SLPOUT  0x11U
#define ST7735S_DISPON  0x29U
#define ST7735S_CASET   0x2AU
#define ST7735S_RASET   0x2BU
#define ST7735S_RAMWR   0x2CU

static inline void delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms == 0U ? 1U : ms));
}

static inline esp_err_t gpio_pin_out(gpio_num_t pin, int level)
{
    if (pin == GPIO_NUM_NC) {
        return ESP_OK;
    }
    return gpio_set_level(pin, level);
}

static inline void ST7735S_DC_Low(void)  { (void)gpio_pin_out(s_dc, 0); }
static inline void ST7735S_DC_High(void) { (void)gpio_pin_out(s_dc, 1); }
static inline void ST7735S_RST_Low(void)  { (void)gpio_pin_out(s_rst, 0); }
static inline void ST7735S_RST_High(void) { (void)gpio_pin_out(s_rst, 1); }
static inline void ST7735S_BLK_Low(void)  { (void)gpio_pin_out(s_blk, 0); }
static inline void ST7735S_BLK_High(void) { (void)gpio_pin_out(s_blk, 1); }

static esp_err_t spi_tx(const uint8_t *tx, size_t len)
{
    return spi_comm_transceive(tx, NULL, len);
}

static void ST7735S_WriteIndex(uint8_t command)
{
    ST7735S_DC_Low();
    (void)spi_tx(&command, 1U);
}

static void ST7735S_WriteData8(uint8_t data)
{
    ST7735S_DC_High();
    (void)spi_tx(&data, 1U);
}

static void ST7735S_WriteData16(uint16_t data)
{
    uint8_t buf[2];
    buf[0] = (uint8_t)(data >> 8);
    buf[1] = (uint8_t)(data & 0xFFU);
    ST7735S_DC_High();
    (void)spi_tx(buf, sizeof(buf));
}

static void ST7735S_Reset(void)
{
    ST7735S_RST_Low();
    delay_ms(100);
    ST7735S_RST_High();
    delay_ms(100);
}

static esp_err_t gpio_out_init(gpio_num_t pin)
{
    if (pin == GPIO_NUM_NC) {
        return ESP_OK;
    }
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << (unsigned)pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    return gpio_config(&io);
}

esp_err_t ST7735S_Init(const st7735s_cfg_t *cfg)
{
    if (cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (cfg->spi_clock_hz == 0U) {
        return ESP_ERR_INVALID_ARG;
    }

    s_dc = cfg->dc;
    s_rst = cfg->rst;
    s_blk = cfg->blk;

    esp_err_t err = gpio_out_init(s_dc);
    if (err != ESP_OK) {
        return err;
    }
    err = gpio_out_init(s_rst);
    if (err != ESP_OK) {
        return err;
    }
    err = gpio_out_init(s_blk);
    if (err != ESP_OK) {
        return err;
    }

    err = spi_comm_init(cfg->host, cfg->mosi, cfg->miso, cfg->sclk, cfg->cs, cfg->spi_clock_hz, 0);
    if (err != ESP_OK) {
        return err;
    }

    ST7735S_BLK_High();
    ST7735S_Reset();

    ST7735S_WriteIndex(ST7735S_SLPOUT);
    delay_ms(120);

    ST7735S_WriteIndex(0xB1U);
    ST7735S_WriteData8(0x01U);
    ST7735S_WriteData8(0x2CU);
    ST7735S_WriteData8(0x2DU);

    ST7735S_WriteIndex(0xB2U);
    ST7735S_WriteData8(0x01U);
    ST7735S_WriteData8(0x2CU);
    ST7735S_WriteData8(0x2DU);

    ST7735S_WriteIndex(0xB3U);
    ST7735S_WriteData8(0x01U);
    ST7735S_WriteData8(0x2CU);
    ST7735S_WriteData8(0x2DU);
    ST7735S_WriteData8(0x01U);
    ST7735S_WriteData8(0x2CU);
    ST7735S_WriteData8(0x2DU);

    ST7735S_WriteIndex(0xB4U);
    ST7735S_WriteData8(0x07U);

    ST7735S_WriteIndex(0xC0U);
    ST7735S_WriteData8(0xA2U);
    ST7735S_WriteData8(0x02U);
    ST7735S_WriteData8(0x84U);
    ST7735S_WriteIndex(0xC1U);
    ST7735S_WriteData8(0xC5U);

    ST7735S_WriteIndex(0xC2U);
    ST7735S_WriteData8(0x0AU);
    ST7735S_WriteData8(0x00U);

    ST7735S_WriteIndex(0xC3U);
    ST7735S_WriteData8(0x8AU);
    ST7735S_WriteData8(0x2AU);
    ST7735S_WriteIndex(0xC4U);
    ST7735S_WriteData8(0x8AU);
    ST7735S_WriteData8(0xEEU);

    ST7735S_WriteIndex(0xC5U);
    ST7735S_WriteData8(0x0EU);

    ST7735S_WriteIndex(0x36U);
    ST7735S_WriteData8(0xC0U);

    ST7735S_WriteIndex(0xE0U);
    ST7735S_WriteData8(0x0FU);
    ST7735S_WriteData8(0x1AU);
    ST7735S_WriteData8(0x0FU);
    ST7735S_WriteData8(0x18U);
    ST7735S_WriteData8(0x2FU);
    ST7735S_WriteData8(0x28U);
    ST7735S_WriteData8(0x20U);
    ST7735S_WriteData8(0x22U);
    ST7735S_WriteData8(0x1FU);
    ST7735S_WriteData8(0x1BU);
    ST7735S_WriteData8(0x23U);
    ST7735S_WriteData8(0x37U);
    ST7735S_WriteData8(0x00U);
    ST7735S_WriteData8(0x07U);
    ST7735S_WriteData8(0x02U);
    ST7735S_WriteData8(0x10U);

    ST7735S_WriteIndex(0xE1U);
    ST7735S_WriteData8(0x0FU);
    ST7735S_WriteData8(0x1BU);
    ST7735S_WriteData8(0x0FU);
    ST7735S_WriteData8(0x17U);
    ST7735S_WriteData8(0x33U);
    ST7735S_WriteData8(0x2CU);
    ST7735S_WriteData8(0x29U);
    ST7735S_WriteData8(0x2EU);
    ST7735S_WriteData8(0x30U);
    ST7735S_WriteData8(0x30U);
    ST7735S_WriteData8(0x39U);
    ST7735S_WriteData8(0x3FU);
    ST7735S_WriteData8(0x00U);
    ST7735S_WriteData8(0x07U);
    ST7735S_WriteData8(0x03U);
    ST7735S_WriteData8(0x10U);

    ST7735S_WriteIndex(0x2AU);
    ST7735S_WriteData8(0x00U);
    ST7735S_WriteData8(0x00U);
    ST7735S_WriteData8(0x00U);
    ST7735S_WriteData8(0x7FU);

    ST7735S_WriteIndex(0x2BU);
    ST7735S_WriteData8(0x00U);
    ST7735S_WriteData8(0x00U);
    ST7735S_WriteData8(0x00U);
    ST7735S_WriteData8(0x9FU);

    ST7735S_WriteIndex(0xF0U);
    ST7735S_WriteData8(0x01U);
    ST7735S_WriteIndex(0xF6U);
    ST7735S_WriteData8(0x00U);

    ST7735S_WriteIndex(0x3AU);
    ST7735S_WriteData8(0x05U);

    ST7735S_WriteIndex(ST7735S_DISPON);
    delay_ms(10);

    s_ready = true;
    return ESP_OK;
}

void ST7735S_SetAddressWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    if (!s_ready || (w == 0U) || (h == 0U)) {
        return;
    }
    uint16_t x_start = x;
    uint16_t y_start = y;
    uint16_t x_end = (uint16_t)(x + w - 1U);
    uint16_t y_end = (uint16_t)(y + h - 1U);

    ST7735S_WriteIndex(ST7735S_CASET);
    ST7735S_WriteData8(0x00U);
    ST7735S_WriteData8((uint8_t)x_start);
    ST7735S_WriteData8(0x00U);
    ST7735S_WriteData8((uint8_t)(x_end + s_x_offset));

    ST7735S_WriteIndex(ST7735S_RASET);
    ST7735S_WriteData8(0x00U);
    ST7735S_WriteData8((uint8_t)y_start);
    ST7735S_WriteData8(0x00U);
    ST7735S_WriteData8((uint8_t)(y_end + s_y_offset));

    ST7735S_WriteIndex(ST7735S_RAMWR);
}

static void ST7735S_WriteColor565(uint16_t color)
{
    ST7735S_WriteData16(color);
}

void ST7735S_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (!s_ready || (x >= s_width) || (y >= s_height)) {
        return;
    }
    ST7735S_SetAddressWindow(x, y, 1U, 1U);
    ST7735S_WriteColor565(color);
}

void ST7735S_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (!s_ready || (w == 0U) || (h == 0U) || (x >= s_width) || (y >= s_height)) {
        return;
    }
    if (w > (uint16_t)(s_width - x)) {
        w = (uint16_t)(s_width - x);
    }
    if (h > (uint16_t)(s_height - y)) {
        h = (uint16_t)(s_height - y);
    }
    ST7735S_SetAddressWindow(x, y, w, h);
    uint32_t n = (uint32_t)w * (uint32_t)h;
    while (n != 0U) {
        ST7735S_WriteColor565(color);
        n--;
    }
}

void ST7735S_FillScreen(uint16_t color)
{
    ST7735S_FillRect(0U, 0U, s_width, s_height, color);
}

void ST7735S_WritePixels565(uint16_t w, uint16_t h, const uint16_t *rgb565)
{
    if (!s_ready || (w == 0U) || (h == 0U) || (rgb565 == NULL)) {
        return;
    }
    const size_t total = (size_t)w * (size_t)h * 2U;
    const uint8_t *p = (const uint8_t *)rgb565;
    ST7735S_DC_High();
    for (size_t off = 0; off < total; off += 4096U) {
        size_t chunk = total - off;
        if (chunk > 4096U) {
            chunk = 4096U;
        }
        (void)spi_tx(p + off, chunk);
    }
}