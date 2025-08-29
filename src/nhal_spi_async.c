#include "esp_log.h"
#include "nhal_esp32_defs.h"
#include "nhal_esp32_helpers.h"

#include "nhal_common.h"
#include "nhal_spi_types.h"
#include "nhal_spi_master_async.h"

#include "esp_err.h"
#include "driver/spi_master.h"
#include <string.h>
#include <stdlib.h>

#if defined(NHAL_SPI_ASYNC_SUPPORT)

static nhal_async_complete_cb_t global_callback = NULL;

// Structure to track async operations
typedef struct {
    struct nhal_spi_context *spi_ctx;
    void *user_context;  // Context to pass to callback
} nhal_spi_async_transaction_t;

// ESP-IDF transaction completion callback
static void spi_async_transaction_cb(spi_transaction_t *trans) {
    nhal_spi_async_transaction_t *async_trans = (nhal_spi_async_transaction_t *)trans->user;
    
    if (async_trans && global_callback) {
        global_callback(async_trans->user_context);
    }
    
    // Clean up the transaction
    if (async_trans) {
        free(async_trans);
    }
    if (trans) {
        free(trans);
    }
}

nhal_result_t nhal_spi_master_init_async(
    struct nhal_spi_context * ctx,
    const struct nhal_async_config * async_cfg
) {
    if (ctx == NULL || async_cfg == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }
    
    struct nhal_spi_async_impl_config *impl_async_cfg = (struct nhal_spi_async_impl_config *)async_cfg->impl_config;
    if (impl_async_cfg == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    BaseType_t mutex_ret_err = xSemaphoreTake(ctx->impl_ctx->mutex, pdMS_TO_TICKS(1000));
    if (mutex_ret_err == pdTRUE) {
        // Create async device handle for ESP-IDF
        spi_device_interface_config_t esp_device_config = {0};
        esp_device_config.command_bits = 0;
        esp_device_config.address_bits = 0;
        esp_device_config.dummy_bits = 0;
        esp_device_config.clock_speed_hz = ctx->actual_frequency_hz;
        esp_device_config.duty_cycle_pos = 128;
        esp_device_config.cs_ena_pretrans = 0;
        esp_device_config.cs_ena_posttrans = 0;
        esp_device_config.flags = 0;
        esp_device_config.queue_size = 3; // Default queue size
        esp_device_config.pre_cb = NULL;
        esp_device_config.post_cb = spi_async_transaction_cb;
        esp_device_config.spics_io_num = -1; // Use software CS
        esp_device_config.mode = 0; // Will be set based on current config
        
        esp_err_t ret_err = spi_bus_add_device(ctx->spi_bus_id, &esp_device_config, &ctx->impl_ctx->async_device_handle);
        if (ret_err != ESP_OK) {
            xSemaphoreGive(ctx->impl_ctx->mutex);
            return nhal_map_esp_err(ret_err);
        }
        
        // Async mode initialized
        
        xSemaphoreGive(ctx->impl_ctx->mutex);
        return NHAL_OK;
    } else {
        return NHAL_ERR_BUSY;
    }
}

nhal_result_t nhal_spi_master_deinit_async(
    struct nhal_spi_context * ctx
) {
    if (ctx == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    BaseType_t mutex_ret_err = xSemaphoreTake(ctx->impl_ctx->mutex, pdMS_TO_TICKS(1000));
    if (mutex_ret_err == pdTRUE) {
        // Remove async device if present
        if (ctx->impl_ctx->async_device_handle != NULL) {
            esp_err_t ret_err = spi_bus_remove_device(ctx->impl_ctx->async_device_handle);
            if (ret_err != ESP_OK) {
                xSemaphoreGive(ctx->impl_ctx->mutex);
                return nhal_map_esp_err(ret_err);
            }
            ctx->impl_ctx->async_device_handle = NULL;
        }
        
        // Async mode disabled
        
        xSemaphoreGive(ctx->impl_ctx->mutex);
        return NHAL_OK;
    } else {
        return NHAL_ERR_BUSY;
    }
}

nhal_result_t nhal_spi_master_set_async_callback(
    struct nhal_spi_context *ctx,
    nhal_async_complete_cb_t callback
) {
    if (ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    global_callback = callback;
    return NHAL_OK;
}

nhal_async_status_t nhal_spi_master_get_async_status(
    struct nhal_spi_context *ctx
) {
    if (ctx == NULL) {
        return NHAL_ASYNC_STATUS_ERROR;
    }
    
    // For now, always return idle - a real implementation would track ongoing operations
    return NHAL_ASYNC_STATUS_IDLE;
}

nhal_result_t nhal_spi_write_async(
    struct nhal_spi_context * ctx,
    const uint8_t * data, size_t len
) {
    if (ctx == NULL || data == NULL || len == 0) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (ctx->impl_ctx->async_device_handle == NULL) {
        return NHAL_ERR_NOT_INITIALIZED;
    }
    
    // Allocate transaction structure
    spi_transaction_t *trans = heap_caps_malloc(sizeof(spi_transaction_t), MALLOC_CAP_DMA);
    if (trans == NULL) {
        return NHAL_ERR_OUT_OF_MEMORY;
    }
    
    nhal_spi_async_transaction_t *async_trans = malloc(sizeof(nhal_spi_async_transaction_t));
    if (async_trans == NULL) {
        free(trans);
        return NHAL_ERR_OUT_OF_MEMORY;
    }
    
    // Setup transaction
    memset(trans, 0, sizeof(spi_transaction_t));
    trans->length = len * 8; // Length in bits
    trans->tx_buffer = data;
    trans->rx_buffer = NULL;
    trans->user = async_trans;
    
    // Setup async tracking
    async_trans->spi_ctx = ctx;
    async_trans->user_context = ctx;  // Pass SPI context as default
    
    // Queue the transaction
    esp_err_t ret_err = spi_device_queue_trans(ctx->impl_ctx->async_device_handle, trans, pdMS_TO_TICKS(ctx->impl_ctx->timeout_ms));
    if (ret_err != ESP_OK) {
        free(async_trans);
        free(trans);
        return nhal_map_esp_err(ret_err);
    }
    
    return NHAL_OK;
}

nhal_result_t nhal_spi_read_async(
    struct nhal_spi_context * ctx,
    uint8_t * data, size_t len
) {
    if (ctx == NULL || data == NULL || len == 0) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (ctx->impl_ctx->async_device_handle == NULL) {
        return NHAL_ERR_NOT_INITIALIZED;
    }
    
    // Allocate transaction structure
    spi_transaction_t *trans = heap_caps_malloc(sizeof(spi_transaction_t), MALLOC_CAP_DMA);
    if (trans == NULL) {
        return NHAL_ERR_OUT_OF_MEMORY;
    }
    
    nhal_spi_async_transaction_t *async_trans = malloc(sizeof(nhal_spi_async_transaction_t));
    if (async_trans == NULL) {
        free(trans);
        return NHAL_ERR_OUT_OF_MEMORY;
    }
    
    // Setup transaction
    memset(trans, 0, sizeof(spi_transaction_t));
    trans->length = len * 8; // Length in bits
    trans->tx_buffer = NULL;
    trans->rx_buffer = data;
    trans->user = async_trans;
    
    // Setup async tracking
    async_trans->spi_ctx = ctx;
    async_trans->user_context = ctx;  // Pass SPI context as default
    
    // Queue the transaction
    esp_err_t ret_err = spi_device_queue_trans(ctx->impl_ctx->async_device_handle, trans, pdMS_TO_TICKS(ctx->impl_ctx->timeout_ms));
    if (ret_err != ESP_OK) {
        free(async_trans);
        free(trans);
        return nhal_map_esp_err(ret_err);
    }
    
    return NHAL_OK;
}

nhal_result_t nhal_spi_write_read_async(
    struct nhal_spi_context * ctx,
    const uint8_t * tx_data, size_t tx_len,
    uint8_t * rx_data, size_t rx_len
) {
    if (ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if ((tx_data == NULL && tx_len > 0) || (rx_data == NULL && rx_len > 0)) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (ctx->impl_ctx->async_device_handle == NULL) {
        return NHAL_ERR_NOT_INITIALIZED;
    }
    
    // Allocate transaction structure
    spi_transaction_t *trans = heap_caps_malloc(sizeof(spi_transaction_t), MALLOC_CAP_DMA);
    if (trans == NULL) {
        return NHAL_ERR_OUT_OF_MEMORY;
    }
    
    nhal_spi_async_transaction_t *async_trans = malloc(sizeof(nhal_spi_async_transaction_t));
    if (async_trans == NULL) {
        free(trans);
        return NHAL_ERR_OUT_OF_MEMORY;
    }
    
    // Setup transaction
    memset(trans, 0, sizeof(spi_transaction_t));
    size_t transfer_len = (tx_len > rx_len) ? tx_len : rx_len;
    trans->length = transfer_len * 8; // Length in bits
    trans->tx_buffer = tx_data;
    trans->rx_buffer = rx_data;
    trans->user = async_trans;
    
    // Setup async tracking
    async_trans->spi_ctx = ctx;
    async_trans->user_context = ctx;  // Pass SPI context as default
    
    // Queue the transaction
    esp_err_t ret_err = spi_device_queue_trans(ctx->impl_ctx->async_device_handle, trans, pdMS_TO_TICKS(ctx->impl_ctx->timeout_ms));
    if (ret_err != ESP_OK) {
        free(async_trans);
        free(trans);
        return nhal_map_esp_err(ret_err);
    }
    
    return NHAL_OK;
}

#endif /* NHAL_SPI_ASYNC_SUPPORT */