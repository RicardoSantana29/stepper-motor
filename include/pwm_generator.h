#ifndef PWM_GENERATOR_H
#define PWM_GENERATOR_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h" // Módulo LEDC para PWM
#include "esp_log.h"
#include "uart_echo.h" // Para incluir la estructura pwm_command_t y la cola

// Definiciones para el PWM
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (GPIO_NUM_32) // Elige el GPIO que quieras para la salida PWM
#define LEDC_DIRECTION_IO       (GPIO_NUM_33) // Pin de salida para la dirección
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_10_BIT // Resolución del ciclo de trabajo (0-1023)
#define LEDC_FREQUENCY          (1000) // 1 kHz

// Declaración de la tarea del generador PWM
void pwm_generator_task(void *arg);

#endif // PWM_GENERATOR_H