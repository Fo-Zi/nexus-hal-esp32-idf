#include "esp_log.h"
#include "nhal_esp32_defs.h"
#include "nhal_esp32_helpers.h"

#include "nhal_common.h"
#include "nhal_spi_types.h"
#include "nhal_spi_master.h"

#include "esp_err.h"
#include "driver/spi_master.h"

static void nhal_config_to_esp_config(struct nhal_spi_config *config, spi_device_interface_config_t *esp_config) {
    esp_config->command_bits = 0;
    esp_config->address_bits = 0;
    esp_config->dummy_bits = 0;
    esp_config->clock_speed_hz = config->frequency_hz;
    esp_config->duty_cycle_pos = 128; // 50% duty cycle
    esp_config->cs_ena_pretrans = 0;
    esp_config->cs_ena_posttrans = 0;
    esp_config->flags = 0;
    esp_config->queue_size = 1; // Basic mode uses minimal queue
    esp_config->pre_cb = NULL;
    esp_config->post_cb = NULL;

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

static void nhal_bus_config_to_esp_config(struct nhal_spi_config *config, spi_bus_config_t *esp_bus_config) {
    esp_bus_config->mosi_io_num = config->impl_config->mosi_pin;
    esp_bus_config->miso_io_num = config->impl_config->miso_pin;
    esp_bus_config->sclk_io_num = config->impl_config->sclk_pin;
    esp_bus_config->quadwp_io_num = -1;
    esp_bus_config->quadhd_io_num = -1;
    esp_bus_config->max_transfer_sz = 0; // Use default for basic mode (no DMA)
    esp_bus_config->flags = SPICOMMON_BUSFLAG_MASTER;
}

nhal_result_t nhal_spi_master_init(struct nhal_spi_context *ctx) {
    if (ctx == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (ctx->impl_ctx->is_initialized) {
        return NHAL_OK;
    }

    // Create mutex for thread safety
    ctx->impl_ctx->mutex = xSemaphoreCreateMutex();
    if (ctx->impl_ctx->mutex == NULL) {
        return NHAL_ERR_OTHER;
    }

    ctx->impl_ctx->is_initialized = true;
    ctx->impl_ctx->is_configured = false;
    ctx->impl_ctx->is_driver_installed = false;
    ctx->impl_ctx->device_handle = NULL;

#if defined(NHAL_SPI_ASYNC_DMA_SUPPORT)
    ctx->impl_ctx->async_device_handle = NULL;
#endif

    return NHAL_OK;
}

nhal_result_t nhal_spi_master_deinit(struct nhal_spi_context *ctx) {
    if (ctx == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_OK;
    }

    BaseType_t mutex_ret_err = xSemaphoreTake(ctx->impl_ctx->mutex, pdMS_TO_TICKS(1000));
    if (mutex_ret_err == pdTRUE) {
        // Remove device if attached
        if (ctx->impl_ctx->device_handle != NULL) {
            esp_err_t ret_err = spi_bus_remove_device(ctx->impl_ctx->device_handle);
            if (ret_err != ESP_OK) {
                xSemaphoreGive(ctx->impl_ctx->mutex);
                return nhal_map_esp_err(ret_err);
            }
            ctx->impl_ctx->device_handle = NULL;
        }

        // Free SPI bus if installed
        if (ctx->impl_ctx->is_driver_installed) {
            esp_err_t ret_err = spi_bus_free(ctx->spi_bus_id);
            if (ret_err != ESP_OK) {
                xSemaphoreGive(ctx->impl_ctx->mutex);
                return nhal_map_esp_err(ret_err);
            }
            ctx->impl_ctx->is_driver_installed = false;
        }

        // Clean up mutex and reset state
        SemaphoreHandle_t mutex_to_delete = ctx->impl_ctx->mutex;
        ctx->impl_ctx->is_initialized = false;
        ctx->impl_ctx->mutex = NULL;

        xSemaphoreGive(mutex_to_delete);
        vSemaphoreDelete(mutex_to_delete);

        return NHAL_OK;
    } else {
        return NHAL_ERR_BUSY;
    }
}

nhal_result_t nhal_spi_master_set_config(struct nhal_spi_context *ctx, struct nhal_spi_config *config) {
    if (ctx == NULL || config == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    esp_err_t ret_err;
    nhal_result_t spi_result = NHAL_OK;
    spi_bus_config_t esp_bus_config = {0};
    spi_device_interface_config_t esp_device_config = {0};

    nhal_bus_config_to_esp_config(config, &esp_bus_config);
    nhal_config_to_esp_config(config, &esp_device_config);

    BaseType_t mutex_ret_err = xSemaphoreTake(ctx->impl_ctx->mutex, pdMS_TO_TICKS(1000));
    if (mutex_ret_err == pdTRUE) {
        // Initialize SPI bus if not already done
        if (!ctx->impl_ctx->is_driver_installed) {
            ret_err = spi_bus_initialize(ctx->spi_bus_id, &esp_bus_config, SPI_DMA_DISABLED);
            if (ret_err != ESP_OK) {
                spi_result = nhal_map_esp_err(ret_err);
                goto free_mutex_and_ret;
            }
            ctx->impl_ctx->is_driver_installed = true;
        }

        // Add device to SPI bus
        ret_err = spi_bus_add_device(ctx->spi_bus_id, &esp_device_config, &ctx->impl_ctx->device_handle);
        if (ret_err != ESP_OK) {
            spi_result = nhal_map_esp_err(ret_err);
            goto free_mutex_and_ret;
        }

        // Store actual frequency (ESP-IDF may adjust it)
        ctx->actual_frequency_hz = config->frequency_hz; // ESP-IDF will adjust to closest supported
        ctx->impl_ctx->is_configured = true;

        free_mutex_and_ret:
            xSemaphoreGive(ctx->impl_ctx->mutex);
            return spi_result;
    } else {
        return NHAL_ERR_BUSY;
    }
}

nhal_result_t nhal_spi_master_get_config(struct nhal_spi_context *ctx, struct nhal_spi_config *config) {
    return NHAL_ERR_OTHER; // ESP-IDF doesn't provide a way to get configuration
}

nhal_result_t nhal_spi_master_write(struct nhal_spi_context *ctx, const uint8_t *data, size_t len, nhal_timeout_ms timeout) {
    if (ctx == NULL || data == NULL || len == 0 || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!ctx->impl_ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    BaseType_t mutex_ret_err = xSemaphoreTake(ctx->impl_ctx->mutex, pdMS_TO_TICKS(timeout));
    if (mutex_ret_err == pdTRUE) {
        spi_transaction_t trans = {0};
        trans.length = len * 8; // Length in bits
        trans.tx_buffer = data;
        trans.rx_buffer = NULL;

        // Transparent optimization: For sync-and-async mode with DMA support,
        // automatically use DMA for larger transfers, CPU for smaller ones
        spi_device_handle_t handle_to_use = ctx->impl_ctx->device_handle;

#if defined(NHAL_SPI_ASYNC_SUPPORT)
        if (ctx->impl_ctx->async_device_handle != NULL &&
            len >= 64) { // Use DMA for transfers >= 64 bytes
            handle_to_use = ctx->impl_ctx->async_device_handle;
        }
#endif

        nhal_result_t spi_result = nhal_map_esp_err(
            spi_device_transmit(handle_to_use, &trans)
        );

        xSemaphoreGive(ctx->impl_ctx->mutex);
        return spi_result;
    } else {
        return NHAL_ERR_BUSY;
    }
}

nhal_result_t nhal_spi_master_read(struct nhal_spi_context *ctx, uint8_t *data, size_t len, nhal_timeout_ms timeout) {
    if (ctx == NULL || data == NULL || len == 0 || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!ctx->impl_ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    BaseType_t mutex_ret_err = xSemaphoreTake(ctx->impl_ctx->mutex, pdMS_TO_TICKS(timeout));
    if (mutex_ret_err == pdTRUE) {
        spi_transaction_t trans = {0};
        trans.length = len * 8; // Length in bits
        trans.tx_buffer = NULL;
        trans.rx_buffer = data;

        // Transparent optimization: Choose DMA vs CPU based on transfer size
        spi_device_handle_t handle_to_use = ctx->impl_ctx->device_handle;

#if defined(NHAL_SPI_ASYNC_SUPPORT)
        if (ctx->impl_ctx->async_device_handle != NULL &&
            len >= 64) { // Use DMA for transfers >= 64 bytes
            handle_to_use = ctx->impl_ctx->async_device_handle;
        }
#endif

        nhal_result_t spi_result = nhal_map_esp_err(
            spi_device_transmit(handle_to_use, &trans)
        );

        xSemaphoreGive(ctx->impl_ctx->mutex);
        return spi_result;
    } else {
        return NHAL_ERR_BUSY;
    }
}

nhal_result_t nhal_spi_master_write_read(struct nhal_spi_context *ctx, const uint8_t *tx_data, size_t tx_len, uint8_t *rx_data, size_t rx_len, nhal_timeout_ms timeout) {
    if (ctx == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if ((tx_data == NULL && tx_len > 0) || (rx_data == NULL && rx_len > 0)) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!ctx->impl_ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    BaseType_t mutex_ret_err = xSemaphoreTake(ctx->impl_ctx->mutex, pdMS_TO_TICKS(timeout));
    if (mutex_ret_err == pdTRUE) {
        spi_transaction_t trans = {0};

        // For simultaneous write/read, lengths must match in ESP-IDF
        size_t transfer_len = (tx_len > rx_len) ? tx_len : rx_len;
        trans.length = transfer_len * 8; // Length in bits
        trans.tx_buffer = tx_data;
        trans.rx_buffer = rx_data;

        // Transparent optimization: Choose DMA vs CPU based on transfer size
        spi_device_handle_t handle_to_use = ctx->impl_ctx->device_handle;

#if defined(NHAL_SPI_ASYNC_SUPPORT)
        if (ctx->impl_ctx->async_device_handle != NULL &&
            transfer_len >= 64) { // Use DMA for transfers >= 64 bytes
            handle_to_use = ctx->impl_ctx->async_device_handle;
        }
#endif

        nhal_result_t spi_result = nhal_map_esp_err(
            spi_device_transmit(handle_to_use, &trans)
        );

        xSemaphoreGive(ctx->impl_ctx->mutex);
        return spi_result;
    } else {
        return NHAL_ERR_BUSY;
    }
}
