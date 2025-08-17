#include "esp_log.h"
#include "hal_esp32_defs.h"

#include "hal_common.h"
#include "hal_uart_types.h"
#include "hal_uart_async.h"

#include "esp_err.h"
#include "driver/uart.h"

// Only compile if async buffered support is enabled
#if defined(HAL_UART_ASYNC_BUFFERED_SUPPORT)

static void hal_async_config_to_esp_config(struct hal_uart_config *config, const struct hal_uart_async_buffered_config *async_cfg, uart_config_t *esp_config) {
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
}

hal_uart_result_t hal_uart_enable_async_mode(
    struct hal_uart_context * uart_ctxt,
    const struct hal_uart_async_buffered_config *buffered_cfg
) {
    if (uart_ctxt == NULL || buffered_cfg == NULL || uart_ctxt->impl_ctx == NULL) {
        return HAL_UART_ERR_INVALID_ARG;
    }
    
    if (!uart_ctxt->impl_ctx->is_initialized) {
        return HAL_UART_ERR_INVALID_ARG;
    }
    
    if (!uart_ctxt->impl_ctx->is_driver_installed) {
        return HAL_UART_ERR_INVALID_ARG;
    }
    
    BaseType_t mutex_ret_err = xSemaphoreTake(uart_ctxt->impl_ctx->mutex, pdMS_TO_TICKS(1000));
    if (mutex_ret_err == pdTRUE) {
        // Update mode to sync-and-async
        uart_ctxt->current_mode = HAL_UART_OP_MODE_SYNC_AND_ASYNC;
        
        // Store async configuration in context extension
        uart_ctxt->async_buffered.tx_buffer_size = buffered_cfg->tx_buffer_size;
        uart_ctxt->async_buffered.rx_buffer_size = buffered_cfg->rx_buffer_size;
        uart_ctxt->async_buffered.tx_complete_cb = buffered_cfg->tx_complete_cb;
        uart_ctxt->async_buffered.rx_complete_cb = buffered_cfg->rx_complete_cb;
        uart_ctxt->async_buffered.error_cb = buffered_cfg->error_cb;
        uart_ctxt->async_buffered.callback_context = buffered_cfg->callback_context;
        uart_ctxt->async_buffered.tx_buffer = buffered_cfg->tx_buffer;
        uart_ctxt->async_buffered.rx_buffer = buffered_cfg->rx_buffer;
        uart_ctxt->async_buffered.tx_bytes_queued = 0;
        uart_ctxt->async_buffered.rx_bytes_available = 0;
        uart_ctxt->async_buffered.is_async_initialized = true;
        
        xSemaphoreGive(uart_ctxt->impl_ctx->mutex);
        return HAL_UART_OK;
    } else {
        return HAL_UART_ERR_BUS_BUSY;
    }
}

hal_uart_result_t hal_uart_disable_async_mode(
    struct hal_uart_context * uart_ctxt
) {
    if (uart_ctxt == NULL || uart_ctxt->impl_ctx == NULL) {
        return HAL_UART_ERR_INVALID_ARG;
    }
    
    BaseType_t mutex_ret_err = xSemaphoreTake(uart_ctxt->impl_ctx->mutex, pdMS_TO_TICKS(1000));
    if (mutex_ret_err == pdTRUE) {
        // Revert to sync-only mode
        uart_ctxt->current_mode = HAL_UART_OP_MODE_SYNC_ONLY;
        uart_ctxt->async_buffered.is_async_initialized = false;
        
        xSemaphoreGive(uart_ctxt->impl_ctx->mutex);
        return HAL_UART_OK;
    } else {
        return HAL_UART_ERR_BUS_BUSY;
    }
}

hal_uart_result_t hal_uart_set_buffered_config(
    struct hal_uart_context * uart_ctxt,
    const struct hal_uart_async_buffered_config *buffered_cfg
) {
    if (uart_ctxt == NULL || buffered_cfg == NULL) {
        return HAL_UART_ERR_INVALID_ARG;
    }
    
    if (uart_ctxt->current_mode != HAL_UART_OP_MODE_SYNC_AND_ASYNC || !uart_ctxt->async_buffered.is_async_initialized) {
        return HAL_UART_ERR_INVALID_ARG; // Async mode not enabled or not initialized
    }
    
    // Update configuration in context extension
    uart_ctxt->async_buffered.tx_buffer_size = buffered_cfg->tx_buffer_size;
    uart_ctxt->async_buffered.rx_buffer_size = buffered_cfg->rx_buffer_size;
    uart_ctxt->async_buffered.tx_complete_cb = buffered_cfg->tx_complete_cb;
    uart_ctxt->async_buffered.rx_complete_cb = buffered_cfg->rx_complete_cb;
    uart_ctxt->async_buffered.error_cb = buffered_cfg->error_cb;
    uart_ctxt->async_buffered.callback_context = buffered_cfg->callback_context;
    uart_ctxt->async_buffered.tx_buffer = buffered_cfg->tx_buffer;
    uart_ctxt->async_buffered.rx_buffer = buffered_cfg->rx_buffer;
    
    return HAL_UART_OK;
}

hal_uart_result_t hal_uart_write_async(
    struct hal_uart_context * uart_ctxt,
    const uint8_t *data, size_t len,
    hal_timeout_ms timeout
) {
    if (uart_ctxt == NULL || data == NULL || len == 0) {
        return HAL_UART_ERR_INVALID_ARG;
    }
    
    if (uart_ctxt->current_mode != HAL_UART_OP_MODE_SYNC_AND_ASYNC || !uart_ctxt->async_buffered.is_async_initialized) {
        return HAL_UART_ERR_INVALID_ARG; // Async mode not enabled or not initialized
    }
    
    // Use ESP-IDF's buffered write function
    int bytes_written = uart_write_bytes(uart_ctxt->uart_bus_id, (const char *)data, len);
    if (bytes_written == len) {
        uart_ctxt->async_buffered.tx_bytes_queued += len;
        return HAL_UART_OK;
    } else if (bytes_written >= 0) {
        uart_ctxt->async_buffered.tx_bytes_queued += bytes_written;
        return HAL_UART_ERR_TIMEOUT; // Partial write
    } else {
        return HAL_UART_ERR_OTHER;
    }
}

