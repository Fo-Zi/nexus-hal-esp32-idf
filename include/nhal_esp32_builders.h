#ifndef NHAL_ESP32_BUILDERS_H
#define NHAL_ESP32_BUILDERS_H

#include "driver/uart.h"

#define NHAL_ESP32_I2C_MASTER_BUILD(name, bus_id, sda, scl, sda_pullup, scl_pullup, clock_freq) \
    static struct nhal_i2c_impl_config name##_i2c_impl_cfg = { \
        .mode = I2C_MODE_MASTER, \
        .sda_io_num = (sda), \
        .scl_io_num = (scl), \
        .sda_pullup_en = (sda_pullup) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE, \
        .scl_pullup_en = (scl_pullup) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE \
    }; \
    static struct nhal_i2c_impl_ctx name##_i2c_impl_ctx = {0}; \
    static struct nhal_i2c_config name##_i2c_cfg = { \
        .clock_speed_hz = (clock_freq), \
        .impl_config = &name##_i2c_impl_cfg \
    }; \
    static struct nhal_i2c_context name##_i2c_ctx = { \
        .i2c_bus_id = (bus_id), \
        .impl_ctx = &name##_i2c_impl_ctx \
    };

#define NHAL_ESP32_I2C_CONFIG_REF(name) (&name##_i2c_cfg)
#define NHAL_ESP32_I2C_CONTEXT_REF(name) (&name##_i2c_ctx)

#define NHAL_ESP32_PIN_BUILD(name, pin_number, dir, pullup_en, pulldown_en, intr ) \
    static struct nhal_pin_id name##_pin_id = { \
        .pin_num = (pin_number) \
    }; \
    static struct nhal_pin_impl_config name##_pin_impl_cfg = { \
        .intr_type = (intr) \
    }; \
    static struct nhal_pin_impl_ctx name##_pin_impl_ctx = {0}; \
    static struct nhal_pin_config name##_pin_cfg = { \
        .direction = (dir), \
        .pull_mode = (pullup_en) ? ((pulldown_en) ? NHAL_PIN_PMODE_PULL_UP_AND_DOWN : NHAL_PIN_PMODE_PULL_UP) : ((pulldown_en) ? NHAL_PIN_PMODE_PULL_DOWN : NHAL_PIN_PMODE_NONE), \
        .impl_config = &name##_pin_impl_cfg \
    }; \
    static struct nhal_pin_context name##_pin_ctx = { \
        .pin_id = (&name##_pin_id), \
        .impl_ctx = &name##_pin_impl_ctx \
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
    static struct nhal_uart_impl_ctx name##_uart_impl_ctx = {0}; \
    static struct nhal_uart_config name##_uart_cfg = { \
        .baudrate = (baud_rate), \
        .parity = NHAL_UART_PARITY_NONE, \
        .stop_bits = NHAL_UART_STOP_BITS_1, \
        .data_bits = NHAL_UART_DATA_BITS_8, \
        .impl_config = &name##_uart_impl_cfg \
    }; \
    static struct nhal_uart_context name##_uart_ctx = { \
        .uart_bus_id = (uart_num), \
        .impl_ctx = &name##_uart_impl_ctx \
    };

#define NHAL_ESP32_UART_CONFIG_REF(name) (&name##_uart_cfg)
#define NHAL_ESP32_UART_CONTEXT_REF(name) (&name##_uart_ctx)

#define NHAL_ESP32_I2C_ASYNC_BUILD(name, dma_chan, max_transfer_size) \
    static struct nhal_i2c_async_impl_config name##_i2c_async_impl_cfg = { \
        .dma_channel = (dma_chan), \
        .max_transfer_size = (max_transfer_size) \
    }; \
    static struct nhal_async_config name##_i2c_async_cfg = { \
        .impl_config = &name##_i2c_async_impl_cfg \
    };

#define NHAL_ESP32_I2C_ASYNC_CONFIG_REF(name) (&name##_i2c_async_cfg)

#define NHAL_ESP32_SPI_ASYNC_BUILD(name, dma_chan, max_transfer_size) \
    static struct nhal_spi_async_impl_config name##_spi_async_impl_cfg = { \
        .dma_channel = (dma_chan), \
        .max_transfer_size = (max_transfer_size) \
    }; \
    static struct nhal_async_config name##_spi_async_cfg = { \
        .impl_config = &name##_spi_async_impl_cfg \
    };

#define NHAL_ESP32_SPI_ASYNC_CONFIG_REF(name) (&name##_spi_async_cfg)

#define NHAL_ESP32_UART_ASYNC_BUILD(name, tx_buffer_size, rx_buffer_size, use_interrupts) \
    static struct nhal_uart_async_impl_config name##_uart_async_impl_cfg = { \
        .tx_buffer_size = (tx_buffer_size), \
        .rx_buffer_size = (rx_buffer_size), \
        .use_interrupts = (use_interrupts) \
    }; \
    static struct nhal_async_config name##_uart_async_cfg = { \
        .impl_config = &name##_uart_async_impl_cfg \
    };

#define NHAL_ESP32_UART_ASYNC_CONFIG_REF(name) (&name##_uart_async_cfg)

#endif
