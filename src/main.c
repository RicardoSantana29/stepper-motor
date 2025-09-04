#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "uart_echo.h"
#include "pwm_generator.h" // Incluimos nuestra nueva cabecera PWM

#include "pulse_counter.h" // <-- ¡INCLUIMOS NUESTRO NUEVO MÓDULO!

#include "button_handler.h"

void app_main(void) {
    // Creamos la tarea del eco UART
    xTaskCreate(uart_echo_task, "uart_echo_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);

    // Creamos la tarea del generador PWM
    xTaskCreate(pwm_generator_task, "pwm_generator_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);

    // Creamos la nueva tarea del contador de pulsos
    xTaskCreate(pulse_counter_task, "pulse_counter_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);

    // --- AÑADIDO: Creamos la nueva tarea para el botón ---
    xTaskCreate(button_handler_task, "button_handler_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}