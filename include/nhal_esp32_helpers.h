#ifndef NHAL_ESP32_HELPERS_H
#define NHAL_ESP32_HELPERS_H

#include "esp_err.h"
#include "nhal_pin_types.h"
#include "nhal_i2c_types.h"
#include "driver/gpio.h"

nhal_result_t nhal_map_esp_err(esp_err_t esp_err);
nhal_pin_pull_mode_t esp32_to_nhal_pin_pull_mode(uint8_t pullup_en,uint8_t pulldown_en);
void nhal_to_esp32_pull_mode(nhal_pin_pull_mode_t pull_mode, gpio_pullup_t *pullup_en, gpio_pulldown_t *pulldown_en);
nhal_result_t nhal_i2c_address_to_esp(nhal_i2c_address_t addr, uint8_t *esp_addr);

#endif
