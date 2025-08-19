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
    #if ESP_IDF_VERSION_MAJOR < 5
        #error "NHAL implementation requires ESP-IDF v5.0 or newer"
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
