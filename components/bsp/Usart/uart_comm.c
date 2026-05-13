#include "uart_comm.h"

#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "bsp_uart";

#define UART_COMM_PORT       UART_NUM_1
#define UART_COMM_BUF_SIZE   (1024)

esp_err_t uart_comm_init(int tx_io, int rx_io, int baud)
{
    uart_config_t cfg = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_driver_install(UART_COMM_PORT, UART_COMM_BUF_SIZE * 2, UART_COMM_BUF_SIZE * 2, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_param_config(UART_COMM_PORT, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_set_pin(UART_COMM_PORT, tx_io, rx_io, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(err));
        return err;
    }

    uart_flush(UART_COMM_PORT);
    ESP_LOGI(TAG, "UART%d TX=%d RX=%d baud=%d", (int)UART_COMM_PORT, tx_io, rx_io, baud);
    return ESP_OK;
}

esp_err_t uart_comm_write(const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    int n = uart_write_bytes(UART_COMM_PORT, data, len);
    if (n < 0) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

int uart_comm_read(uint8_t *buf, size_t max_len, uint32_t timeout_ticks)
{
    if (buf == NULL || max_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    int n = uart_read_bytes(UART_COMM_PORT, buf, max_len, (TickType_t)timeout_ticks);
    if (n < 0) {
        return ESP_FAIL;
    }
    return n;
}
