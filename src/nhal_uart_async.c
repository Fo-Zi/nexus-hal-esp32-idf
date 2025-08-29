#include "esp_log.h"
#include "nhal_esp32_defs.h"

#include "nhal_common.h"
#include "nhal_uart_types.h"
#include "nhal_uart_async.h"

#include "esp_err.h"
#include "driver/uart.h"

// Only compile if async buffered support is enabled
#if defined(NHAL_UART_ASYNC_BUFFERED_SUPPORT)

static void nhal_async_config_to_esp_config(struct nhal_uart_config *config, const struct nhal_uart_async_buffered_config *async_cfg, uart_config_t *esp_config) {
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
}

nhal_result_t nhal_uart_enable_async_mode(
    struct nhal_uart_context * ctx,
    const struct nhal_uart_async_buffered_config *buffered_cfg
) {
    if (ctx == NULL || buffered_cfg == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (!ctx->impl_ctx->is_driver_installed) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    BaseType_t mutex_ret_err = xSemaphoreTake(ctx->impl_ctx->mutex, pdMS_TO_TICKS(1000));
    if (mutex_ret_err == pdTRUE) {
        // Async mode initialized
        
        // Store async configuration in context extension
        ctx->async_buffered.tx_buffer_size = buffered_cfg->tx_buffer_size;
        ctx->async_buffered.rx_buffer_size = buffered_cfg->rx_buffer_size;
        ctx->async_buffered.tx_complete_cb = buffered_cfg->tx_complete_cb;
        ctx->async_buffered.rx_complete_cb = buffered_cfg->rx_complete_cb;
        ctx->async_buffered.error_cb = buffered_cfg->error_cb;
        ctx->async_buffered.callback_context = buffered_cfg->callback_context;
        ctx->async_buffered.tx_buffer = buffered_cfg->tx_buffer;
        ctx->async_buffered.rx_buffer = buffered_cfg->rx_buffer;
        ctx->async_buffered.tx_bytes_queued = 0;
        ctx->async_buffered.rx_bytes_available = 0;
        ctx->async_buffered.is_async_initialized = true;
        
        xSemaphoreGive(ctx->impl_ctx->mutex);
        return NHAL_OK;
    } else {
        return NHAL_ERR_BUS_BUSY;
    }
}

nhal_result_t nhal_uart_disable_async_mode(
    struct nhal_uart_context * ctx
) {
    if (ctx == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    BaseType_t mutex_ret_err = xSemaphoreTake(ctx->impl_ctx->mutex, pdMS_TO_TICKS(1000));
    if (mutex_ret_err == pdTRUE) {
        // Async mode disabled
        ctx->async_buffered.is_async_initialized = false;
        
        xSemaphoreGive(ctx->impl_ctx->mutex);
        return NHAL_OK;
    } else {
        return NHAL_ERR_BUS_BUSY;
    }
}

nhal_result_t nhal_uart_set_buffered_config(
    struct nhal_uart_context * ctx,
    const struct nhal_uart_async_buffered_config *buffered_cfg
) {
    if (ctx == NULL || buffered_cfg == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (!ctx->async_buffered.is_async_initialized) {
        return NHAL_ERR_INVALID_ARG; // Async mode not enabled or not initialized
    }
    
    // Update configuration in context extension
    ctx->async_buffered.tx_buffer_size = buffered_cfg->tx_buffer_size;
    ctx->async_buffered.rx_buffer_size = buffered_cfg->rx_buffer_size;
    ctx->async_buffered.tx_complete_cb = buffered_cfg->tx_complete_cb;
    ctx->async_buffered.rx_complete_cb = buffered_cfg->rx_complete_cb;
    ctx->async_buffered.error_cb = buffered_cfg->error_cb;
    ctx->async_buffered.callback_context = buffered_cfg->callback_context;
    ctx->async_buffered.tx_buffer = buffered_cfg->tx_buffer;
    ctx->async_buffered.rx_buffer = buffered_cfg->rx_buffer;
    
    return NHAL_OK;
}

nhal_result_t nhal_uart_write_async(
    struct nhal_uart_context * ctx,
    const uint8_t *data, size_t len
) {
    if (ctx == NULL || data == NULL || len == 0) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (!ctx->async_buffered.is_async_initialized) {
        return NHAL_ERR_INVALID_ARG; // Async mode not enabled or not initialized
    }
    
    // Use ESP-IDF's buffered write function
    int bytes_written = uart_write_bytes(ctx->uart_bus_id, (const char *)data, len);
    if (bytes_written == len) {
        ctx->async_buffered.tx_bytes_queued += len;
        return NHAL_OK;
    } else if (bytes_written >= 0) {
        ctx->async_buffered.tx_bytes_queued += bytes_written;
        return NHAL_ERR_TIMEOUT; // Partial write
    } else {
        return NHAL_ERR_OTHER;
    }
}

nhal_result_t nhal_uart_read_async(
    struct nhal_uart_context * ctx,
    uint8_t *data, size_t len, size_t *bytes_read
) {
    if (ctx == NULL || data == NULL || len == 0 || bytes_read == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (!ctx->async_buffered.is_async_initialized) {
        return NHAL_ERR_INVALID_ARG; // Async mode not enabled or not initialized
    }
    
    // Use ESP-IDF's buffered read function
    int read_result = uart_read_bytes(ctx->uart_bus_id, data, len, pdMS_TO_TICKS(ctx->impl_ctx->timeout_ms));
    if (read_result >= 0) {
        *bytes_read = read_result;
        if (read_result == len) {
            return NHAL_OK;
        } else {
            return NHAL_ERR_TIMEOUT; // Partial read or timeout
        }
    } else {
        *bytes_read = 0;
        return NHAL_ERR_OTHER;
    }
}

nhal_result_t nhal_uart_get_rx_available(
    struct nhal_uart_context * ctx,
    size_t *bytes_available
) {
    if (ctx == NULL || bytes_available == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (!ctx->async_buffered.is_async_initialized) {
        return NHAL_ERR_INVALID_ARG; // Async mode not enabled or not initialized
    }
    
    // Get number of bytes available in RX buffer
    size_t available;
    esp_err_t ret_err = uart_get_buffered_data_len(ctx->uart_bus_id, &available);
    if (ret_err == ESP_OK) {
        *bytes_available = available;
        ctx->async_buffered.rx_bytes_available = available;
        return NHAL_OK;
    } else {
        *bytes_available = 0;
        return nhal_map_esp_err(ret_err);
    }
}

nhal_result_t nhal_uart_get_tx_free(
    struct nhal_uart_context * ctx,
    size_t *bytes_free
) {
    if (ctx == NULL || bytes_free == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (!ctx->async_buffered.is_async_initialized) {
        return NHAL_ERR_INVALID_ARG; // Async mode not enabled or not initialized
    }
    
    // ESP-IDF doesn't provide direct access to TX buffer free space
    // Estimate based on buffer size and queued bytes
    size_t estimated_free = ctx->async_buffered.tx_buffer_size - ctx->async_buffered.tx_bytes_queued;
    if (estimated_free > ctx->async_buffered.tx_buffer_size) {
        estimated_free = 0; // Handle underflow
    }
    
    *bytes_free = estimated_free;
    return NHAL_OK;
}

nhal_result_t nhal_uart_flush_tx(
    struct nhal_uart_context * ctx
) {
    if (ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (!ctx->async_buffered.is_async_initialized) {
        return NHAL_ERR_INVALID_ARG; // Async mode not enabled or not initialized
    }
    
    // Wait for TX to complete
    esp_err_t ret_err = uart_wait_tx_done(ctx->uart_bus_id, pdMS_TO_TICKS(ctx->impl_ctx->timeout_ms));
    if (ret_err == ESP_OK) {
        ctx->async_buffered.tx_bytes_queued = 0; // Reset queued count
        return NHAL_OK;
    } else {
        return nhal_map_esp_err(ret_err);
    }
}

nhal_result_t nhal_uart_clear_rx(
    struct nhal_uart_context * ctx
) {
    if (ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (!ctx->async_buffered.is_async_initialized) {
        return NHAL_ERR_INVALID_ARG; // Async mode not enabled or not initialized
    }
    
    // Flush RX buffer
    esp_err_t ret_err = uart_flush_input(ctx->uart_bus_id);
    if (ret_err == ESP_OK) {
        ctx->async_buffered.rx_bytes_available = 0; // Reset available count
        return NHAL_OK;
    } else {
        return nhal_map_esp_err(ret_err);
    }
}

#endif /* NHAL_UART_ASYNC_BUFFERED_SUPPORT */