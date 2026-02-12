/**
* @file sensor_manager.c
* @brief Implementation of sensor reading management
*
* Contains the initialization logic, conversion (calibration) functions
* and analog sensor readings.
*/
#include "sensor_manager.h"
#include <stdio.h>
#include "../analog_sensor/analog_sensor.h" 
#include "../adc_manager/adc_manager.h"

static float convert_linear_interpolation(float v, float max_v, float max_value, float min_value) {
    if (v < 0.0f) {
        return SENSOR_READ_ERROR;
    }
    if (max_v == 0.0f) {
        return min_value;
    }

    float value_range = max_value - min_value;
    return min_value + (v * (value_range / max_v));
}

static const analog_sensor_t temperature_sensor = {
    .adc_channel = ADS1115_MUX_SINGLE_0,
    .param1      = SENSOR_TEMPERATURE_MAX_VOLTAGE, 
    .param2      = SENSOR_TEMPERATURE_MAX_VALUE,  
    .param3      = SENSOR_TEMPERATURE_MIN_VALUE,   
    .convert     = convert_linear_interpolation
};

static const analog_sensor_t conductivity_sensor = {
    .adc_channel = ADS1115_MUX_SINGLE_1,
    .param1      = SENSOR_CONDUCTIVITY_MAX_VOLTAGE, 
    .param2      = SENSOR_CONDUCTIVITY_MAX_VALUE,  
    .param3      = SENSOR_CONDUCTIVITY_MIN_VALUE,   
    .convert     = convert_linear_interpolation
};

static const analog_sensor_t flow_sensor = {
    .adc_channel = ADS1115_MUX_SINGLE_2,
    .param1      = SENSOR_FLOW_MAX_VOLTAGE, 
    .param2      = SENSOR_FLOW_MAX_VALUE,  
    .param3      = SENSOR_FLOW_MIN_VALUE,   
    .convert     = convert_linear_interpolation
};

int sensors_init(void) {
    printf("[INFO] Inicializando sensores...\n");

    if (adc_module_init() != ADC_STATUS_OK) {
        printf("[ERRO FATAL] Falha ao inicializar o módulo ADC. Verifique o hardware.\n");
        return 1;
    }

    printf("[OK] Sensores inicializados com sucesso.\n");
    return 0;
}

int sensors_read_all(sensors_reading_t* reading) {
    if (!reading) {
        printf("[ERRO] Ponteiro para leitura dos sensores é nulo.\n");
        return 1;
    }

    // Verificação da conexão com o módulo ADC ADS1115.
    if (!adc_module_is_connected()) {
        printf("[ERRO EM EXECUÇÃO] Perda de comunicação com o ADC.\n");
        reading->temperature = SENSOR_READ_ERROR;
        reading->conductivity = SENSOR_READ_ERROR;
        reading->flow = SENSOR_READ_ERROR;
        return 1;
    }

    float temp_v, cond_v, flow_v;

    analog_sensor_read(&temperature_sensor, &temp_v, &reading->temperature);
    analog_sensor_read(&conductivity_sensor, &cond_v, &reading->conductivity);
    analog_sensor_read(&flow_sensor, &flow_v, &reading->flow);

    printf("[DADOS] Temp: %.2f C (%.5f V) | Cond: %.2f %% (%.5f V) | Flow: %.2f L/min (%.5f V)\n", 
           reading->temperature, temp_v, 
           reading->conductivity, cond_v, 
           reading->flow, flow_v
    );
    
    return 0;
}