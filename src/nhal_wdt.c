#include "nhal_esp32_defs.h"
#include "nhal_esp32_helpers.h"
#include <nhal_wdt_types.h>
#include <nhal_wdt.h>

#include "esp_task_wdt.h"
#include "esp_err.h"
#include "esp_log.h"

// Sanity for required ESP-IDF configuration
#ifndef CONFIG_ESP_TASK_WDT
#error "CONFIG_ESP_TASK_WDT must be enabled in sdkconfig for HAL watchdog functionality"
#endif


nhal_result_t nhal_wdt_init(struct nhal_wdt_context * ctx) {
    if (ctx == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (ctx->impl_ctx->is_initialized) {
        return NHAL_OK;
    }

    ctx->impl_ctx->is_initialized = true;
    ctx->impl_ctx->is_configured = false;
    ctx->is_started = false;

    return NHAL_OK;
}

nhal_result_t nhal_wdt_deinit(struct nhal_wdt_context * ctx) {
    if (ctx == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_OK;
    }

    // Stop watchdog if running
    if (ctx->is_started) {
        nhal_result_t result = nhal_map_esp_err(esp_task_wdt_deinit());
        if (result != NHAL_OK) {
            return result;
        }
        ctx->is_started = false;
    }

    ctx->impl_ctx->is_initialized = false;
    return NHAL_OK;
}

nhal_result_t nhal_wdt_set_config(struct nhal_wdt_context * ctx, struct nhal_wdt_config * config) {
    if (ctx == NULL || ctx->impl_ctx == NULL || config == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    // Cache configuration in context for later use by enable()
    ctx->timeout_ms = config->timeout_ms;
    ctx->impl_ctx->is_configured = true;

    return NHAL_OK;
}

nhal_result_t nhal_wdt_get_config(struct nhal_wdt_context * ctx, struct nhal_wdt_config * config) {
    if (ctx == NULL || ctx->impl_ctx == NULL || config == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    config->timeout_ms = ctx->timeout_ms;

    return NHAL_OK;
}

nhal_result_t nhal_wdt_enable(struct nhal_wdt_context * ctx) {
    if (ctx == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!ctx->impl_ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    if (ctx->is_started) {
        return NHAL_ERR_ALREADY_STARTED;
    }

    // Configure ESP32 Task Watchdog Timer
    esp_task_wdt_config_t esp_config = {
        .timeout_ms = ctx->timeout_ms,
        .idle_core_mask = 0
    };

    nhal_result_t result = nhal_map_esp_err(esp_task_wdt_init(&esp_config));
    if (result != NHAL_OK) {
        return result;
    }

    ctx->is_started = true;
    return NHAL_OK;
}

nhal_result_t nhal_wdt_disable(struct nhal_wdt_context * ctx) {
    if (ctx == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!ctx->impl_ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    if (!ctx->is_started) {
        return NHAL_ERR_NOT_STARTED;
    }

    nhal_result_t result = nhal_map_esp_err(esp_task_wdt_deinit());
    if (result != NHAL_OK) {
        return result;
    }

    ctx->is_started = false;
    return NHAL_OK;
}

nhal_result_t nhal_wdt_feed(struct nhal_wdt_context * ctx) {
    if (ctx == NULL || ctx->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!ctx->impl_ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    if (!ctx->is_started) {
        return NHAL_ERR_NOT_STARTED;
    }

    // For ESP32, we need to add current task to watchdog and then reset
    // This is a simplified implementation - in practice, you might want to
    // add the task once during enable and then just reset
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();

    esp_err_t add_result = esp_task_wdt_add(current_task);
    if (add_result != ESP_OK && add_result != ESP_ERR_INVALID_STATE) {
        // ESP_ERR_INVALID_STATE means task is already added, which is OK
        return nhal_map_esp_err(add_result);
    }

    nhal_result_t result = nhal_map_esp_err(esp_task_wdt_reset());
    return result;
}
