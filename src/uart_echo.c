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
    //ESP_LOGI(TAG, "Envía 'PULSOS <cantidad>' para generar pulsos. Ej: PULSOS 50");
    ESP_LOGI(TAG, "Envía 'PULSOS <cantidad> <I/D>' para generar pulsos. Ej: PULSOS 50 I");

    while (1) {
        int len = uart_read_bytes(UART_PORT, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0';
            ESP_LOGI(TAG, "Recibido: %s", (char *) data);
            //uart_write_bytes(UART_PORT, (const char *) data, len); // Eco de lo recibido

             // 1. Extraemos la primera palabra (debería ser "PULSOS")
            char *cmd_token = strtok((char *)data, " ");
            if (cmd_token != NULL && strcmp(cmd_token, "PULSOS") == 0) {
                
                // 2. Extraemos la segunda palabra (el número de pulsos)
                char *num_token = strtok(NULL, " ");
                // 3. --- AÑADIDO --- Extraemos la tercera palabra (la dirección)
                // Usamos " \r\n" como delimitadores para limpiar posibles saltos de línea del final.
                char *dir_token = strtok(NULL, " \r\n");

                // 4. --- AÑADIDO --- Verificamos que hemos recibido las 3 partes del comando
                if (num_token != NULL && dir_token != NULL) {
                    int num_pulses = atoi(num_token);
                    
                    // 5. --- AÑADIDO --- Validamos que los argumentos son correctos
                    // El número de pulsos debe ser positivo Y la dirección debe ser "I" o "D".
                    if (num_pulses > 0 && (strcmp(dir_token, "I") == 0 || strcmp(dir_token, "D") == 0)) {
                        
                        // 6. --- CAMBIO --- Creamos la nueva estructura de comando y la rellenamos
                        pwm_command_t command;
                        command.num_pulses = num_pulses;
                        command.direction = dir_token[0]; // Guardamos el primer carácter ('I' o 'D')
                        
                        // Enviamos el comando completo a la cola
                        if (xQueueSend(pwm_command_queue, &command, portMAX_DELAY) != pdPASS) {
                            ESP_LOGE(TAG, "No se pudo enviar el comando a la cola");
                        } else {
                            // --- CAMBIO --- El mensaje de log ahora incluye la dirección
                            ESP_LOGI(TAG, "Comando enviado: %d pulsos, Dirección: %c", command.num_pulses, command.direction);
                        }
                    } else {
                        // --- AÑADIDO --- Mensaje de error si los argumentos son inválidos
                        uart_write_bytes(UART_PORT, "Error: Argumentos inválidos.\r\n", strlen("Error: Argumentos inválidos.\r\n"));
                    }
                } else {
                    // --- AÑADIDO --- Mensaje de error si faltan argumentos
                    uart_write_bytes(UART_PORT, "Error: Faltan argumentos. Formato: PULSOS <numero> <I/D>\r\n", strlen("Error: Faltan argumentos. Formato: PULSOS <numero> <I/D>\r\n"));
                }
            } else {
                 uart_write_bytes(UART_PORT, "Comando desconocido.\r\n", strlen("Comando desconocido.\r\n"));
            }
        }
    }
}