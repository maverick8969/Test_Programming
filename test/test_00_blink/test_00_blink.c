/**
 * @file test_00_blink.c
 * @brief Phase 1 - Test 00: Blink and Serial Output
 *
 * OBJECTIVE:
 * - Verify ESP32 can be programmed
 * - Test built-in LED
 * - Verify serial communication at 115200 baud
 *
 * SUCCESS CRITERIA:
 * - Built-in LED blinks every 1 second
 * - Serial monitor shows "Blink Test - Running..." messages
 * - No errors during upload
 *
 * HARDWARE REQUIRED:
 * - ESP32 development board
 * - USB cable
 *
 * USAGE:
 * 1. Connect ESP32 via USB
 * 2. Run: pio run -e test_00_blink -t upload -t monitor
 * 3. Observe LED blinking and serial output
 *
 * Expected Output:
 * ========================================
 * Peristaltic Pump System - Test 00
 * Blink Test
 * ========================================
 * ESP-IDF Version: v5.x
 * Chip: ESP32
 * Cores: 2
 * ========================================
 * [0] LED: ON  | Free Heap: 295000 bytes
 * [1] LED: OFF | Free Heap: 295000 bytes
 * [2] LED: ON  | Free Heap: 295000 bytes
 * ========================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "sdkconfig.h"

// Pin definition
#define BLINK_GPIO GPIO_NUM_2

// Tag for logging
static const char *TAG = "BLINK_TEST";

/**
 * @brief Print system information
 */
void print_system_info(void) {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    printf("========================================\n");
    printf("Peristaltic Pump System - Test 00\n");
    printf("Blink Test\n");
    printf("========================================\n");
    printf("ESP-IDF Version: %s\n", esp_get_idf_version());
    printf("Chip: %s\n", CONFIG_IDF_TARGET);
    printf("Cores: %d\n", chip_info.cores);
    printf("Silicon Revision: %d\n", chip_info.revision);
    printf("Flash: %dMB %s\n",
           spi_flash_get_chip_size() / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    printf("========================================\n");
}

/**
 * @brief Main application entry point
 */
void app_main(void) {
    // Print system information
    print_system_info();

    // Configure GPIO for LED
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BLINK_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Blink test started. LED on GPIO %d", BLINK_GPIO);
    ESP_LOGI(TAG, "Press Ctrl+] to exit monitor");
    printf("\n");

    // Main blink loop
    uint32_t counter = 0;
    bool led_state = false;

    while (1) {
        // Toggle LED
        led_state = !led_state;
        gpio_set_level(BLINK_GPIO, led_state);

        // Print status
        printf("[%lu] LED: %-3s | Free Heap: %lu bytes\n",
               counter,
               led_state ? "ON" : "OFF",
               esp_get_free_heap_size());

        // Delay 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));

        counter++;
    }
}
