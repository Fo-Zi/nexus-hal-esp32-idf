#include "nhal_esp32_defs.h"
#include "nhal_esp32_helpers.h"
#include <nhal_pin_types.h>
#include <nhal_pin.h>

#include "driver/gpio.h"
#include "esp_err.h"

void nhal_config_to_esp_config(struct nhal_pin_context * ctx, struct nhal_pin_config * config, gpio_config_t * esp_pin_config){
    esp_pin_config->pin_bit_mask = (1ULL << ctx->pin_num);
    esp_pin_config->mode = (config->direction == NHAL_PIN_DIR_OUTPUT) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
    esp_pin_config->intr_type = (gpio_int_type_t)config->impl_config->intr_type;
    nhal_to_esp32_pull_mode(config->pull_mode, &esp_pin_config->pull_up_en, &esp_pin_config->pull_down_en);
};

static bool isr_service_initialized = false;
static int isr_service_ref_count = 0;

static gpio_int_type_t nhal_trigger_to_esp_int_type(nhal_pin_int_trigger_t trigger) {
    switch (trigger) {
        case NHAL_PIN_INT_TRIGGER_RISING_EDGE:
            return GPIO_INTR_POSEDGE;
        case NHAL_PIN_INT_TRIGGER_FALLING_EDGE:
            return GPIO_INTR_NEGEDGE;
        case NHAL_PIN_INT_TRIGGER_BOTH_EDGES:
            return GPIO_INTR_ANYEDGE;
        case NHAL_PIN_INT_TRIGGER_HIGH_LEVEL:
            return GPIO_INTR_HIGH_LEVEL;
        case NHAL_PIN_INT_TRIGGER_LOW_LEVEL:
            return GPIO_INTR_LOW_LEVEL;
        case NHAL_PIN_INT_TRIGGER_NONE:
        default:
            return GPIO_INTR_DISABLE;
    }
}

static void IRAM_ATTR gpio_isr_wrapper(void *arg) {
    struct nhal_pin_context *ctx = (struct nhal_pin_context *)arg;
    if (ctx && ctx->user_callback) {
        ctx->user_callback(ctx, ctx->user_data);
    }
}
nhal_result_t nhal_pin_init(struct nhal_pin_context * ctx){
    if (ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (ctx->is_initialized) {
        return NHAL_OK;
    }

    if(!isr_service_initialized){
        // Global ISR init service for all pins configured as interrupts
        nhal_result_t result = nhal_map_esp_err(gpio_install_isr_service(0));
        if(result != NHAL_OK){
            return result;
        }
        isr_service_initialized = true;
    }

    isr_service_ref_count++;
    ctx->is_initialized = true;
    ctx->is_configured = false;
    ctx->is_interrupt_configured = false;
    ctx->is_interrupt_enabled = false;
    ctx->user_callback = NULL;
    ctx->user_data = NULL;
    ctx->interrupt_trigger = NHAL_PIN_INT_TRIGGER_NONE;
    return NHAL_OK;
};

nhal_result_t nhal_pin_deinit(struct nhal_pin_context * ctx){
    if (ctx == NULL ) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->is_initialized) {
        return NHAL_OK;
    }

    // Disable interrupt if enabled
    if (ctx->is_interrupt_enabled) {
        gpio_isr_handler_remove(ctx->pin_num);
        gpio_set_intr_type(ctx->pin_num, GPIO_INTR_DISABLE);
        ctx->is_interrupt_enabled = false;
    }

    ctx->is_initialized = false;
    ctx->is_interrupt_configured = false;
    isr_service_ref_count--;

    // Only uninstall ISR service when no pins are using it
    if(isr_service_ref_count <= 0 && isr_service_initialized){
        gpio_uninstall_isr_service();
        isr_service_initialized = false;
        isr_service_ref_count = 0;
    }

    return NHAL_OK;
};

