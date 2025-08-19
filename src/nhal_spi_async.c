#include "esp_log.h"
#include "nhal_esp32_defs.h"

#include "nhal_common.h"
#include "nhal_spi_types.h"
#include "nhal_spi_master_async.h"

#include "esp_err.h"
#include "driver/spi_master.h"

// Only compile if async DMA support is enabled
#if defined(NHAL_SPI_ASYNC_DMA_SUPPORT)

// Structure to track async operations
typedef struct {
    nhal_spi_async_complete_cb_t callback;
    void *callback_ctx;
    nhal_result_t *result_ptr;
    struct nhal_spi_context *spi_ctx;
} nhal_spi_async_transaction_t;

// ESP-IDF transaction completion callback
static void spi_async_transaction_cb(spi_transaction_t *trans) {
    nhal_spi_async_transaction_t *async_trans = (nhal_spi_async_transaction_t *)trans->user;
    
    if (async_trans && async_trans->callback) {
        nhal_result_t result = nhal_map_esp_err(ESP_OK); // Transaction completed successfully
        async_trans->callback(async_trans->callback_ctx, result);
    }
    
    // Clean up the transaction
    if (async_trans) {
        free(async_trans);
    }
}

static void nhal_async_config_to_esp_config(struct nhal_spi_config *config, const struct nhal_spi_async_dma_config *async_cfg, spi_device_interface_config_t *esp_config) {
    esp_config->command_bits = 0;
    esp_config->address_bits = 0;
    esp_config->dummy_bits = 0;
    esp_config->clock_speed_hz = config->frequency_hz;
    esp_config->duty_cycle_pos = 128; // 50% duty cycle
    esp_config->cs_ena_pretrans = 0;
    esp_config->cs_ena_posttrans = 0;
    esp_config->flags = 0;
    esp_config->queue_size = async_cfg->queue_size;
    esp_config->pre_cb = NULL;
    esp_config->post_cb = spi_async_transaction_cb;
    
    // Set SPI mode (CPOL and CPHA)
    switch (config->mode) {
        case NHAL_SPI_MODE_0:
            esp_config->mode = 0;
            break;
        case NHAL_SPI_MODE_1:
            esp_config->mode = 1;
            break;
        case NHAL_SPI_MODE_2:
            esp_config->mode = 2;
            break;
        case NHAL_SPI_MODE_3:
            esp_config->mode = 3;
            break;
        default:
            esp_config->mode = 0;
            break;
    }
    
    // Set bit order
    if (config->bit_order == NHAL_SPI_BIT_ORDER_LSB_FIRST) {
        esp_config->flags |= SPI_DEVICE_BIT_LSBFIRST;
    }
    
    // Set CS pin
    esp_config->spics_io_num = config->impl_config->cs_pin;
}

static void nhal_async_bus_config_to_esp_config(const struct nhal_spi_async_dma_config *async_cfg, spi_bus_config_t *esp_bus_config) {
    // Copy basic pin configuration from the basic config
    esp_bus_config->mosi_io_num = async_cfg->basic_config.mosi_pin;
    esp_bus_config->miso_io_num = async_cfg->basic_config.miso_pin;
    esp_bus_config->sclk_io_num = async_cfg->basic_config.sclk_pin;
    esp_bus_config->quadwp_io_num = -1;
    esp_bus_config->quadhd_io_num = -1;
    esp_bus_config->max_transfer_sz = async_cfg->max_transfer_sz;
    esp_bus_config->flags = SPICOMMON_BUSFLAG_MASTER;
}

