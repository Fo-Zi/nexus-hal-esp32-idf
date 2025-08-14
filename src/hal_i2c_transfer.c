#include "hal_esp32_defs.h"
#include "hal_i2c_transfer.h"

#include "driver/i2c.h"

hal_i2c_result_t hal_i2c_perform_transfer(
    struct hal_i2c_context * ctx,
    hal_i2c_transfer_op_t *ops,
    size_t num_ops,
    hal_timeout_ms timeout_ms
) {
    if (ctx == NULL || ops == NULL || num_ops == 0) {
        return HAL_I2C_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        return HAL_I2C_ERR_OTHER;
    }

    esp_err_t ret = ESP_OK;

    for (size_t i = 0; i < num_ops; ++i) {
        hal_i2c_transfer_op_t* op = &ops[i];

        if (!(op->flags & HAL_I2C_TRANSFER_MSG_NO_START)) {
            ret = i2c_master_start(cmd);
            if (ret != ESP_OK) goto end_transfer;
        }

        uint8_t addr_byte = (op->address << 1);
        if (op->flags & HAL_I2C_TRANSFER_MSG_READ) {
            addr_byte |= I2C_MASTER_READ;
        } else {
            addr_byte |= I2C_MASTER_WRITE;
        }

        ret = i2c_master_write_byte(cmd, addr_byte, true);
        if (ret != ESP_OK) goto end_transfer;

        if (op->flags & HAL_I2C_TRANSFER_MSG_READ) {
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
    }

    if (!(ops[num_ops - 1].flags & HAL_I2C_TRANSFER_MSG_NO_STOP)) {
        ret = i2c_master_stop(cmd);
        if (ret != ESP_OK) goto end_transfer;
    }

    ret = i2c_master_cmd_begin(ctx->i2c_id, cmd, pdMS_TO_TICKS(timeout_ms));

end_transfer:
    i2c_cmd_link_delete(cmd);
    return esp_err_to_i2c_hal_err(ret);

}
