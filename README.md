# NHAL ESP-IDF Implementation

## Overview

This component provides a complete ESP-IDF implementation of the NHAL interfaces. It bridges the standardized NHAL API to ESP-IDF's native peripheral drivers, enabling portable code to run on ESP32 family microcontrollers.

## Design Philosophy

### ESP-IDF Integration
This implementation leverages ESP-IDF's proven peripheral drivers while providing the NHAL abstraction layer. It handles:
- ESP-IDF driver initialization and configuration
- Error code mapping from ESP-IDF to NHAL standards
- FreeRTOS integration for thread safety
- Platform-specific optimizations (DMA, interrupt handling)

### Transparent Optimization
The implementation automatically optimizes operations based on:
- **Transfer size**: Small transfers use CPU copy, large transfers use DMA
- **Hardware capabilities**: Utilizes available ESP32 hardware features
- **Context mode**: Sync-only vs sync-and-async operation modes

### State Management
Enforces the NHAL state lifecycle through runtime validation:
1. Context structures track initialization and configuration status
2. Operations validate state before execution
3. Proper error codes returned for invalid state transitions

## Dependencies

### Required Dependencies
- **ESP-IDF**: v5.0 or newer
- **FreeRTOS**: Included with ESP-IDF
- **CMake**: 3.16 or newer for build system

### ESP-IDF Components Used
- `driver` - Peripheral drivers (I2C, SPI, UART, GPIO)
- `esp_timer` - High-resolution timing
- `freertos` - RTOS services and synchronization
- `esp_common` - Common ESP-IDF utilities

### Target Platforms
- ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6, ESP32-H2
- Other ESP32 family devices supported by ESP-IDF v5.0+

## Implementation Mapping

### I2C Master
- **Files**: `nhal_i2c.c`, `nhal_i2c_async.c`, `nhal_i2c_transfer.c`
- **ESP-IDF APIs**: `i2c_master_*` functions from `driver/i2c_master.h`
- **Features**: Master mode, 7-bit addressing, timeout support
- **Optimizations**: Automatic DMA usage for large transfers

### SPI Master
- **Files**: `nhal_spi.c`, `nhal_spi_async.c`  
- **ESP-IDF APIs**: `spi_master_*` functions from `driver/spi_master.h`
- **Features**: Full-duplex, configurable clock/mode, CS control
- **Optimizations**: DMA for async operations, CPU for sync small transfers

### UART
- **Files**: `nhal_uart.c`, `nhal_uart_async.c`
- **ESP-IDF APIs**: `uart_*` functions from `driver/uart.h`  
- **Features**: Configurable baud rates, parity, stop bits, flow control
- **Async Support**: Ring buffer-based non-blocking I/O

### GPIO/Pin Control
- **File**: `nhal_pin.c`
- **ESP-IDF APIs**: `gpio_*` functions from `driver/gpio.h`
- **Features**: Input/output, pull-up/down, interrupt support
- **Thread Safety**: Interrupt service installation and management

### Watchdog Timer
- **File**: `nhal_wdt.c`
- **ESP-IDF APIs**: `esp_task_wdt_*` functions from `esp_task_wdt.h`
- **Features**: Task watchdog configuration, feeding, panic handling
- **Integration**: FreeRTOS task monitoring

### Common Utilities
- **Files**: `nhal_common.c`, `nhal_esp32_defs.c`
- **Functions**: Delay operations, error mapping, ESP32-specific definitions
- **Services**: Microsecond/millisecond delays using ESP-IDF timing

## Error Mapping

ESP-IDF error codes are mapped to NHAL standard errors:

| ESP-IDF Error | NHAL Error |
|---------------|------------|
| `ESP_OK` | `NHAL_OK` |
| `ESP_ERR_TIMEOUT` | `NHAL_ERR_TIMEOUT` |
| `ESP_ERR_INVALID_ARG` | `NHAL_ERR_INVALID_ARG` |
| `ESP_ERR_NOT_SUPPORTED` | `NHAL_ERR_UNSUPPORTED` |
| `ESP_ERR_NO_MEM` | `NHAL_ERR_OUT_OF_MEMORY` |
| Other errors | `NHAL_ERR_OTHER` |

## Platform Configuration

### Implementation-Specific Structures
The implementation defines concrete versions of all `impl_*` structures:

```c
struct nhal_i2c_impl_config {
    uint8_t sda_io_num;      // SDA pin number  
    uint8_t scl_io_num;      // SCL pin number
    uint8_t sda_pullup_en;   // SDA pull-up enable
    uint8_t scl_pullup_en;   // SCL pull-up enable  
    uint32_t clk_speed;      // Clock frequency in Hz
};

struct nhal_spi_impl_config {
    uint8_t mosi_pin;        // MOSI pin number
    uint8_t miso_pin;        // MISO pin number
    uint8_t sclk_pin;        // Clock pin number
    uint8_t cs_pin;          // Chip select pin number
};
// ... similar structures for UART, PIN, WDT
```

### Context Management
Implementation contexts include:
- Mutex handles for thread safety
- ESP-IDF driver state tracking
- Async operation handles (queues, DMA descriptors)
- Configuration and initialization status flags

## Integration with ESP-IDF Projects

### Component Registration
Add to your project's CMakeLists.txt:

```cmake
idf_component_register(
    REQUIRES
        nhal-esp32
)
```

### Build Configuration
The implementation includes automatic version checking:
- CMake validates ESP-IDF version at configure time
- Preprocessor checks provide compile-time validation
- Clear error messages guide version upgrade if needed

### Configuration Requirements
Some peripherals require ESP-IDF configuration:
- **Task Watchdog**: `CONFIG_ESP_TASK_WDT=y` must be enabled
- **DMA**: Automatically configured based on transfer requirements

## Memory and Performance

### Memory Usage
- **Sync-only mode**: Minimal RAM usage, stack-based operations
- **Sync-and-async mode**: Additional RAM for queues and DMA descriptors
- **Per-context overhead**: ~100-200 bytes depending on peripheral type

### Performance Characteristics
- **I2C**: Up to 1MHz clock, DMA for transfers >32 bytes
- **SPI**: Up to 80MHz clock, DMA for transfers >64 bytes  
- **UART**: Up to 5Mbps, ring buffers for async operations
- **GPIO**: Microsecond-level response times for pin operations

## Version Compatibility

This implementation is tested and supported with:
- **ESP-IDF**: v5.0.x, v5.1.x, v5.2.x, v5.3.x+
- **Build Systems**: ESP-IDF native build, PlatformIO
- **Development Tools**: ESP-IDF toolchain, compatible debuggers

Older ESP-IDF versions (<5.0) are not supported due to breaking API changes in peripheral drivers.