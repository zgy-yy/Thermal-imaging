#include "spi_comm.h"

#include <string.h>

#include "esp_log.h"

static const char *TAG = "spi_comm";

static spi_device_handle_t s_dev;
static spi_host_device_t s_host;
static bool s_inited;

esp_err_t spi_comm_init(spi_host_device_t host, int mosi, int miso, int sclk, int cs,
                        uint32_t clock_hz, int mode)
{
    if (s_inited) {
        return ESP_ERR_INVALID_STATE;
    }
    if (sclk < 0 || mosi < 0 || cs < 0 || clock_hz == 0 || mode < 0 || mode > 3) {
        return ESP_ERR_INVALID_ARG;
    }

    s_host = host;

    spi_bus_config_t buscfg = {
        .mosi_io_num = mosi,
        .miso_io_num = miso,
        .sclk_io_num = sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    esp_err_t err = spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(err));
        return err;
    }

    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = mode,
        .duty_cycle_pos = 0,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz = (int)clock_hz,
        .input_delay_ns = 0,
        .spics_io_num = cs,
        .flags = 0,
        .queue_size = 4,
        .pre_cb = NULL,
        .post_cb = NULL,
    };

    err = spi_bus_add_device(host, &devcfg, &s_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(err));
        spi_bus_free(host);
        return err;
    }

    s_inited = true;
    ESP_LOGI(TAG, "SPI host=%d MOSI=%d MISO=%d CLK=%d CS=%d %luHz mode=%d",
             (int)host, mosi, miso, sclk, cs, (unsigned long)clock_hz, mode);
    return ESP_OK;
}

esp_err_t spi_comm_deinit(void)
{
    if (!s_inited) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = spi_bus_remove_device(s_dev);
    if (err != ESP_OK) {
        return err;
    }
    s_dev = NULL;
    err = spi_bus_free(s_host);
    s_inited = false;
    return err;
}

esp_err_t spi_comm_transceive(const uint8_t *tx, uint8_t *rx, size_t len)
{
    if (!s_inited || len == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    if (tx == NULL && rx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (tx == NULL && rx != NULL) {
        /* Master 读 MISO 仍需 MOSI 侧提供时钟下的发送字节，请传入与 len 等长的哑发缓冲（如全 0xFF） */
        return ESP_ERR_INVALID_ARG;
    }
    if (len > 4096) {
        return ESP_ERR_INVALID_SIZE;
    }

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = (uint32_t)(len * 8);
    t.tx_buffer = tx;
    t.rx_buffer = rx;
    return spi_device_transmit(s_dev, &t);
}