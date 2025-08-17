#include "esp_log.h"
#include "hal_esp32_defs.h"

#include "hal_common.h"
#include "hal_i2c_types.h"
#include "hal_i2c_master.h"

#include "esp_err.h"
#include "driver/i2c.h"

static void hal_config_to_esp_config(struct hal_i2c_config * config ,i2c_config_t * esp_config){
    esp_config->mode                = I2C_MODE_MASTER;
    esp_config->sda_io_num          = config->impl_config->sda_io_num;
    esp_config->scl_io_num          = config->impl_config->scl_io_num;
    esp_config->sda_pullup_en       = config->impl_config->sda_pullup_en;
    esp_config->scl_pullup_en       = config->impl_config->scl_pullup_en;
    esp_config->master.clk_speed    = config->impl_config->clk_speed;
};

hal_i2c_result_t hal_i2c_master_init(struct hal_i2c_context * ctxt){
    if (ctxt == NULL || ctxt->impl_ctx == NULL) {
        return HAL_I2C_ERR_INVALID_ARG;
    }

    if (ctxt->impl_ctx->is_initialized) {
        return HAL_I2C_OK;
    }

    // Initialize context to sync-only mode
    ctxt->current_mode = HAL_I2C_OP_MODE_SYNC_ONLY;

    // Create mutex for sync safety
    ctxt->impl_ctx->mutex = xSemaphoreCreateMutex();
    if (ctxt->impl_ctx->mutex == NULL) {
        return HAL_I2C_ERR_OTHER;
    }

    ctxt->impl_ctx->is_initialized = true;
    ctxt->impl_ctx->is_driver_installed = false;

#if defined(HAL_I2C_ASYNC_DMA_SUPPORT)
    ctxt->impl_ctx->async_cmd_handle = NULL;
#endif

    return HAL_I2C_OK;
};

hal_i2c_result_t hal_i2c_master_deinit(struct hal_i2c_context *i2c_ctx){
    if (i2c_ctx == NULL || i2c_ctx->impl_ctx == NULL) {
        return HAL_I2C_ERR_INVALID_ARG;
    }

    if (!i2c_ctx->impl_ctx->is_initialized) {
        return HAL_I2C_OK;
    }

    BaseType_t mutex_ret_err = xSemaphoreTake(i2c_ctx->impl_ctx->mutex , pdMS_TO_TICKS(1000) );
    if(mutex_ret_err == pdTRUE){
        if (i2c_ctx->impl_ctx->is_driver_installed) {
            esp_err_t ret_err = i2c_driver_delete(i2c_ctx->i2c_bus_id);
            if(ret_err != ESP_OK){
                xSemaphoreGive(i2c_ctx->impl_ctx->mutex);
                return esp_err_to_i2c_hal_err(ret_err);
            }
            i2c_ctx->impl_ctx->is_driver_installed = false;
        }

        // Clean up mutex and reset state
        SemaphoreHandle_t mutex_to_delete = i2c_ctx->impl_ctx->mutex;
        i2c_ctx->impl_ctx->is_initialized = false;
        i2c_ctx->impl_ctx->mutex = NULL;

        xSemaphoreGive(mutex_to_delete);
        vSemaphoreDelete(mutex_to_delete);

        return HAL_I2C_OK;
    }else{
        return HAL_I2C_ERR_BUS_BUSY;
    }
};

hal_i2c_result_t hal_i2c_master_set_config(struct hal_i2c_context *i2c_ctx, struct hal_i2c_config *config){

    esp_err_t ret_err;
    hal_i2c_result_t i2c_result = HAL_I2C_OK;
    i2c_config_t esp_config = {0};

    hal_config_to_esp_config(config ,&esp_config);

    BaseType_t mutex_ret_err = xSemaphoreTake(i2c_ctx->impl_ctx->mutex , pdMS_TO_TICKS(1000) );
    if(mutex_ret_err == pdTRUE){
        ret_err = i2c_param_config(config->i2c_bus_id ,&esp_config);
        if(ret_err != ESP_OK){
            i2c_result =  esp_err_to_i2c_hal_err(ret_err);
            goto free_mutex_and_ret;
        };

        ret_err = i2c_driver_install(i2c_ctx->i2c_bus_id, I2C_MODE_MASTER, 0 , 0 , 0);
        if(ret_err != ESP_OK){
            i2c_result = esp_err_to_i2c_hal_err(ret_err);
            goto free_mutex_and_ret;
        };

        i2c_ctx->impl_ctx->is_driver_installed = true;

        free_mutex_and_ret:
            xSemaphoreGive(i2c_ctx->impl_ctx->mutex);
            return i2c_result;

    }else{
        return HAL_I2C_ERR_BUS_BUSY;
    }

};

hal_i2c_result_t hal_i2c_master_get_config(struct hal_i2c_context *i2c_ctx, struct hal_i2c_config *config){
    return HAL_I2C_ERR_OTHER;   // idf doesn't support this feature
};

hal_i2c_result_t hal_i2c_master_write(struct hal_i2c_context *i2c_ctx, uint8_t dev_address, const uint8_t *data, size_t len, hal_timeout_ms timeout){

    BaseType_t mutex_ret_err = xSemaphoreTake(i2c_ctx->impl_ctx->mutex , pdMS_TO_TICKS(timeout) );
    if(mutex_ret_err == pdTRUE){
        hal_i2c_result_t i2c_result = esp_err_to_i2c_hal_err(
            i2c_master_write_to_device(
                i2c_ctx->i2c_bus_id,
                dev_address,
                data,
                len,
                pdMS_TO_TICKS(timeout)
            )
        );
        xSemaphoreGive(i2c_ctx->impl_ctx->mutex);
        return i2c_result;
    }else{
        return HAL_I2C_ERR_BUS_BUSY;
    }
};

hal_i2c_result_t hal_i2c_master_read(struct hal_i2c_context *i2c_ctx, uint8_t dev_address, uint8_t *data, size_t len, hal_timeout_ms timeout){
    BaseType_t mutex_ret_err = xSemaphoreTake(i2c_ctx->impl_ctx->mutex , pdMS_TO_TICKS(timeout) );
    if(mutex_ret_err == pdTRUE){
        hal_i2c_result_t i2c_result = esp_err_to_i2c_hal_err(
            i2c_master_read_from_device(
                i2c_ctx->i2c_bus_id,
                dev_address,
                data,
                len,
                pdMS_TO_TICKS(timeout)
            )
        );
        xSemaphoreGive(i2c_ctx->impl_ctx->mutex);
        return i2c_result;
    }else{
        return HAL_I2C_ERR_BUS_BUSY;
    }
};

hal_i2c_result_t hal_i2c_master_write_read_reg(
    struct hal_i2c_context *i2c_ctx,
    uint8_t dev_address,
    const uint8_t *reg_address, size_t reg_len,
    uint8_t *data, size_t data_len,
    hal_timeout_ms timeout
){
    BaseType_t mutex_ret_err = xSemaphoreTake(i2c_ctx->impl_ctx->mutex , pdMS_TO_TICKS(timeout) );
    if(mutex_ret_err == pdTRUE){
        hal_i2c_result_t i2c_result = esp_err_to_i2c_hal_err(
            i2c_master_write_read_device(
                i2c_ctx->i2c_bus_id,
                dev_address,
                reg_address,
                reg_len,
                data,
                data_len,
                pdMS_TO_TICKS(timeout)
            )
        );
        xSemaphoreGive(i2c_ctx->impl_ctx->mutex);
        return i2c_result;
    }else{
        return HAL_I2C_ERR_BUS_BUSY;
    }
};
