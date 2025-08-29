#include "nhal_esp32_defs.h"
#include "nhal_esp32_helpers.h"

#include <nhal_uart_basic.h>
#include <nhal_uart_types.h>

#include "driver/uart.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void nhal_config_to_esp_config(struct nhal_uart_config *config, uart_config_t *esp_config) {
    esp_config->baud_rate = config->baudrate;

    switch (config->data_bits) {
        case NHAL_UART_DATA_BITS_7:
            esp_config->data_bits = UART_DATA_7_BITS;
            break;
        case NHAL_UART_DATA_BITS_8:
        default:
            esp_config->data_bits = UART_DATA_8_BITS;
            break;
    }

    switch (config->parity) {
        case NHAL_UART_PARITY_EVEN:
            esp_config->parity = UART_PARITY_EVEN;
            break;
        case NHAL_UART_PARITY_ODD:
            esp_config->parity = UART_PARITY_ODD;
            break;
        case NHAL_UART_PARITY_NONE:
        default:
            esp_config->parity = UART_PARITY_DISABLE;
            break;
    }

    switch (config->stop_bits) {
        case NHAL_UART_STOP_BITS_2:
            esp_config->stop_bits = UART_STOP_BITS_2;
            break;
        case NHAL_UART_STOP_BITS_1:
        default:
            esp_config->stop_bits = UART_STOP_BITS_1;
            break;
    }

    esp_config->flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    esp_config->rx_flow_ctrl_thresh = 0;
    esp_config->source_clk = UART_SCLK_DEFAULT;

};

nhal_result_t nhal_uart_init(struct nhal_uart_context * ctx) {
    if (ctx == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (ctx->impl_ctx->is_initialized) {
        return NHAL_OK;
    }

    // Context initialized
    ctx->impl_ctx->is_initialized = true;
    ctx->impl_ctx->is_configured = false;

    return NHAL_OK;
}

nhal_result_t nhal_uart_deinit(struct nhal_uart_context * ctx) {
    esp_err_t err = uart_driver_delete(ctx->uart_bus_id);
    return nhal_map_esp_err(err);
}

nhal_result_t nhal_uart_set_config(struct nhal_uart_context * ctx, struct nhal_uart_config *cfg) {
    if (ctx == NULL || cfg == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    uart_config_t esp_uart_config;
    nhal_config_to_esp_config(cfg, &esp_uart_config);

    // Store timeout from impl config
    ctx->impl_ctx->timeout_ms = cfg->impl_config->default_timeout_ms;

    esp_err_t err = uart_param_config(ctx->uart_bus_id, &esp_uart_config);
    if (err != ESP_OK) {
        return nhal_map_esp_err(err);
    }

    struct nhal_uart_impl_config *impl_cfg = (struct nhal_uart_impl_config *)cfg->impl_config;
    
    err = uart_driver_install(ctx->uart_bus_id, 
                             impl_cfg->rx_buffer_size, 
                             impl_cfg->tx_buffer_size, 
                             impl_cfg->queue_size, 
                             NULL, 
                             impl_cfg->intr_alloc_flags);
    if (err != ESP_OK) {
        return nhal_map_esp_err(err);
    }

    err = uart_set_pin(ctx->uart_bus_id, 
                       impl_cfg->tx_pin_number, 
                       impl_cfg->rx_pin_number, 
                       UART_PIN_NO_CHANGE, 
                       UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        uart_driver_delete(ctx->uart_bus_id);
        return nhal_map_esp_err(err);
    }

    ctx->impl_ctx->is_configured = true;
    return NHAL_OK;
}

nhal_result_t nhal_uart_get_config(struct nhal_uart_context * ctx, struct nhal_uart_config *cfg) {
    return NHAL_ERR_OTHER; // esp-idf does not provide a function to get config
}

nhal_result_t nhal_uart_write(struct nhal_uart_context * ctx, const uint8_t *data, size_t len) {
    if (ctx == NULL || data == NULL || len == 0 || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!ctx->impl_ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    int bytes_written = uart_write_bytes(ctx->uart_bus_id, (const char *)data, len);
    if (bytes_written == len) {
        return NHAL_OK;
    }
    return NHAL_ERR_OTHER;
}

nhal_result_t nhal_uart_read(struct nhal_uart_context * ctx, uint8_t *data, size_t len) {
    if (ctx == NULL || data == NULL || len == 0 || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!ctx->impl_ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    int bytes_read = uart_read_bytes(ctx->uart_bus_id, data, len, pdMS_TO_TICKS(ctx->impl_ctx->timeout_ms));
    if (bytes_read == len) {
        return NHAL_OK;
    } else if (bytes_read >= 0) {
        return NHAL_ERR_TIMEOUT;
    } else {
        return NHAL_ERR_OTHER;
    }
}
