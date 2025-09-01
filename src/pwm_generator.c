#include "pwm_generator.h"

static const char *TAG = "PWM_GENERATOR";

// Configuración del LEDC (PWM)
static void ledc_init(void) {
    // Configuración del temporizador
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY, // Frecuencia de 1 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Configuración del canal PWM
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Inicia con el PWM apagado
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ESP_LOGI(TAG, "LEDC configurado en GPIO %d a %d Hz", LEDC_OUTPUT_IO, LEDC_FREQUENCY);
}

void pwm_generator_task(void *arg) {
    ledc_init(); // Inicializa el módulo LEDC

    pwm_command_t received_command;
    int current_duty = 0; // 0 = apagado, 511 = 50% ciclo de trabajo para 10-bit

    while (1) {
        // Espera por un comando en la cola
        if (xQueueReceive(pwm_command_queue, &received_command, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG, "Recibido comando: Generar %d pulsos", received_command.num_pulses);

            // Genera los pulsos
            if (received_command.num_pulses > 0) {
                // Establece un ciclo de trabajo del 50%
                current_duty = (1 << LEDC_DUTY_RES) / 2; // (2^10) / 2 = 1024 / 2 = 512
                ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, current_duty));
                ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
                ESP_LOGI(TAG, "PWM ENCENDIDO con ciclo de trabajo %d", current_duty);

                // Espera por la cantidad de pulsos.
                // Con 1 kHz, un pulso dura 1 ms.
                // Delay = num_pulses * (1000ms / 1000Hz) = num_pulses * 1ms
                vTaskDelay(pdMS_TO_TICKS(received_command.num_pulses));

                // Apaga el PWM después de generar los pulsos
                ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0));
                ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
                ESP_LOGI(TAG, "PWM APAGADO");
            } else {
                // Si se envía 0 o un número negativo, apaga el PWM
                ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0));
                ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
                ESP_LOGI(TAG, "Comando para apagar PWM recibido.");
            }
        }
    }
}