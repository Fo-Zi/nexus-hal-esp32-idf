# ESP-IDF HAL Implementation

## Overview
This directory contains the ESP32-specific implementation of the Hardware Abstraction Layer (HAL) interfaces defined in the `hal-interface` module.

## Requirements
- **ESP-IDF**: v5.5 or newer
- **CMake**: 3.16 or newer
- **Target platforms**: ESP32, ESP32-S2, ESP32-S3, ESP32-C3

## Version Compatibility
This HAL implementation is based on ESP-IDF v5.5.
Using older versions may result in compilation errors or undefined behavior.

### Supported ESP-IDF Features
- UART driver with sync and async operations
- I2C master mode operations with transparent DMA optimization
- SPI master mode operations with transparent DMA optimization
- GPIO pin control
- FreeRTOS integration

## Usage
When including `my_basic_hal` component in your ESP-IDF project, the build system will automatically verify ESP-IDF version compatibility and fail with a descriptive error if requirements are not met.

### Integration
Add the HAL as a component dependency in your project's CMakeLists.txt:
```cmake
idf_component_register(
    REQUIRES
        my_basic_hal
)
```

## Version Enforcement
The HAL implementation includes multiple layers of version checking:

1. **CMake Build-Time Check**: Validates ESP-IDF version during build configuration
2. **Preprocessor Check**: Additional compile-time validation in headers
3. **Clear Error Messages**: Descriptive errors explain version requirements and solutions

## Implementation Files
- `hal_uart.c` - UART implementation using ESP-IDF UART driver
- `hal_i2c.c` - I2C implementation using ESP-IDF I2C driver
- `hal_spi.c` - SPI implementation using ESP-IDF SPI master driver
- `hal_pin.c` - GPIO implementation using ESP-IDF GPIO driver
- `hal_common.c` - Common utility functions
- `hal_esp32_defs.c` - ESP32-specific definitions and conversions

## Interface Compatibility
This implementation provides the interfaces defined in:
- `hal_uart_basic.h` - Synchronous UART operations with automatic optimization
- `hal_uart_async.h` - Asynchronous UART operations with buffering (optional)
- `hal_i2c_master.h` - Synchronous I2C master operations with transparent DMA
- `hal_i2c_master_async.h` - Asynchronous I2C master operations (optional)
- `hal_spi_master.h` - Synchronous SPI master operations with transparent DMA
- `hal_spi_master_async.h` - Asynchronous SPI master operations (optional)
- `hal_pin.h` - GPIO pin control
- `hal_common.h` - Common types and utilities

## Operation Modes
Each peripheral supports two operation modes:
- **Sync-Only Mode**: Blocking operations only, minimal memory footprint
- **Sync-and-Async Mode**: Both blocking and non-blocking operations with DMA support

The HAL automatically optimizes transfer methods (CPU vs DMA) based on transfer size and hardware capabilities.
