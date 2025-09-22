#include "nhal_esp32_defs.h"
#include "nhal_esp32_helpers.h"
#include "driver/gpio.h"

nhal_result_t nhal_map_esp_err(esp_err_t esp_err){
    switch (esp_err) {
        case ESP_OK:
            return NHAL_OK;
        case ESP_ERR_TIMEOUT:
            return NHAL_ERR_TIMEOUT;
        case ESP_ERR_INVALID_ARG:
            return NHAL_ERR_INVALID_ARG;
        case ESP_ERR_NOT_SUPPORTED:
            return NHAL_ERR_UNSUPPORTED;
        case ESP_ERR_NO_MEM:
            return NHAL_ERR_OUT_OF_MEMORY;
        default:
            return NHAL_ERR_OTHER;
    }
};

nhal_pin_pull_mode_t esp32_to_nhal_pin_pull_mode(uint8_t pullup_en, uint8_t pulldown_en) {
    if (pullup_en) {
        return NHAL_PIN_PMODE_PULL_UP;
    } else if (pulldown_en) {
        return NHAL_PIN_PMODE_PULL_DOWN;
    } else {
        return NHAL_PIN_PMODE_NONE;
    }
}

void nhal_to_esp32_pull_mode(nhal_pin_pull_mode_t pull_mode, gpio_pullup_t *pullup_en, gpio_pulldown_t *pulldown_en) {
    switch (pull_mode) {
        case NHAL_PIN_PMODE_PULL_UP:
            *pullup_en = GPIO_PULLUP_ENABLE;
            *pulldown_en = GPIO_PULLDOWN_DISABLE;
            break;
        case NHAL_PIN_PMODE_PULL_DOWN:
            *pullup_en = GPIO_PULLUP_DISABLE;
            *pulldown_en = GPIO_PULLDOWN_ENABLE;
            break;
        case NHAL_PIN_PMODE_NONE:
            *pullup_en = GPIO_PULLUP_DISABLE;
            *pulldown_en = GPIO_PULLDOWN_DISABLE;
            break;
        default:
            *pullup_en = GPIO_PULLUP_DISABLE;
            *pulldown_en = GPIO_PULLDOWN_DISABLE;
            break;
    }
}

nhal_result_t nhal_i2c_address_to_esp(nhal_i2c_address_t addr, uint8_t *esp_addr) {
    if (addr.type == NHAL_I2C_7BIT_ADDR) {
        *esp_addr = addr.addr.address_7bit;
        return NHAL_OK;
    } else if (addr.type == NHAL_I2C_10BIT_ADDR) {
        // ESP32 I2C driver doesn't support 10-bit addressing
        return NHAL_ERR_UNSUPPORTED;
    } else {
        return NHAL_ERR_INVALID_ARG;
    }
}
