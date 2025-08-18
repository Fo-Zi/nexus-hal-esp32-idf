#include "esp_log.h"
#include "nhal_esp32_defs.h"

#include "nhal_common.h"
#include "nhal_i2c_types.h"
#include "nhal_i2c_master_async.h"

#include "esp_err.h"
#include "driver/i2c.h"

// Only compile if async DMA support is enabled
#if defined(NHAL_I2C_ASYNC_DMA_SUPPORT)

// ESP-IDF does not provide true hardware async I2C operations
// Therefore we do not implement these functions and return appropriate errors

nhal_result_t nhal_i2c_enable_async_mode(
    struct nhal_i2c_context * i2c_ctx,
    const struct nhal_i2c_async_dma_config *async_cfg
) {
    if (i2c_ctx == NULL || async_cfg == NULL || i2c_ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if (!i2c_ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    // ESP-IDF does not support true async I2C operations
    // Return error to indicate this feature is not available
    return NHAL_ERR_OTHER;
}

nhal_result_t nhal_i2c_disable_async_mode(
    struct nhal_i2c_context * i2c_ctx
) {
    if (i2c_ctx == NULL || i2c_ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    // ESP-IDF does not support true async I2C operations
    // Return error to indicate this feature is not available
    return NHAL_ERR_OTHER;
}

nhal_result_t nhal_i2c_master_write_async(
    struct nhal_i2c_context *i2c_ctx,
    uint8_t dev_address,
    const uint8_t *data, size_t len,
    nhal_timeout_ms timeout,
    nhal_i2c_async_complete_cb_t callback,
    void * callback_ctx
) {
    if (i2c_ctx == NULL || data == NULL || len == 0 || callback == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    // ESP-IDF does not support true async I2C operations
    // Return error to indicate this feature is not available
    return NHAL_ERR_OTHER;
}

nhal_result_t nhal_i2c_master_read_async(
    struct nhal_i2c_context *i2c_ctx,
    uint8_t dev_address,
    uint8_t *data, size_t len,
    nhal_timeout_ms timeout,
    nhal_i2c_async_complete_cb_t callback,
    void * callback_ctx
) {
    if (i2c_ctx == NULL || data == NULL || len == 0 || callback == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    // ESP-IDF does not support true async I2C operations
    // Return error to indicate this feature is not available
    return NHAL_ERR_OTHER;
}

nhal_result_t nhal_i2c_master_write_read_reg_async(
    struct nhal_i2c_context *i2c_ctx,
    uint8_t dev_address,
    const uint8_t *reg_address, size_t reg_len,
    uint8_t *data, size_t data_len,
    nhal_timeout_ms timeout,
    nhal_i2c_async_complete_cb_t callback,
    void * callback_ctx
) {
    if (i2c_ctx == NULL || callback == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    if ((reg_address == NULL && reg_len > 0) || (data == NULL && data_len > 0)) {
        return NHAL_ERR_INVALID_ARG;
    }
    
    // ESP-IDF does not support true async I2C operations
    // Return error to indicate this feature is not available
    return NHAL_ERR_OTHER;
}

#endif /* NHAL_I2C_ASYNC_DMA_SUPPORT */