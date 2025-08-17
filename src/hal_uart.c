#include "hal_esp32_defs.h"

#include <hal_uart_basic.h>
#include <hal_uart_types.h>

#include "driver/uart.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void hal_config_to_esp_config(struct hal_uart_config *config, uart_config_t *esp_config) {
    esp_config->baud_rate = config->baudrate;

    switch (config->data_bits) {
        case HAL_UART_DATA_BITS_7:
            esp_config->data_bits = UART_DATA_7_BITS;
            break;
        case HAL_UART_DATA_BITS_8:
        default:
            esp_config->data_bits = UART_DATA_8_BITS;
            break;
    }

    switch (config->parity) {
        case HAL_UART_PARITY_EVEN:
            esp_config->parity = UART_PARITY_EVEN;
            break;
        case HAL_UART_PARITY_ODD:
            esp_config->parity = UART_PARITY_ODD;
            break;
        case HAL_UART_PARITY_NONE:
        default:
            esp_config->parity = UART_PARITY_DISABLE;
            break;
    }

    switch (config->stop_bits) {
        case HAL_UART_STOP_BITS_2:
            esp_config->stop_bits = UART_STOP_BITS_2;
            break;
        case HAL_UART_STOP_BITS_1:
        default:
            esp_config->stop_bits = UART_STOP_BITS_1;
            break;
    }

    esp_config->flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    esp_config->rx_flow_ctrl_thresh = 0;
    esp_config->source_clk = UART_SCLK_DEFAULT;

};

hal_uart_result_t hal_uart_init(struct hal_uart_context * uart_ctxt) {
    // Initialize context to sync-only mode
    uart_ctxt->current_mode = HAL_UART_OP_MODE_SYNC_ONLY;
    return HAL_UART_OK;
}

hal_uart_result_t hal_uart_deinit(struct hal_uart_context * uart_ctxt) {
    esp_err_t err = uart_driver_delete(uart_ctxt->uart_bus_id);
    return esp_err_to_uart_hal_err(err);
}

hal_uart_result_t hal_uart_set_config(struct hal_uart_context * uart_ctxt, struct hal_uart_config *cfg) {
    uart_config_t esp_uart_config;
    hal_config_to_esp_config(cfg, &esp_uart_config);

    esp_err_t err = uart_param_config(uart_ctxt->uart_bus_id, &esp_uart_config);
    if (err != ESP_OK) {
        return esp_err_to_uart_hal_err(err);
    }

    err = uart_driver_install(uart_ctxt->uart_bus_id, 0, 0, 0, NULL, 0);
    return esp_err_to_uart_hal_err(err);
}

hal_uart_result_t hal_uart_get_config(struct hal_uart_context * uart_ctxt, struct hal_uart_config *cfg) {
    return HAL_UART_ERR_OTHER; // esp-idf does not provide a function to get config
}

hal_uart_result_t hal_uart_write(struct hal_uart_context * uart_ctxt, const uint8_t *data, size_t len, hal_timeout_ms timeout) {
    int bytes_written = uart_write_bytes(uart_ctxt->uart_bus_id, (const char *)data, len);
    if (bytes_written == len) {
        return HAL_UART_OK;
    }
    return HAL_UART_ERR_OTHER;
}

hal_uart_result_t hal_uart_read(struct hal_uart_context * uart_ctxt, uint8_t *data, size_t len, hal_timeout_ms timeout) {
    int bytes_read = uart_read_bytes(uart_ctxt->uart_bus_id, data, len, pdMS_TO_TICKS(timeout));
    if (bytes_read == len) {
        return HAL_UART_OK;
    } else if (bytes_read >= 0) {
        return HAL_UART_ERR_TIMEOUT;
    } else {
        return HAL_UART_ERR_OTHER;
    }
}