nhal_result_t nhal_pin_set_config(struct nhal_pin_context * ctx, struct nhal_pin_config * config ){
    if (ctx == NULL || config == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    gpio_config_t esp_pin_config;
    nhal_config_to_esp_config(ctx, config, &esp_pin_config);

    nhal_result_t result = nhal_map_esp_err(gpio_config(&esp_pin_config));
    if(result != NHAL_OK){
        return result;
    }

    ctx->is_configured = true;
    return result;

};

nhal_result_t nhal_pin_get_config(struct nhal_pin_context * ctx, struct nhal_pin_config * config ){
    return NHAL_ERR_OTHER; // idf doesn't provide a way to get pin config
};

nhal_result_t nhal_pin_get_state(struct nhal_pin_context * ctx, nhal_pin_state_t *value){
    if (ctx == NULL || value == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    *value = gpio_get_level(ctx->pin_num);
    return NHAL_OK;
};

nhal_result_t nhal_pin_set_state(struct nhal_pin_context * ctx, nhal_pin_state_t value){
    if (ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    nhal_result_t result = nhal_map_esp_err(gpio_set_level(ctx->pin_num, value));
    if(result != NHAL_OK){
        return result;
    }
    return result;
};

nhal_result_t nhal_pin_set_interrupt_config(
    struct nhal_pin_context *ctx,
    nhal_pin_int_trigger_t trigger,
    nhal_pin_callback_t callback,
    void *user_data
){
    if (ctx == NULL || callback == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    // Store the interrupt configuration
    ctx->user_callback = callback;
    ctx->user_data = user_data;
    ctx->interrupt_trigger = trigger;
    ctx->is_interrupt_configured = true;
    ctx->is_interrupt_enabled = false;

    return NHAL_OK;
}

nhal_result_t nhal_pin_interrupt_enable(struct nhal_pin_context *ctx){
    if (ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    if (!ctx->is_interrupt_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    if (ctx->is_interrupt_enabled) {
        return NHAL_OK; // Already enabled
    }

    // Set the interrupt type
    gpio_int_type_t esp_int_type = nhal_trigger_to_esp_int_type(ctx->interrupt_trigger);
    nhal_result_t result = nhal_map_esp_err(gpio_set_intr_type(ctx->pin_num, esp_int_type));
    if (result != NHAL_OK) {
        return result;
    }

    // Add the ISR handler
    result = nhal_map_esp_err(gpio_isr_handler_add(ctx->pin_num, gpio_isr_wrapper, ctx));
    if (result != NHAL_OK) {
        gpio_set_intr_type(ctx->pin_num, GPIO_INTR_DISABLE); // Cleanup on failure
        return result;
    }

    ctx->is_interrupt_enabled = true;
    return NHAL_OK;
}

nhal_result_t nhal_pin_interrupt_disable(struct nhal_pin_context *ctx){
    if (ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!ctx->is_interrupt_enabled) {
        return NHAL_OK; // Already disabled
    }

    // Remove the ISR handler
    gpio_isr_handler_remove(ctx->pin_num);

    // Disable the interrupt
    nhal_result_t result = nhal_map_esp_err(gpio_set_intr_type(ctx->pin_num, GPIO_INTR_DISABLE));
    if (result != NHAL_OK) {
        return result;
    }

    ctx->is_interrupt_enabled = false;
    return NHAL_OK;
}


nhal_result_t nhal_pin_set_direction(struct nhal_pin_context * ctx, nhal_pin_dir_t direction, nhal_pin_pull_mode_t pull_mode){
    if (ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    gpio_config_t esp_pin_config = {0};

    esp_pin_config.pin_bit_mask = (1ULL << ctx->pin_num);
    esp_pin_config.mode = (direction == NHAL_PIN_DIR_OUTPUT) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
    esp_pin_config.intr_type = GPIO_INTR_DISABLE;

    switch(pull_mode) {
        case NHAL_PIN_PMODE_NONE:
            esp_pin_config.pull_up_en = GPIO_PULLUP_DISABLE;
            esp_pin_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case NHAL_PIN_PMODE_PULL_UP:
            esp_pin_config.pull_up_en = GPIO_PULLUP_ENABLE;
            esp_pin_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case NHAL_PIN_PMODE_PULL_DOWN:
            esp_pin_config.pull_up_en = GPIO_PULLUP_DISABLE;
            esp_pin_config.pull_down_en = GPIO_PULLDOWN_ENABLE;
            break;
        default:
            return NHAL_ERR_INVALID_ARG;
    }

    nhal_result_t result = nhal_map_esp_err(gpio_config(&esp_pin_config));
    return result;
};
