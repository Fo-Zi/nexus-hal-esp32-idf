#include "esp_log.h"
#include "nhal_esp32_defs.h"
#include "nhal_esp32_helpers.h"

#include "nhal_common.h"
#include "nhal_i2c_types.h"
#include "nhal_i2c_master.h"

#include "esp_err.h"

#include "driver/i2c_types.h"
#include "driver/i2c.h"
#include "driver/i2c_master.h"

static void nhal_config_to_esp_config(struct nhal_i2c_config * config ,i2c_config_t * esp_config){
    esp_config->mode                = I2C_MODE_MASTER;
    esp_config->sda_io_num          = config->impl_config->sda_io_num;
    esp_config->scl_io_num          = config->impl_config->scl_io_num;
    esp_config->sda_pullup_en       = config->impl_config->sda_pullup_en;
    esp_config->scl_pullup_en       = config->impl_config->scl_pullup_en;
    esp_config->master.clk_speed    = config->clock_speed_hz;
};

nhal_result_t nhal_i2c_master_init(struct nhal_i2c_context * ctx){
    if (ctx == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (ctx->impl_ctx->is_initialized) {
        return NHAL_OK;
    }

    // Create mutex for sync safety
    ctx->impl_ctx->mutex = xSemaphoreCreateMutex();
    if (ctx->impl_ctx->mutex == NULL) {
        return NHAL_ERR_OTHER;
    }

    ctx->impl_ctx->is_initialized = true;
    ctx->impl_ctx->is_configured = false;
    ctx->impl_ctx->is_driver_installed = false;

#if defined(NHAL_I2C_ASYNC_DMA_SUPPORT)
    ctx->impl_ctx->async_cmd_handle = NULL;
#endif

    return NHAL_OK;
};

nhal_result_t nhal_i2c_master_deinit(struct nhal_i2c_context *ctx){
    if (ctx == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_OK;
    }

    BaseType_t mutex_ret_err = xSemaphoreTake(ctx->impl_ctx->mutex , pdMS_TO_TICKS(1000) );
    if(mutex_ret_err == pdTRUE){
        if (ctx->impl_ctx->is_driver_installed) {
            esp_err_t ret_err = i2c_driver_delete(ctx->i2c_bus_id);
            if(ret_err != ESP_OK){
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
    }else{
        return NHAL_ERR_BUSY;
    }
};

nhal_result_t nhal_i2c_master_set_config(struct nhal_i2c_context *ctx, struct nhal_i2c_config *config){

    esp_err_t ret_err;
    nhal_result_t i2c_result = NHAL_OK;
    i2c_config_t esp_config = {0};

    nhal_config_to_esp_config(config ,&esp_config);

    BaseType_t mutex_ret_err = xSemaphoreTake(ctx->impl_ctx->mutex , pdMS_TO_TICKS(1000) );
    if(mutex_ret_err == pdTRUE){
        ret_err = i2c_param_config(ctx->i2c_bus_id ,&esp_config);
        if(ret_err != ESP_OK){
            i2c_result =  nhal_map_esp_err(ret_err);
            goto free_mutex_and_ret;
        };

        ret_err = i2c_driver_install(ctx->i2c_bus_id, I2C_MODE_MASTER, 0 , 0 , 0);
        if(ret_err != ESP_OK){
            i2c_result = nhal_map_esp_err(ret_err);
            goto free_mutex_and_ret;
        };

        ctx->impl_ctx->is_driver_installed = true;
        ctx->impl_ctx->is_configured = true;

        free_mutex_and_ret:
            xSemaphoreGive(ctx->impl_ctx->mutex);
            return i2c_result;

    }else{
        return NHAL_ERR_BUSY;
    }

};

nhal_result_t nhal_i2c_master_get_config(struct nhal_i2c_context *ctx, struct nhal_i2c_config *config){
    return NHAL_ERR_OTHER;   // idf doesn't support this feature
};

nhal_result_t nhal_i2c_master_write(struct nhal_i2c_context *ctx, nhal_i2c_address dev_address, const uint8_t *data, size_t len, nhal_timeout_ms timeout){

    if (ctx == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!ctx->impl_ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    BaseType_t mutex_ret_err = xSemaphoreTake(ctx->impl_ctx->mutex , pdMS_TO_TICKS(timeout) );
    if(mutex_ret_err == pdTRUE){
        nhal_result_t i2c_result = nhal_map_esp_err(
            i2c_master_write_to_device(
                ctx->i2c_bus_id,
                dev_address,
                data,
                len,
                pdMS_TO_TICKS(timeout)
            )
        );
        xSemaphoreGive(ctx->impl_ctx->mutex);
        return i2c_result;
    }else{
        return NHAL_ERR_BUSY;
    }
};

nhal_result_t nhal_i2c_master_read(struct nhal_i2c_context *ctx, nhal_i2c_address dev_address, uint8_t *data, size_t len, nhal_timeout_ms timeout){

    if (ctx == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!ctx->impl_ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    BaseType_t mutex_ret_err = xSemaphoreTake(ctx->impl_ctx->mutex , pdMS_TO_TICKS(timeout) );
    if(mutex_ret_err == pdTRUE){
        nhal_result_t i2c_result;

        if (len == 0) {
            // 0-byte reads are not supported
            i2c_result = NHAL_ERR_INVALID_ARG;
        } else {
            // Normal read operation
            i2c_result = nhal_map_esp_err(
                i2c_master_read_from_device(
                    ctx->i2c_bus_id,
                    dev_address,
                    data,
                    len,
                    pdMS_TO_TICKS(timeout)
                )
            );
        }

        xSemaphoreGive(ctx->impl_ctx->mutex);
        return i2c_result;
    }else{
        return NHAL_ERR_BUSY;
    }
};

nhal_result_t nhal_i2c_master_write_read_reg(
    struct nhal_i2c_context *ctx,
    nhal_i2c_address dev_address,
    const uint8_t *reg_address, size_t reg_len,
    uint8_t *data, size_t data_len,
    nhal_timeout_ms timeout
){
    if (ctx == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!ctx->impl_ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    BaseType_t mutex_ret_err = xSemaphoreTake(ctx->impl_ctx->mutex , pdMS_TO_TICKS(timeout) );
    if(mutex_ret_err == pdTRUE){
        nhal_result_t i2c_result = nhal_map_esp_err(
            i2c_master_write_read_device(
                ctx->i2c_bus_id,
                dev_address,
                reg_address,
                reg_len,
                data,
                data_len,
                pdMS_TO_TICKS(timeout)
            )
        );
        xSemaphoreGive(ctx->impl_ctx->mutex);
        return i2c_result;
    }else{
        return NHAL_ERR_BUSY;
    }
};
