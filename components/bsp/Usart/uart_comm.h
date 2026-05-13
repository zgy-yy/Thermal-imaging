#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t uart_comm_init(int tx_io, int rx_io, int baud);
esp_err_t uart_comm_write(const uint8_t *data, size_t len);
int uart_comm_read(uint8_t *buf, size_t max_len, TickType_t timeout_ticks);

#ifdef __cplusplus
}
#endif
