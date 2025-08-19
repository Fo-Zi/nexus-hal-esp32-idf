#include "nhal_esp32_defs.h"

nhal_result_t nhal_map_esp_err(esp_err_t esp_err){
    switch (esp_err) {
        case ESP_OK:
            return NHAL_OK;
        case ESP_ERR_TIMEOUT:
            return NHAL_ERR_TIMEOUT;
        case ESP_ERR_INVALID_ARG:
            return NHAL_ERR_INVALID_ARG;
        case ESP_ERR_NOT_SUPPORTED:
            return NHAL_ERR_UNSUPPORTED;
        case ESP_ERR_NO_MEM:
            return NHAL_ERR_OUT_OF_MEMORY;
        default:
            return NHAL_ERR_OTHER;
    }
};
