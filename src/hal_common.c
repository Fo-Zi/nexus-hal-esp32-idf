/**
 * @file hal_common.c
 * @brief Implementation of common HAL functions
 */

#include "hal_common.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP-IDF version compatibility check
#ifdef ESP_IDF_VERSION_MAJOR
    #if ESP_IDF_VERSION_MAJOR < 5 || (ESP_IDF_VERSION_MAJOR == 5 && ESP_IDF_VERSION_MINOR < 5)
        #error "HAL implementation requires ESP-IDF v5.5 or newer"
    #endif
#else
    #warning "ESP-IDF version cannot be determined - compatibility not guaranteed"
#endif

void hal_delay_microseconds(uint32_t microseconds)
{
    if (microseconds == 0) {
        return;
    }
    
    // Use ESP-IDF ROM function for precise microsecond delays
    esp_rom_delay_us(microseconds);
}

void hal_delay_milliseconds(uint32_t milliseconds)
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