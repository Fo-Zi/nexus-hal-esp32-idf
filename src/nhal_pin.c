#include "nhal_esp32_defs.h"
#include "nhal_esp32_helpers.h"
#include <nhal_pin_types.h>
#include <nhal_pin.h>

#include "driver/gpio.h"
#include "esp_err.h"

void nhal_config_to_esp_config(struct nhal_pin_context * pin_ctxt, struct nhal_pin_config * config, gpio_config_t * esp_pin_config){
    esp_pin_config->pin_bit_mask = (1ULL << pin_ctxt->pin_id->pin_num);
    esp_pin_config->mode = (config->direction == NHAL_PIN_DIR_OUTPUT) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
    esp_pin_config->intr_type = (gpio_int_type_t)config->impl_config->intr_type;
    nhal_to_esp32_pull_mode(config->pull_mode, &esp_pin_config->pull_up_en, &esp_pin_config->pull_down_en);
};

static bool isr_service_initialized = false;
static int isr_service_ref_count = 0;
nhal_result_t nhal_pin_init(struct nhal_pin_context * pin_ctxt){
    if (pin_ctxt == NULL || pin_ctxt->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (pin_ctxt->impl_ctx->is_initialized) {
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
    pin_ctxt->impl_ctx->is_initialized = true;
    pin_ctxt->impl_ctx->is_configured = false;
    return NHAL_OK;
};

nhal_result_t nhal_pin_deinit(struct nhal_pin_context * pin_ctxt){
    if (pin_ctxt == NULL || pin_ctxt->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!pin_ctxt->impl_ctx->is_initialized) {
        return NHAL_OK;
    }

    // Remove ISR handler for this specific pin if it was set
    gpio_isr_handler_remove(pin_ctxt->pin_id->pin_num);

    pin_ctxt->impl_ctx->is_initialized = false;
    isr_service_ref_count--;

    // Only uninstall ISR service when no pins are using it
    if(isr_service_ref_count <= 0 && isr_service_initialized){
        gpio_uninstall_isr_service();
        isr_service_initialized = false;
        isr_service_ref_count = 0;
    }

    return NHAL_OK;
};

nhal_result_t nhal_pin_set_config(struct nhal_pin_context * pin_ctxt, struct nhal_pin_config * config ){
    if (pin_ctxt == NULL || config == NULL || pin_ctxt->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!pin_ctxt->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    gpio_config_t esp_pin_config;
    nhal_config_to_esp_config(pin_ctxt, config, &esp_pin_config);

    nhal_result_t result = nhal_map_esp_err(gpio_config(&esp_pin_config));
    if(result != NHAL_OK){
        return result;
    }

    pin_ctxt->impl_ctx->is_configured = true;
    return result;

};

nhal_result_t nhal_pin_get_config(struct nhal_pin_context * pin_ctxt, struct nhal_pin_config * config ){
    return NHAL_ERR_OTHER; // idf doesn't provide a way to get pin config
};

nhal_result_t nhal_pin_get_state(struct nhal_pin_context * pin_ctxt, nhal_pin_state_t *value){
    if (pin_ctxt == NULL || value == NULL || pin_ctxt->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!pin_ctxt->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!pin_ctxt->impl_ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    *value = gpio_get_level(pin_ctxt->pin_id->pin_num);
    return NHAL_OK;
};

nhal_result_t nhal_pin_set_state(struct nhal_pin_context * pin_ctxt, nhal_pin_state_t value){
    if (pin_ctxt == NULL || pin_ctxt->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!pin_ctxt->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!pin_ctxt->impl_ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    nhal_result_t result = nhal_map_esp_err(gpio_set_level(pin_ctxt->pin_id->pin_num, value));
    if(result != NHAL_OK){
        return result;
    }
    return result;
};

nhal_result_t nhal_pin_set_callback( struct nhal_pin_context * pin_ctxt, nhal_pin_callback_t callback ){
    if (pin_ctxt == NULL || callback == NULL || pin_ctxt->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!pin_ctxt->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!pin_ctxt->impl_ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    nhal_result_t result = nhal_map_esp_err(gpio_isr_handler_add(pin_ctxt->pin_id->pin_num, callback, NULL));
    if(result != NHAL_OK){
        return result;
    }
    return result;
};

nhal_result_t nhal_pin_set_direction(struct nhal_pin_context * pin_ctxt, nhal_pin_dir_t direction, nhal_pin_pull_mode_t pull_mode){
    if (pin_ctxt == NULL || pin_ctxt->impl_ctx == NULL) {
        return NHAL_ERR_INVALID_ARG;
    }

    if (!pin_ctxt->impl_ctx->is_initialized) {
        return NHAL_ERR_NOT_INITIALIZED;
    }

    if (!pin_ctxt->impl_ctx->is_configured) {
        return NHAL_ERR_NOT_CONFIGURED;
    }

    gpio_config_t esp_pin_config = {0};

    esp_pin_config.pin_bit_mask = (1ULL << pin_ctxt->pin_id->pin_num);
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
        case NHAL_PIN_PMODE_PULL_UP_AND_DOWN:
            esp_pin_config.pull_up_en = GPIO_PULLUP_ENABLE;
            esp_pin_config.pull_down_en = GPIO_PULLDOWN_ENABLE;
            break;
        default:
            return NHAL_ERR_INVALID_ARG;
    }

    nhal_result_t result = nhal_map_esp_err(gpio_config(&esp_pin_config));
    return result;
};
