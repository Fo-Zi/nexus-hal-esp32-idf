#include "nhal_esp32_defs.h"
#include "nhal_i2c_transfer.h"

#include "driver/i2c.h"
#include "nhal_i2c_types.h"

nhal_result_t nhal_i2c_master_perform_transfer(
    struct nhal_i2c_context * ctx,
    nhal_i2c_transfer_op_t *ops,
    size_t num_ops,
    nhal_timeout_ms timeout_ms
) {
    if (ctx == NULL || ops == NULL || num_ops == 0) {
        return NHAL_ERR_INVALID_ARG;
    }

    
    BaseType_t mutex_ret_err = xSemaphoreTake(ctx->impl_ctx->mutex , pdMS_TO_TICKS(timeout_ms) );
    if(mutex_ret_err == pdTRUE){

        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        if (cmd == NULL) {
            return NHAL_ERR_OTHER;
        }

        esp_err_t ret = ESP_OK;

        for (size_t i = 0; i < num_ops; ++i) {
            nhal_i2c_transfer_op_t* op = &ops[i];

            if (!(op->flags & NHAL_I2C_TRANSFER_MSG_NO_START)) {
                ret = i2c_master_start(cmd);
                if (ret != ESP_OK) goto end_transfer;
            }

            if (!(op->flags & NHAL_I2C_TRANSFER_MSG_NO_ADDR)) {
                uint8_t addr_byte = (op->address & 0xFE);
                if (op->flags & NHAL_I2C_TRANSFER_MSG_READ) {
                    addr_byte |= I2C_MASTER_READ;
                } else {
                    addr_byte |= I2C_MASTER_WRITE;
                }

                ret = i2c_master_write_byte(cmd, addr_byte, true);
                if (ret != ESP_OK) goto end_transfer;
            }

            if (op->flags & NHAL_I2C_TRANSFER_MSG_READ) {
                if (op->read.length > 0) {
                    ret = i2c_master_read(cmd, op->read.buffer, op->read.length, I2C_MASTER_LAST_NACK);
                    if (ret != ESP_OK) goto end_transfer;
                }
            } else {
                if (op->write.length > 0) {
                    ret = i2c_master_write(cmd, op->write.bytes, op->write.length, true);
                    if (ret != ESP_OK) goto end_transfer;
                }
            }

            if (!(op->flags & NHAL_I2C_TRANSFER_MSG_NO_STOP)) {
                ret = i2c_master_stop(cmd);
                if (ret != ESP_OK) goto end_transfer;
            }
        }

        ret = i2c_master_cmd_begin(ctx->i2c_bus_id, cmd, pdMS_TO_TICKS(timeout_ms));

    end_transfer:
        i2c_cmd_link_delete(cmd);
        xSemaphoreGive(ctx->impl_ctx->mutex);
        return nhal_map_esp_err(ret);
    
    }else{
        return NHAL_ERR_TIMEOUT;
    }

};
