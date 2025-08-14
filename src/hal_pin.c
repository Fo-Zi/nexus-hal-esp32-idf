#include "hal_esp32_defs.h"
#include <hal_pin_types.h>
#include <hal_pin.h>

#include "driver/gpio.h"
#include "esp_err.h"

void hal_config_to_esp_config(struct hal_pin_config * config, gpio_config_t * esp_pin_config){
    esp_pin_config->intr_type       = config->impl_config->intr_type ;
    esp_pin_config->pull_up_en      = config->impl_config->pull_up_en   ;
    esp_pin_config->pull_down_en    = config->impl_config->pull_down_en ;
};

static bool isr_service_initialized = false;
hal_pin_result_t hal_pin_init(struct hal_pin_context * pin_ctxt){

    if(!isr_service_initialized){

        // Global ISR init service for all pins configured as interrupts ->
        hal_pin_result_t result =  esp_err_to_i2c_hal_err(gpio_install_isr_service(0));
        if(result != HAL_PIN_OK){

        };
        isr_service_initialized = true;
        return result;

    }else{
        return HAL_PIN_OK;
    };

};

hal_pin_result_t hal_pin_deinit(struct hal_pin_context * pin_ctxt){
    if(isr_service_initialized){
        gpio_uninstall_isr_service();
        isr_service_initialized = false;
    }
    return HAL_PIN_OK;
};

hal_pin_result_t hal_pin_set_config(struct hal_pin_context * pin_ctxt, struct hal_pin_config * config ){

    gpio_config_t esp_pin_config;
    hal_config_to_esp_config(config,&esp_pin_config);

    hal_pin_result_t result = esp_err_to_i2c_hal_err(gpio_config(&esp_pin_config));
    if(result != HAL_PIN_OK){

    }

    return result;

};

hal_pin_result_t hal_pin_get_config(struct hal_pin_context * pin_ctxt, struct hal_pin_config * config ){
    return HAL_PIN_ERR_OTHER; // idf doesn't provide a way to get pin config
};

hal_pin_result_t hal_pin_get_state(struct hal_pin_context * pin_ctxt, hal_pin_state_t *value){
    *value = gpio_get_level(pin_ctxt->pin_id->num);
    return HAL_PIN_OK;
};

hal_pin_result_t hal_pin_set_state(struct hal_pin_context * pin_ctxt, hal_pin_state_t value){
    hal_pin_result_t result = esp_err_to_i2c_hal_err(gpio_set_level(pin_ctxt->pin_id->num, value));
    if(result != HAL_PIN_OK){

    }
    return result;
};

hal_pin_result_t hal_pin_set_callback( struct hal_pin_context * pin_ctxt, hal_pin_callback_t callback ){
    hal_pin_result_t result = esp_err_to_pin_hal_err(gpio_isr_handler_add(pin_ctxt->pin_id->num, callback, NULL));
    if(result != HAL_PIN_OK){

    }
    return result;
};
