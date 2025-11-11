/**
 * @file main.c
 * @brief Main application placeholder
 *
 * This is the main component required by ESP-IDF.
 * For Phase 1 testing, we use separate test environments.
 *
 * This file will be populated in Phase 7 with the complete application.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void) {
    printf("Peristaltic Pump System - Main Application\n");
    printf("This is a placeholder. Use test environments for Phase 1:\n");
    printf("  pio run -e test_00_blink -t upload -t monitor\n");
    printf("  pio run -e test_01_buttons -t upload -t monitor\n");
    printf("  pio run -e test_02_encoder -t upload -t monitor\n");

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
