/**
 * @file nhal_common.c
 * @brief Implementation of common NHAL functions
 */

#include "nhal_common.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

// ESP-IDF version compatibility check
#ifdef ESP_IDF_VERSION_MAJOR
    #if ESP_IDF_VERSION_MAJOR < 5 || (ESP_IDF_VERSION_MAJOR == 5 && ESP_IDF_VERSION_MINOR < 5)
        #error "NHAL implementation requires ESP-IDF v5.5 or newer"
    #endif
#else
    #warning "ESP-IDF version cannot be determined - compatibility not guaranteed"
#endif

void nhal_delay_microseconds(uint32_t microseconds)
{
    if (microseconds == 0) {
        return;
    }
    
    // Use ESP-IDF ROM function for precise microsecond delays
    esp_rom_delay_us(microseconds);
}

void nhal_delay_milliseconds(uint32_t milliseconds)
{
    if (milliseconds == 0) {
        return;
    }
    
    TickType_t ticks = pdMS_TO_TICKS(milliseconds);
    if (ticks > 0) {
        vTaskDelay(ticks);
    } else {
        // If less than 1 tick, delay at least 1 tick
        vTaskDelay(1);
    }
}

nhal_result_t nhal_map_esp_err(int esp_err_code)
{
    switch (esp_err_code) {
        case ESP_OK:
            return NHAL_OK;
        case ESP_ERR_TIMEOUT:
            return NHAL_ERR_TIMEOUT;
        case ESP_ERR_INVALID_ARG:
            return NHAL_ERR_INVALID_ARG;
        case ESP_ERR_INVALID_STATE:
            return NHAL_ERR_INVALID_STATE;
        case ESP_ERR_NOT_FOUND:
            return NHAL_ERR_NOT_FOUND;
        case ESP_ERR_NOT_SUPPORTED:
            return NHAL_ERR_NOT_SUPPORTED;
        case ESP_ERR_NO_MEM:
            return NHAL_ERR_NO_MEMORY;
        case ESP_ERR_INVALID_SIZE:
            return NHAL_ERR_OTHER;
        case ESP_ERR_INVALID_CRC:
            return NHAL_ERR_OTHER;
        case ESP_ERR_INVALID_VERSION:
            return NHAL_ERR_OTHER;
        case ESP_ERR_INVALID_MAC:
            return NHAL_ERR_OTHER;
        case ESP_ERR_INVALID_RESPONSE:
            return NHAL_ERR_COMMUNICATION;
        default:
            return NHAL_ERR_OTHER;
    }
}