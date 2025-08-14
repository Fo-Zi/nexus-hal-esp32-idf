#ifndef HAL_IMPL_ESP32_DEFS_H
#define HAL_IMPL_ESP32_DEFS_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#include "hal_i2c_types.h"
#include "hal_uart_types.h"
#include "hal_pin_types.h"

//==============================================================================
// PLATFORM-SPECIFIC CONFIGURATION STRUCTURES
//==============================================================================
// The application must create and populate these structures and pass them to
// the HAL via the `impl_specific` pointers in the generic config structs.

struct hal_i2c_config{
    uint8_t     port            ;
    uint8_t     slave_addr      ;
    uint8_t     mode            ;
    uint8_t     sda_io_num      ;
    uint8_t     scl_io_num      ;
    uint8_t     sda_pullup_en   ;
    uint8_t     scl_pullup_en   ;
    uint16_t    clk_speed       ;
} ;

struct hal_uart_config{
    uint8_t port            ;
    uint8_t tx_pin_number   ;
    uint8_t rx_pin_number   ;
    uint8_t tx_buffer_size  ;
    uint8_t rx_buffer_size  ;
    uint8_t rts_pin_number  ;
    uint8_t cts_pin_number  ;
    uint8_t baud_rate       ;
    uint8_t data_bits       ;
    uint8_t parity          ;
    uint8_t stop_bits       ;
    uint8_t flow_ctrl       ;
    uint8_t source_clk      ;
    uint8_t intr_alloc_flags;
    uint8_t queue_len       ;
    uint8_t queue_msg_size  ;
} ;

struct hal_pin_config{
    uint8_t pin_num         ;
    uint8_t intr_type       ;
    uint8_t mode            ;
    uint8_t pull_up_en      ;
    uint8_t pull_down_en    ;
} ;

//==============================================================================
// INTERNAL IMPLEMENTATION CONTEXT STRUCTURES
//==============================================================================
// These structures are allocated and managed by the HAL `_init` functions.
// The application does not need to interact with them directly.

struct hal_pin_context{
    uint8_t pin_num;
    bool is_initialized;
} ;

struct hal_i2c_context{
    uint8_t i2c_id;
    bool is_initialized;
    bool is_driver_installed;
} ;

struct hal_uart_context{
    uint8_t uart_id;
    bool is_initialized;
    bool is_driver_installed;
};

hal_i2c_result_t esp_err_to_i2c_hal_err(esp_err_t esp_err);
hal_uart_result_t esp_err_to_uart_hal_err(esp_err_t esp_err);
hal_pin_result_t esp_err_to_pin_hal_err(esp_err_t esp_err);

#endif // HAL_ESP32_SPECIFICS_H
