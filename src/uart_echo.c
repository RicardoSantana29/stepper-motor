#include <stdio.h>
#include <string.h>
#include <stdlib.h> // Para atoi
#include "uart_echo.h"
#include "freertos/queue.h" // Necesario para crear la cola

static const char *TAG = "UART_ECHO";

// Definimos la cola de comandos
QueueHandle_t pwm_command_queue;

void uart_echo_task(void *arg) {
    // Creamos la cola aquí. ¡Es importante que se cree antes de que ambas tareas intenten usarla!
    // El tamaño de la cola determina cuántos comandos puede almacenar antes de bloquearse
    pwm_command_queue = xQueueCreate(10, sizeof(pwm_command_t));
    if (pwm_command_queue == NULL) {
        ESP_LOGE(TAG, "No se pudo crear la cola de comandos PWM");
        vTaskDelete(NULL);
    }

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    if (data == NULL) {
        ESP_LOGE(TAG, "No se pudo asignar memoria para el buffer UART");
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "Tarea de eco UART iniciada en el puerto %d", UART_PORT);
    ESP_LOGI(TAG, "Envía 'PULSOS <cantidad>' para generar pulsos. Ej: PULSOS 50");

    while (1) {
        int len = uart_read_bytes(UART_PORT, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0';
            ESP_LOGI(TAG, "Recibido: %s", (char *) data);
            uart_write_bytes(UART_PORT, (const char *) data, len); // Eco de lo recibido

            // Parsear el comando
            char *token = strtok((char *)data, " ");
            if (token != NULL && strcmp(token, "PULSOS") == 0) {
                token = strtok(NULL, " ");
                if (token != NULL) {
                    int num_pulses = atoi(token);
                    if (num_pulses > 0) {
                        pwm_command_t command;
                        command.num_pulses = num_pulses;
                        
                        // Enviar el comando a la cola
                        if (xQueueSend(pwm_command_queue, &command, portMAX_DELAY) != pdPASS) {
                            ESP_LOGE(TAG, "No se pudo enviar el comando PWM a la cola");
                        } else {
                            ESP_LOGI(TAG, "Comando enviado: Generar %d pulsos", num_pulses);
                            uart_write_bytes(UART_PORT, "Comando recibido para generar pulsos.\r\n", strlen("Comando recibido para generar pulsos.\r\n"));
                        }
                    } else {
                        ESP_LOGW(TAG, "Cantidad de pulsos inválida: %d", num_pulses);
                        uart_write_bytes(UART_PORT, "Cantidad de pulsos invalida. Usa 'PULSOS <numero>'.\r\n", strlen("Cantidad de pulsos invalida. Usa 'PULSOS <numero>'.\r\n"));
                    }
                } else {
                    ESP_LOGW(TAG, "Falta la cantidad de pulsos.");
                    uart_write_bytes(UART_PORT, "Falta la cantidad de pulsos. Usa 'PULSOS <numero>'.\r\n", strlen("Falta la cantidad de pulsos. Usa 'PULSOS <numero>'.\r\n"));
                }
            } else {
                 uart_write_bytes(UART_PORT, "Comando desconocido. Usa 'PULSOS <numero>'.\r\n", strlen("Comando desconocido. Usa 'PULSOS <numero>'.\r\n"));
            }
        }
    }
}