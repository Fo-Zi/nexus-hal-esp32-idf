#include "hal_esp32_defs.h"

#include "hal_common.h"
#include "hal_i2c_types.h"
#include "hal_i2c.h"

#include "esp_err.h"
#include "driver/i2c.h"

void hal_config_to_esp_config(struct hal_i2c_config * config ,i2c_config_t * esp_config){
    esp_config->mode                = config->mode;
    esp_config->sda_io_num          = config->sda_io_num;
    esp_config->scl_io_num          = config->scl_io_num;
    esp_config->sda_pullup_en       = config->sda_pullup_en;
    esp_config->scl_pullup_en       = config->scl_pullup_en;
    esp_config->master.clk_speed    = config->clk_speed;
};

hal_i2c_result_t hal_i2c_init(struct hal_i2c_context * ctxt){
    return HAL_I2C_OK;
};

hal_i2c_result_t hal_i2c_deinit(struct hal_i2c_context *i2c_ctx){
    esp_err_t ret_err = i2c_driver_delete(i2c_ctx->i2c_id);
    if(ret_err != ESP_OK){
        return ret_err;
    }
    return HAL_I2C_OK;
};

hal_i2c_result_t hal_i2c_set_config(struct hal_i2c_context *i2c_ctx, struct hal_i2c_config *config){

    esp_err_t ret_err;
    i2c_config_t esp_config;

    hal_config_to_esp_config(config ,&esp_config);
    ret_err = i2c_param_config(config->port ,&esp_config);
    if(ret_err != ESP_OK){
        return esp_err_to_i2c_hal_err(ret_err);
    };

    ret_err = i2c_driver_install(i2c_ctx->i2c_id, config->mode, 0 , 0 , 0);
    if(ret_err != ESP_OK){
        return esp_err_to_i2c_hal_err(ret_err);
    };

    return HAL_I2C_OK;
};

hal_i2c_result_t hal_i2c_get_config(struct hal_i2c_context *i2c_ctx, struct hal_i2c_config *config){
    return HAL_I2C_ERR_OTHER;
};

hal_i2c_result_t hal_i2c_write(struct hal_i2c_context *i2c_ctx, uint8_t dev_address, const uint8_t *data, size_t len, hal_timeout_ms timeout_ms){
    return esp_err_to_i2c_hal_err(
        i2c_master_write_to_device(
            i2c_ctx->i2c_id,
            dev_address,
            data,
            len,
            pdMS_TO_TICKS(timeout_ms)
        )
    );
};

hal_i2c_result_t hal_i2c_read(struct hal_i2c_context *i2c_ctx, uint8_t dev_address, uint8_t *data, size_t len, hal_timeout_ms timeout_ms){
    return esp_err_to_i2c_hal_err(
        i2c_master_read_from_device(
            i2c_ctx->i2c_id,
            dev_address,
            data,
            len,
            pdMS_TO_TICKS(timeout_ms)
        )
    );
};

hal_i2c_result_t hal_i2c_write_read_reg(
    struct hal_i2c_context *i2c_ctx,
    uint8_t dev_address,
    const uint8_t *reg_address, size_t reg_len,
    uint8_t *data, size_t data_len,
    hal_timeout_ms timeout
){
    return esp_err_to_i2c_hal_err(
        i2c_master_write_read_device(
            i2c_ctx->i2c_id,
            dev_address,
            reg_address,
            reg_len,
            data,
            data_len,
            pdMS_TO_TICKS(timeout)
        )
    );
};
