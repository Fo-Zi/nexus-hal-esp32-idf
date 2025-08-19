/**
 * @file nhal_impl_esp32_defs.h
 * @brief This is a private header file that defines the ESP32-specific
 * structures and functions for the HAL implementation. It should not be
 * included directly by higher-level application code.
 */
#ifndef NHAL_IMPL_ESP32_DEFS_H
#define NHAL_IMPL_ESP32_DEFS_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#include "freertos/idf_additions.h"
#include "driver/spi_master.h"
#include "nhal_i2c_types.h"
#include "nhal_uart_types.h"
#include "nhal_pin_types.h"
#include "nhal_spi_types.h"
#include "nhal_wdt_types.h"

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
    uint32_t    clk_speed       ;
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

struct nhal_uart_async_buffered_impl_config{
    struct nhal_uart_impl_config basic_config;
    uint16_t tx_buffer_size;
    uint16_t rx_buffer_size;
    uint8_t queue_size;
} ;
struct nhal_pin_id{
    uint8_t num;
};

struct nhal_pin_impl_config{
    uint8_t intr_type       ;
    uint8_t pull_up_en      ;
    uint8_t pull_down_en    ;
} ;

struct nhal_spi_impl_config{
    uint8_t mosi_pin        ;
    uint8_t miso_pin        ;
    uint8_t sclk_pin        ;
    uint8_t cs_pin          ;
} ;

struct nhal_spi_async_dma_impl_config{
    struct nhal_spi_impl_config basic_config;
    uint16_t max_transfer_sz ;
    uint8_t dma_chan        ;
    uint8_t queue_size      ;
} ;

struct nhal_wdt_impl_config{
    bool panic_handler;      /**< Enable panic handler on timeout */
    bool trigger_abort;      /**< Trigger abort() on timeout */
    uint8_t idle_core_mask;  /**< Core mask for idle task monitoring */
} ;

//==============================================================================
// INTERNAL IMPLEMENTATION CONTEXT STRUCTURES
//==============================================================================
// These structures are allocated and managed by the HAL `_init` functions.
// The application does not need to interact with them directly.

struct nhal_pin_impl_ctx{
    bool is_initialized;
    bool is_configured;
} ;

struct nhal_i2c_impl_ctx{
    SemaphoreHandle_t mutex;
    bool is_initialized;
    bool is_configured;
    bool is_driver_installed;
    
#if defined(NHAL_I2C_ASYNC_DMA_SUPPORT)
    i2c_cmd_handle_t async_cmd_handle;
#endif
} ;

struct nhal_uart_impl_ctx{
    SemaphoreHandle_t mutex;
    bool is_initialized;
    bool is_configured;
    bool is_driver_installed;
    
#if defined(NHAL_UART_ASYNC_BUFFERED_SUPPORT)
    QueueHandle_t uart_queue;
#endif
};

struct nhal_spi_impl_ctx{
    SemaphoreHandle_t mutex;
    bool is_initialized;
    bool is_configured;
    bool is_driver_installed;
    spi_device_handle_t device_handle;
    
#if defined(NHAL_SPI_ASYNC_DMA_SUPPORT)
    spi_device_handle_t async_device_handle;
#endif
};

struct nhal_wdt_impl_ctx{
    bool is_initialized;
    bool is_configured;
} ;

nhal_result_t nhal_map_esp_err(esp_err_t esp_err);

#endif // NHAL_IMPL_ESP32_DEFS_H