hal_uart_result_t hal_uart_read_async(
    struct hal_uart_context * uart_ctxt,
    uint8_t *data, size_t len, size_t *bytes_read,
    hal_timeout_ms timeout
) {
    if (uart_ctxt == NULL || data == NULL || len == 0 || bytes_read == NULL) {
        return HAL_UART_ERR_INVALID_ARG;
    }
    
    if (uart_ctxt->current_mode != HAL_UART_OP_MODE_SYNC_AND_ASYNC || !uart_ctxt->async_buffered.is_async_initialized) {
        return HAL_UART_ERR_INVALID_ARG; // Async mode not enabled or not initialized
    }
    
    // Use ESP-IDF's buffered read function
    int read_result = uart_read_bytes(uart_ctxt->uart_bus_id, data, len, pdMS_TO_TICKS(timeout));
    if (read_result >= 0) {
        *bytes_read = read_result;
        if (read_result == len) {
            return HAL_UART_OK;
        } else {
            return HAL_UART_ERR_TIMEOUT; // Partial read or timeout
        }
    } else {
        *bytes_read = 0;
        return HAL_UART_ERR_OTHER;
    }
}

hal_uart_result_t hal_uart_get_rx_available(
    struct hal_uart_context * uart_ctxt,
    size_t *bytes_available
) {
    if (uart_ctxt == NULL || bytes_available == NULL) {
        return HAL_UART_ERR_INVALID_ARG;
    }
    
    if (uart_ctxt->current_mode != HAL_UART_OP_MODE_SYNC_AND_ASYNC || !uart_ctxt->async_buffered.is_async_initialized) {
        return HAL_UART_ERR_INVALID_ARG; // Async mode not enabled or not initialized
    }
    
    // Get number of bytes available in RX buffer
    size_t available;
    esp_err_t ret_err = uart_get_buffered_data_len(uart_ctxt->uart_bus_id, &available);
    if (ret_err == ESP_OK) {
        *bytes_available = available;
        uart_ctxt->async_buffered.rx_bytes_available = available;
        return HAL_UART_OK;
    } else {
        *bytes_available = 0;
        return esp_err_to_uart_hal_err(ret_err);
    }
}

hal_uart_result_t hal_uart_get_tx_free(
    struct hal_uart_context * uart_ctxt,
    size_t *bytes_free
) {
    if (uart_ctxt == NULL || bytes_free == NULL) {
        return HAL_UART_ERR_INVALID_ARG;
    }
    
    if (uart_ctxt->current_mode != HAL_UART_OP_MODE_SYNC_AND_ASYNC || !uart_ctxt->async_buffered.is_async_initialized) {
        return HAL_UART_ERR_INVALID_ARG; // Async mode not enabled or not initialized
    }
    
    // ESP-IDF doesn't provide direct access to TX buffer free space
    // Estimate based on buffer size and queued bytes
    size_t estimated_free = uart_ctxt->async_buffered.tx_buffer_size - uart_ctxt->async_buffered.tx_bytes_queued;
    if (estimated_free > uart_ctxt->async_buffered.tx_buffer_size) {
        estimated_free = 0; // Handle underflow
    }
    
    *bytes_free = estimated_free;
    return HAL_UART_OK;
}

hal_uart_result_t hal_uart_flush_tx(
    struct hal_uart_context * uart_ctxt,
    hal_timeout_ms timeout
) {
    if (uart_ctxt == NULL) {
        return HAL_UART_ERR_INVALID_ARG;
    }
    
    if (uart_ctxt->current_mode != HAL_UART_OP_MODE_SYNC_AND_ASYNC || !uart_ctxt->async_buffered.is_async_initialized) {
        return HAL_UART_ERR_INVALID_ARG; // Async mode not enabled or not initialized
    }
    
    // Wait for TX to complete
    esp_err_t ret_err = uart_wait_tx_done(uart_ctxt->uart_bus_id, pdMS_TO_TICKS(timeout));
    if (ret_err == ESP_OK) {
        uart_ctxt->async_buffered.tx_bytes_queued = 0; // Reset queued count
        return HAL_UART_OK;
    } else {
        return esp_err_to_uart_hal_err(ret_err);
    }
}

hal_uart_result_t hal_uart_clear_rx(
    struct hal_uart_context * uart_ctxt
) {
    if (uart_ctxt == NULL) {
        return HAL_UART_ERR_INVALID_ARG;
    }
    
    if (uart_ctxt->current_mode != HAL_UART_OP_MODE_SYNC_AND_ASYNC || !uart_ctxt->async_buffered.is_async_initialized) {
        return HAL_UART_ERR_INVALID_ARG; // Async mode not enabled or not initialized
    }
    
    // Flush RX buffer
    esp_err_t ret_err = uart_flush_input(uart_ctxt->uart_bus_id);
    if (ret_err == ESP_OK) {
        uart_ctxt->async_buffered.rx_bytes_available = 0; // Reset available count
        return HAL_UART_OK;
    } else {
        return esp_err_to_uart_hal_err(ret_err);
    }
}

#endif /* HAL_UART_ASYNC_BUFFERED_SUPPORT */