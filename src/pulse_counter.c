// src/pulse_counter.c

#include "pulse_counter.h" // Incluimos nuestra propia cabecera
#include <stdio.h>
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"

// --- Definiciones privadas del módulo ---
#define PULSE_GPIO_NUM      GPIO_NUM_27 // Pin donde se reciben los pulsos
static const char *TAG = "PULSE_COUNTER";

// --- Definición de la variable global ---
// Esta es la definición real de la variable declarada como 'extern' en el .h
volatile int g_pulse_count = 0;

// --- Funciones privadas del módulo ---

/**
 * @brief Rutina de servicio de interrupción (ISR).
 * Se ejecuta cada vez que se detecta un flanco de subida en el GPIO.
 * ¡Debe ser lo más rápida posible!
 */
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    // Simplemente incrementamos el contador.
    g_pulse_count++;
}

/**
 * @brief Función principal de la tarea de conteo de pulsos.
 */
void pulse_counter_task(void *arg) {
    ESP_LOGI(TAG, "Iniciando tarea de contador de pulsos en GPIO%d", PULSE_GPIO_NUM);

    // 1. Configurar el pin GPIO para interrupciones
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,    // Interrupción en flanco de subida (positivo)
        .mode = GPIO_MODE_INPUT,           // Pin como entrada
        .pin_bit_mask = (1ULL << PULSE_GPIO_NUM),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,  // Habilitar pull-up interno
    };
    gpio_config(&io_conf);

    // 2. Instalar el servicio de interrupción del GPIO (una sola vez por aplicación)
    // Usamos ESP_INTR_FLAG_IRAM para que la ISR se ejecute desde la RAM, siendo más rápida.
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

    // 3. Agregar el manejador de interrupción para nuestro pin específico
    gpio_isr_handler_add(PULSE_GPIO_NUM, gpio_isr_handler, (void*) PULSE_GPIO_NUM);

    ESP_LOGI(TAG, "Configuración completa. Esperando pulsos...");

    // Bucle principal de la tarea
    while (1) {
        // Esta tarea ahora solo se encarga de mostrar la información
        // periódicamente. El conteo real ocurre en segundo plano por hardware.
        ESP_LOGI(TAG, "Pulsos contados totales: %d", g_pulse_count);
        
        // Esperamos 5 segundos antes de volver a imprimir
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}