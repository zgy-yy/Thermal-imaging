#pragma once

#include <stddef.h>
#include <stdint.h>

#include "driver/spi_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t spi_comm_init(spi_host_device_t host, int mosi, int miso, int sclk, int cs,
                        uint32_t clock_hz, int mode);
esp_err_t spi_comm_deinit(void);
esp_err_t spi_comm_transceive(const uint8_t *tx, uint8_t *rx, size_t len);

#ifdef __cplusplus
}
#endif