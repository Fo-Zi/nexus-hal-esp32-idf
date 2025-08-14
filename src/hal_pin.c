#include "hal_esp32_defs.h"
#include <hal_pin_types.h>
#include <hal_pin.h>

#include "driver/gpio.h"
#include "esp_err.h"

void hal_config_to_esp_config(hal_pin_config_t * config, gpio_config_t * esp_pin_config){
    esp_pin_config->mode            = config->mode         ;
    esp_pin_config->intr_type       = config->intr_type    ;
    esp_pin_config->pull_up_en      = config->pull_up_en   ;
    esp_pin_config->pull_down_en    = config->pull_down_en ;
};

static bool isr_service_initialized = false;
hal_pin_result_t hal_pin_init(hal_pin_context_t * pin_ctxt){

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

hal_pin_result_t hal_pin_deinit(hal_pin_context_t * pin_ctxt){
    if(isr_service_initialized){
        gpio_uninstall_isr_service();
        isr_service_initialized = false;
    }
    return HAL_PIN_OK;
};

hal_pin_result_t hal_pin_set_config(hal_pin_context_t * pin_ctxt, hal_pin_config_t * config ){

    gpio_config_t esp_pin_config;
    hal_config_to_esp_config(config,&esp_pin_config);

    hal_pin_result_t result = esp_err_to_i2c_hal_err(gpio_config(&esp_pin_config));
    if(result != HAL_PIN_OK){

    }

    return result;

};

hal_pin_result_t hal_pin_get_config(hal_pin_context_t * pin_ctxt, hal_pin_config_t * config ){
    return HAL_PIN_ERR_OTHER; // idf doesn't provide a way to get pin config
};

hal_pin_result_t hal_pin_get_state(hal_pin_context_t * pin_ctxt, hal_pin_state_t *value){
    *value = gpio_get_level(pin_ctxt->pin_num);
    return HAL_PIN_OK;
};

hal_pin_result_t hal_pin_set_state(hal_pin_context_t * pin_ctxt, hal_pin_state_t value){
    hal_pin_result_t result = esp_err_to_i2c_hal_err(gpio_set_level(pin_ctxt->pin_num, value));
    if(result != HAL_PIN_OK){

    }
    return result;
};

hal_pin_result_t hal_pin_set_callback( hal_pin_context_t * pin_ctxt, hal_pin_callback_t callback ){
    hal_pin_result_t result = esp_err_to_pin_hal_err(gpio_isr_handler_add(pin_ctxt->pin_num, callback, NULL));
    if(result != HAL_PIN_OK){

    }
    return result;
};