nhal_result_t nhal_spi_enable_async_mode(
    struct nhal_spi_context * spi_ctx,
    const struct nhal_spi_async_dma_config *async_cfg
) {
    if (spi_ctx == NULL || async_cfg == NULL || spi_ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (!spi_ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    BaseType_t mutex_ret_err = xSemaphoreTake(spi_ctx->impl_ctx->mutex, pdMS_TO_TICKS(1000));
    if (mutex_ret_err == pdTRUE) {
        esp_err_t ret_err;
        nhal_result_t spi_result = NHAL_OK;
        spi_bus_config_t esp_bus_config = {0};
        spi_device_interface_config_t esp_device_config = {0};
        
        // Configure for async DMA operations
        nhal_async_bus_config_to_esp_config(async_cfg, &esp_bus_config);
        
        // SPI bus must already be initialized and configured
        if (!spi_ctx->impl_ctx->is_driver_installed) {
            spi_result = NHAL_ERR_INVALID_ARG;
            goto free_mutex_and_ret;
        }
        
        // Update mode to sync-and-async
        spi_ctx->current_mode = NHAL_SPI_OP_MODE_SYNC_AND_ASYNC;
        
        // Store async configuration in context extension
        spi_ctx->async_dma.max_transfer_sz = async_cfg->max_transfer_sz;
        spi_ctx->async_dma.dma_chan = async_cfg->dma_chan;
        spi_ctx->async_dma.queue_size = async_cfg->queue_size;
        spi_ctx->async_dma.tx_operations_queued = 0;
        spi_ctx->async_dma.rx_operations_available = 0;
        spi_ctx->async_dma.is_async_initialized = true;
        
        free_mutex_and_ret:
            xSemaphoreGive(spi_ctx->impl_ctx->mutex);
            return spi_result;
    } else {
        return NHAL_ERR_BUSY;
    }
}

nhal_result_t nhal_spi_disable_async_mode(
    struct nhal_spi_context * spi_ctx
) {
    if (spi_ctx == NULL || spi_ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    BaseType_t mutex_ret_err = xSemaphoreTake(spi_ctx->impl_ctx->mutex, pdMS_TO_TICKS(1000));
    if (mutex_ret_err == pdTRUE) {
        // Remove async device if present
        if (spi_ctx->impl_ctx->async_device_handle != NULL) {
            esp_err_t ret_err = spi_bus_remove_device(spi_ctx->impl_ctx->async_device_handle);
            if (ret_err != ESP_OK) {
                xSemaphoreGive(spi_ctx->impl_ctx->mutex);
                return nhal_map_esp_err(ret_err);
            }
            spi_ctx->impl_ctx->async_device_handle = NULL;
        }
        
        // Revert to sync-only mode
        spi_ctx->current_mode = NHAL_SPI_OP_MODE_SYNC_ONLY;
        spi_ctx->async_dma.is_async_initialized = false;
        
        xSemaphoreGive(spi_ctx->impl_ctx->mutex);
        return NHAL_OK;
    } else {
        return NHAL_ERR_BUSY;
    }
}

nhal_result_t nhal_spi_write_async(
    struct nhal_spi_context * spi_ctx,
    const uint8_t * data, size_t len,
    nhal_timeout_ms timeout,
    nhal_spi_async_complete_cb_t callback,
    void * callback_ctx
) {
    if (spi_ctx == NULL || data == NULL || len == 0 || callback == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (spi_ctx->current_mode != NHAL_SPI_OP_MODE_SYNC_AND_ASYNC || !spi_ctx->async_dma.is_async_initialized) {
        return NHAL_ERR_INVALID_ARG; // Async mode not enabled or not initialized
    }
    
    // Allocate transaction structure
    spi_transaction_t *trans = heap_caps_malloc(sizeof(spi_transaction_t), MALLOC_CAP_DMA);
    if (trans == NULL) {
        return NHAL_ERR_OTHER;
    }
    
    nhal_spi_async_transaction_t *async_trans = malloc(sizeof(nhal_spi_async_transaction_t));
    if (async_trans == NULL) {
        free(trans);
        return NHAL_ERR_OTHER;
    }
    
    // Setup transaction
    memset(trans, 0, sizeof(spi_transaction_t));
    trans->length = len * 8; // Length in bits
    trans->tx_buffer = data;
    trans->rx_buffer = NULL;
    trans->user = async_trans;
    
    // Setup async tracking
    async_trans->callback = callback;
    async_trans->callback_ctx = callback_ctx;
    async_trans->spi_ctx = spi_ctx;
    
    // Queue the transaction
    esp_err_t ret_err = spi_device_queue_trans(spi_ctx->impl_ctx->async_device_handle, trans, pdMS_TO_TICKS(timeout));
    if (ret_err != ESP_OK) {
        free(async_trans);
        free(trans);
        return nhal_map_esp_err(ret_err);
    }
    
    // Update queue count
    spi_ctx->async_dma.tx_operations_queued++;
    
    return NHAL_OK;
}

nhal_result_t nhal_spi_read_async(
    struct nhal_spi_context * spi_ctx,
    uint8_t * data, size_t len,
    nhal_timeout_ms timeout,
    nhal_spi_async_complete_cb_t callback,
    void * callback_ctx
) {
    if (spi_ctx == NULL || data == NULL || len == 0 || callback == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (spi_ctx->current_mode != NHAL_SPI_OP_MODE_SYNC_AND_ASYNC || !spi_ctx->async_dma.is_async_initialized) {
        return NHAL_ERR_INVALID_ARG; // Async mode not enabled or not initialized
    }
    
    // Allocate transaction structure
    spi_transaction_t *trans = heap_caps_malloc(sizeof(spi_transaction_t), MALLOC_CAP_DMA);
    if (trans == NULL) {
        return NHAL_ERR_OTHER;
    }
    
    nhal_spi_async_transaction_t *async_trans = malloc(sizeof(nhal_spi_async_transaction_t));
    if (async_trans == NULL) {
        free(trans);
        return NHAL_ERR_OTHER;
    }
    
    // Setup transaction
    memset(trans, 0, sizeof(spi_transaction_t));
    trans->length = len * 8; // Length in bits
    trans->tx_buffer = NULL;
    trans->rx_buffer = data;
    trans->user = async_trans;
    
    // Setup async tracking
    async_trans->callback = callback;
    async_trans->callback_ctx = callback_ctx;
    async_trans->spi_ctx = spi_ctx;
    
    // Queue the transaction
    esp_err_t ret_err = spi_device_queue_trans(spi_ctx->impl_ctx->async_device_handle, trans, pdMS_TO_TICKS(timeout));
    if (ret_err != ESP_OK) {
        free(async_trans);
        free(trans);
        return nhal_map_esp_err(ret_err);
    }
    
    return NHAL_OK;
}

nhal_result_t nhal_spi_write_read_async(
    struct nhal_spi_context * spi_ctx,
    const uint8_t * tx_data, size_t tx_len,
    uint8_t * rx_data, size_t rx_len,
    nhal_timeout_ms timeout,
    nhal_spi_async_complete_cb_t callback,
    void * callback_ctx
) {
    if (spi_ctx == NULL || callback == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if ((tx_data == NULL && tx_len > 0) || (rx_data == NULL && rx_len > 0)) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (spi_ctx->current_mode != NHAL_SPI_OP_MODE_SYNC_AND_ASYNC || !spi_ctx->async_dma.is_async_initialized) {
        return NHAL_ERR_INVALID_ARG; // Async mode not enabled or not initialized
    }
    
    // Allocate transaction structure
    spi_transaction_t *trans = heap_caps_malloc(sizeof(spi_transaction_t), MALLOC_CAP_DMA);
    if (trans == NULL) {
        return NHAL_ERR_OTHER;
    }
    
    nhal_spi_async_transaction_t *async_trans = malloc(sizeof(nhal_spi_async_transaction_t));
    if (async_trans == NULL) {
        free(trans);
        return NHAL_ERR_OTHER;
    }
    
    // Setup transaction
    memset(trans, 0, sizeof(spi_transaction_t));
    size_t transfer_len = (tx_len > rx_len) ? tx_len : rx_len;
    trans->length = transfer_len * 8; // Length in bits
    trans->tx_buffer = tx_data;
    trans->rx_buffer = rx_data;
    trans->user = async_trans;
    
    // Setup async tracking
    async_trans->callback = callback;
    async_trans->callback_ctx = callback_ctx;
    async_trans->spi_ctx = spi_ctx;
    
    // Queue the transaction
    esp_err_t ret_err = spi_device_queue_trans(spi_ctx->impl_ctx->async_device_handle, trans, pdMS_TO_TICKS(timeout));
    if (ret_err != ESP_OK) {
        free(async_trans);
        free(trans);
        return nhal_map_esp_err(ret_err);
    }
    
    return NHAL_OK;
}

#endif /* NHAL_SPI_ASYNC_DMA_SUPPORT */