#ifndef NHAL_ESP32_BUILDERS_H
#define NHAL_ESP32_BUILDERS_H

#include "driver/uart.h"
#include "nhal_esp32_defs.h"
#include "nhal_pin_types.h"

#define NHAL_ESP32_I2C_MASTER_BUILD(name, bus_id, sda, scl, sda_pullup, scl_pullup, clock_freq, timeout_ms) \
    static struct nhal_i2c_impl_config name##_i2c_impl_cfg = { \
        .mode = I2C_MODE_MASTER, \
        .sda_io_num = (sda), \
        .scl_io_num = (scl), \
        .sda_pullup_en = (sda_pullup) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE, \
        .scl_pullup_en = (scl_pullup) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE, \
        .clock_speed_hz = (clock_freq), \
        .timeout_ms = (timeout_ms) \
    }; \
    static struct nhal_i2c_config name##_i2c_cfg = { \
        .impl_config = &name##_i2c_impl_cfg \
    }; \
    static struct nhal_i2c_context name##_i2c_ctx = { \
        .i2c_bus_id = (bus_id), \
        .is_initialized = false, \
        .is_configured = false, \
        .is_driver_installed = false, \
        .mutex = NULL, \
        .timeout_ms = 0 \
    };

#define NHAL_ESP32_I2C_CONFIG_REF(name) (&name##_i2c_cfg)
#define NHAL_ESP32_I2C_CONTEXT_REF(name) (&name##_i2c_ctx)

#define NHAL_ESP32_PIN_BUILD(name, pin_number, dir, pull_m, intr ) \
    static struct nhal_pin_impl_config name##_pin_impl_cfg = { \
        .intr_type = (intr) \
    }; \
    static struct nhal_pin_config name##_pin_cfg = { \
        .direction = (dir), \
        .pull_mode = (pull_m), \
        .impl_config = &name##_pin_impl_cfg \
    }; \
    static struct nhal_pin_context name##_pin_ctx = { \
        .pin_num = (pin_number) \
    };

#define NHAL_ESP32_PIN_CONFIG_REF(name) (&name##_pin_cfg)
#define NHAL_ESP32_PIN_CONTEXT_REF(name) (&name##_pin_ctx)

#define NHAL_ESP32_UART_BASIC_BUILD(name, uart_num, tx_pin, rx_pin, baud_rate) \
    static struct nhal_uart_impl_config name##_uart_impl_cfg = { \
        .tx_pin_number = (tx_pin), \
        .rx_pin_number = (rx_pin), \
        .tx_buffer_size = 1024, \
        .rx_buffer_size = 1024, \
        .rts_pin_number = -1, \
        .cts_pin_number = -1, \
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, \
        .source_clk = UART_SCLK_APB, \
        .intr_alloc_flags = 0, \
        .queue_size = 10, \
        .queue_msg_size = 0 \
    }; \
    static struct nhal_uart_config name##_uart_cfg = { \
        .baudrate = (baud_rate), \
        .parity = NHAL_UART_PARITY_NONE, \
        .stop_bits = NHAL_UART_STOP_BITS_1, \
        .data_bits = NHAL_UART_DATA_BITS_8, \
        .impl_config = &name##_uart_impl_cfg \
    }; \
    static struct nhal_uart_context name##_uart_ctx = { \
        .uart_bus_id = (uart_num), \
        .is_initialized = false, \
        .is_configured = false, \
        .is_driver_installed = false, \
        .mutex = NULL, \
        .timeout_ms = 1000 \
    };

#define NHAL_ESP32_UART_CONFIG_REF(name) (&name##_uart_cfg)
#define NHAL_ESP32_UART_CONTEXT_REF(name) (&name##_uart_ctx)

#define NHAL_ESP32_SPI_MASTER_BUILD(name, spi_host, mosi_pin, miso_pin, sclk_pin, cs_pin) \
    static struct nhal_spi_impl_config name##_spi_impl_cfg = { \
        .mosi_pin = (mosi_pin), \
        .miso_pin = (miso_pin), \
        .sclk_pin = (sclk_pin), \
        .cs_pin = (cs_pin) \
    }; \
    static struct nhal_spi_config name##_spi_cfg = { \
        .duplex = NHAL_SPI_FULL_DUPLEX, \
        .mode = NHAL_SPI_MODE_0, \
        .bit_order = NHAL_SPI_BIT_ORDER_MSB_FIRST, \
        .impl_config = &name##_spi_impl_cfg \
    }; \
    static struct nhal_spi_context name##_spi_ctx = { \
        .spi_bus_id = (spi_host), \
        .actual_frequency_hz = 0, \
        .is_initialized = false, \
        .is_configured = false, \
        .is_driver_installed = false, \
        .device_handle = NULL, \
        .mutex = NULL \
    };

#define NHAL_ESP32_SPI_CONFIG_REF(name) (&name##_spi_cfg)
#define NHAL_ESP32_SPI_CONTEXT_REF(name) (&name##_spi_ctx)

#endif
