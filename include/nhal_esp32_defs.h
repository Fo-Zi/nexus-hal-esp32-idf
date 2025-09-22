/**
 * @file nhal_impl_esp32_defs.h
 * @brief This is a private header file that defines the ESP32-specific
 * structures and functions for the HAL implementation. It should not be
 * included directly by higher-level application code.
 */
#ifndef NHAL_IMPL_ESP32_DEFS_H
#define NHAL_IMPL_ESP32_DEFS_H

// C std Includes ->
#include <stdbool.h>
#include <stdint.h>

// NHAL Includes ->
#include "nhal_i2c_types.h"
#include "nhal_uart_types.h"
#include "nhal_pin_types.h"
#include "nhal_spi_types.h"

// ESP32 Includes ->
#include "freertos/idf_additions.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "hal/uart_types.h"
#include "esp_err.h"


//==============================================================================
// PLATFORM-SPECIFIC CONFIGURATION STRUCTURES
//==============================================================================
// The application must create and populate these structures and pass them to
// the HAL via the `impl_specific` pointers in the generic config structs.

struct nhal_i2c_impl_config{
    uint8_t     mode            ;
    uint8_t     sda_io_num      ;
    uint8_t     scl_io_num      ;
    uint8_t     sda_pullup_en   ;
    uint8_t     scl_pullup_en   ;
    uint32_t    clock_speed_hz  ;
    nhal_timeout_ms timeout_ms  ;
} ;

struct nhal_uart_impl_config{
    uint8_t tx_pin_number   ;
    uint8_t rx_pin_number   ;
    uint16_t tx_buffer_size ;
    uint16_t rx_buffer_size ;
    uint8_t rts_pin_number  ;
    uint8_t cts_pin_number  ;
    uint8_t flow_ctrl       ;
    uint8_t source_clk      ;
    uint8_t intr_alloc_flags;
    uint8_t queue_size      ;
    uint8_t queue_msg_size  ;
} ;

struct nhal_pin_impl_config{
    uint8_t intr_type       ;
} ;

struct nhal_spi_impl_config{
    uint8_t mosi_pin        ;
    uint8_t miso_pin        ;
    uint8_t sclk_pin        ;
    uint8_t cs_pin          ;
    uint32_t frequency_hz   ;
    nhal_timeout_ms timeout_ms ;
} ;

//==============================================================================
// CONCRETE CONTEXT STRUCTURE DEFINITIONS
//==============================================================================
// These provide the concrete definitions for the forward-declared context
// structures in the interface headers, with all ESP32-specific implementation
// data included directly.

struct nhal_pin_context {
    gpio_num_t pin_num;
    bool is_initialized;
    bool is_configured;
    nhal_pin_callback_t user_callback;
};

struct nhal_i2c_context {
    i2c_port_t i2c_bus_id;
    bool is_initialized;
    bool is_configured;
    bool is_driver_installed;
    SemaphoreHandle_t mutex;
    nhal_timeout_ms timeout_ms;
};

struct nhal_uart_context {
    uart_port_t uart_bus_id;
    bool is_initialized;
    bool is_configured;
    bool is_driver_installed;
    SemaphoreHandle_t mutex;
    nhal_timeout_ms timeout_ms;
};

struct nhal_spi_context {
    spi_host_device_t spi_bus_id;
    bool is_initialized;
    bool is_configured;
    spi_device_handle_t device_handle;
    SemaphoreHandle_t mutex;
    nhal_timeout_ms timeout_ms;
};

#endif // NHAL_IMPL_ESP32_DEFS_H
