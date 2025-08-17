#include "hal_esp32_defs.h"

hal_i2c_result_t esp_err_to_i2c_hal_err(esp_err_t esp_err){
    switch (esp_err) {
        case ESP_OK:
            return HAL_I2C_OK;
        default:
            return HAL_I2C_ERR_OTHER;
    }
};

hal_uart_result_t esp_err_to_uart_hal_err(esp_err_t esp_err){
    switch (esp_err) {
        case ESP_OK:
            return HAL_UART_OK;
        default:
            return HAL_UART_ERR_OTHER;
    }
};

hal_pin_result_t esp_err_to_pin_hal_err(esp_err_t esp_err){
    switch (esp_err) {
        case ESP_OK:
            return HAL_PIN_OK;
        default:
            return HAL_PIN_ERR_OTHER;
    }
};

hal_spi_result_t esp_err_to_spi_hal_err(esp_err_t esp_err){
    switch (esp_err) {
        case ESP_OK:
            return HAL_SPI_OK;
        case ESP_ERR_TIMEOUT:
            return HAL_SPI_ERR_TIMEOUT;
        case ESP_ERR_INVALID_ARG:
            return HAL_SPI_ERR_INVALID_ARG;
        case ESP_ERR_INVALID_STATE:
            return HAL_SPI_ERR_BUS_BUSY;
        default:
            return HAL_SPI_ERR_OTHER;
    }
};
