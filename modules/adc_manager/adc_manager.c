/**
 * @file adc_manager.c
 * @brief Implementação de um driver para o conversor ADC ADS1115.
 *
 * Este módulo fornece uma camada de abstração sobre a biblioteca ADS1115,
 * adicionando mecanismos essenciais de deteção e tratamento de erros.
 */

#include "adc_manager.h"
#include <stdio.h>
#include "hardware/i2c.h"

// --- Configuração do Hardware ---
#define I2C_PORT i2c0
#define I2C_FREQ 400000
#define ADS1115_I2C_ADDR 0x48
const uint8_t SDA_PIN = 0;
const uint8_t SCL_PIN = 1;

// --- Variáveis de Estado do Módulo ---

/**
 * @brief Instância da estrutura de controlo do ADC da biblioteca.
 */
static struct ads1115_adc adc;

/**
 * @brief Flag para verificar a inicialização.
 */
static bool is_initialized = false;

/**
 * @brief Realiza uma verificação de baixo nível para a presença do ADC no barramento I2C.
 *
 * Esta função comunica-se diretamente
 * com o barramento I2C para "pingar" o dispositivo.
 *
 * @return true se o dispositivo responder (ACK) no barramento, false caso contrário.
 */
bool adc_module_is_connected() {
    uint8_t dummy_byte;

    // Tenta ler 1 byte do dispositivo. Retornará
    // um valor >= 0 em caso de sucesso ou um código de erro negativo em caso de falha (timeout, NACK).

    int result = i2c_read_timeout_us(I2C_PORT, ADS1115_I2C_ADDR, &dummy_byte, 1, false, ADC_CONNECTION_CHECK_TIMEOUT_MS * 1000);
    return result == 1;
}

/**
 * @brief Inicializa o módulo ADC, validando a comunicação com o hardware.
 */
adc_status_t adc_module_init(void) {
    // Garante a idempotência da função.
    if (is_initialized) {
        return ADC_STATUS_OK;
    }

    // Configuração de baixo nível dos pinos e do periférico I2C.
    i2c_init(I2C_PORT, I2C_FREQ);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    
    // Impede o arranque do sistema se o hardware
    // essencial não estiver presente.
    if (!adc_module_is_connected()) {
        printf("[ERRO FATAL] O dispositivo ADC (ADS1115) não foi encontrado no barramento I2C.\n");
        is_initialized = false;
        return ADC_STATUS_INIT_FAILED;
    }

    // Apenas se a comunicação for confirmada, prossegue com a configuração
    // lógica do dispositivo usando a biblioteca de abstração.
    ads1115_init(I2C_PORT, ADS1115_I2C_ADDR, &adc);
    ads1115_set_pga(ADS1115_PGA_4_096, &adc);
    ads1115_set_data_rate(ADS1115_RATE_128_SPS, &adc);
    ads1115_write_config(&adc);

    is_initialized = true;
    printf("[OK] Modulo ADC (ADS1115) inicializado.\n");
    return ADC_STATUS_OK;
}

/**
 * @brief Lê um valor de tensão de um canal específico do ADC.
 */
adc_status_t adc_module_read_voltage(enum ads1115_mux_t channel, float *voltage_out) {
    // Verificação defensiva contra ponteiros nulos para evitar falhas de segmentação.
    if (voltage_out == NULL) {
        return ADC_STATUS_INVALID_PARAM;
    }

    // Garante que o módulo não seja utilizado antes da inicialização bem-sucedida.
    if (!is_initialized) {
        return ADC_STATUS_NOT_INITIALIZED;
    }

    uint16_t adc_value;
    
    // Configura e realiza a leitura utilizando a biblioteca.
    ads1115_set_input_mux(channel, &adc);
    ads1115_write_config(&adc);
    ads1115_read_adc(&adc_value, &adc);

    // Converte o valor bruto para Volts e o armazena no ponteiro de saída.
    *voltage_out = ads1115_raw_to_volts(adc_value, &adc);
    
    return ADC_STATUS_OK;
}