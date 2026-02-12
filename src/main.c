#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "modules/ethernet_manager/ethernet_manager.h"
#include "modules/http_client/http_client.h"
#include "modules/sensor_manager/sensor_manager.h"

/**
 * @brief Tarefa principal que encapsula a lógica de leitura e envio.
 *
 * Esta tarefa executa um ciclo de leitura de sensores e envio de dados
 * com uma frequência fixa, controlada pelo FreeRTOS.
 */
void main_task(__unused void *params) {
    // 1. Inicialização dos módulos
    ethernet_config_t eth_config = {
        .mac = {ETHERNET_MAC_0, ETHERNET_MAC_1, ETHERNET_MAC_2, 
                ETHERNET_MAC_3, ETHERNET_MAC_4, ETHERNET_MAC_5},
        .ip = {DEVICE_IP_0, DEVICE_IP_1, DEVICE_IP_2, DEVICE_IP_3},
        .subnet = {SUBNET_MASK_0, SUBNET_MASK_1, SUBNET_MASK_2, SUBNET_MASK_3},
        .gateway = {GATEWAY_IP_0, GATEWAY_IP_1, GATEWAY_IP_2, GATEWAY_IP_3},
        .dns = {8, 8, 8, 8},
        .dhcp = NETINFO_STATIC
    };

    if (sensors_init() != 0) {
        printf("[ERRO] Falha na inicializacao dos sensores. Tarefa Interrompida.\n");
        vTaskDelete(NULL); 
    }

    if (ethernet_init(&eth_config) != 0) {
        printf("[ERRO] Falha na inicialização do Ethernet. Tarefa Interrompida.\n");
        vTaskDelete(NULL);
    }

    printf("[INFO] Iniciando ciclos de envio a cada %d segundos.\n", CYCLE_INTERVAL_MS / 1000);

    sensors_reading_t sensor_data;

    // 2. Loop principal
    while (1) {
        watchdog_update();

        // Passo 1: Ler os dados.
        if (sensors_read_all(&sensor_data) != 0) {
            printf("[ERRO] Falha na leitura dos sensores. Pulando este ciclo.\n");
        } else {
            // Passo 2: Enviar os dados.
            http_status_t status = http_send_sensor_data(
                sensor_data.temperature,
                sensor_data.conductivity,
                sensor_data.flow
            );

            if (status != HTTP_OK) {
                printf("[ERRO] Falha no ciclo de envio (status: %d).\n", status);
            }
        }
        
        // Passo 3: Aguardar o próximo ciclo.
        vTaskDelay(pdMS_TO_TICKS(CYCLE_INTERVAL_MS));
    }
}

int main() {
    stdio_init_all();
    sleep_ms(5000);

    const uint LED_PIN = 25;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);

    watchdog_enable(WATCHDOG_TIMEOUT_MS, 1);

    // Cria a tarefa principal que executará a lógica do dispositivo.
    xTaskCreate(main_task, "MainTask", 2048, NULL, 1, NULL);

    // Inicia o escalonador do FreeRTOS.
    vTaskStartScheduler();

    while(1);
    return 0;
}