/**
 * @file test_01_buttons.c
 * @brief Phase 1 - Test 01: Push Button Testing
 *
 * OBJECTIVE:
 * - Verify all 3 push buttons are wired correctly
 * - Test internal pull-up resistors
 * - Implement software debouncing
 *
 * SUCCESS CRITERIA:
 * - START button (GPIO 13) detected correctly
 * - MODE button (GPIO 14) detected correctly
 * - STOP button (GPIO 33) detected correctly
 * - No false triggers (clean debouncing)
 * - Press and release events tracked
 *
 * HARDWARE REQUIRED:
 * - ESP32 development board
 * - 3x Push buttons (Normally Open)
 * - Wiring: Button between GPIO and GND (no external resistors needed)
 *
 * WIRING:
 *   START Button: GPIO 13 <-> [Button] <-> GND
 *   MODE Button:  GPIO 14 <-> [Button] <-> GND
 *   STOP Button:  GPIO 33 <-> [Button] <-> GND
 *
 * USAGE:
 * 1. Wire buttons as shown above
 * 2. Run: pio run -e test_01_buttons -t upload -t monitor
 * 3. Press each button and observe serial output
 * 4. Try rapid presses to test debouncing
 *
 * Expected Output:
 * ========================================
 * [12345] START button PRESSED
 * [12789] START button RELEASED (duration: 444ms)
 * [15632] MODE button PRESSED
 * [15901] MODE button RELEASED (duration: 269ms)
 * [18450] STOP button PRESSED
 * [20001] STOP button RELEASED (duration: 1551ms)
 * ========================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "../common/pin_definitions.h"

static const char *TAG = "BUTTON_TEST";

// Button state structure
typedef struct {
    gpio_num_t pin;
    const char *name;
    bool current_state;
    bool last_state;
    int64_t press_time;
    int64_t release_time;
    uint32_t press_count;
} button_t;

// Button instances
static button_t buttons[] = {
    {.pin = START_BUTTON_PIN, .name = "START", .current_state = true, .last_state = true, .press_time = 0, .release_time = 0, .press_count = 0},
    {.pin = MODE_BUTTON_PIN,  .name = "MODE",  .current_state = true, .last_state = true, .press_time = 0, .release_time = 0, .press_count = 0},
    {.pin = STOP_BUTTON_PIN,  .name = "STOP",  .current_state = true, .last_state = true, .press_time = 0, .release_time = 0, .press_count = 0}
};

#define NUM_BUTTONS (sizeof(buttons) / sizeof(buttons[0]))

/**
 * @brief Initialize all button GPIOs
 */
static void init_buttons(void) {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        configure_button_gpio(buttons[i].pin);
        ESP_LOGI(TAG, "Configured %s button on GPIO %d", buttons[i].name, buttons[i].pin);
    }
}

/**
 * @brief Read and debounce button state
 * @param btn Pointer to button structure
 * @return true if state changed, false otherwise
 */
static bool read_button(button_t *btn) {
    // Read raw GPIO state (LOW = pressed due to pull-up)
    int raw_state = gpio_get_level(btn->pin);
    bool pressed = (raw_state == 0);  // Active LOW

    // Update current state
    btn->current_state = pressed;

    // Check for state change
    if (btn->current_state != btn->last_state) {
        // Simple debouncing: wait and re-read
        vTaskDelay(pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS));
        raw_state = gpio_get_level(btn->pin);
        pressed = (raw_state == 0);

        if (pressed == btn->current_state) {
            // State change confirmed
            btn->last_state = btn->current_state;
            return true;
        }
    }

    return false;
}

/**
 * @brief Button monitoring task
 */
static void button_task(void *pvParameters) {
    ESP_LOGI(TAG, "Button monitoring started");
    ESP_LOGI(TAG, "Press buttons to test...\n");

    while (1) {
        for (int i = 0; i < NUM_BUTTONS; i++) {
            button_t *btn = &buttons[i];

            if (read_button(btn)) {
                int64_t now = esp_timer_get_time() / 1000;  // Convert to ms

                if (btn->current_state) {
                    // Button pressed
                    btn->press_time = now;
                    btn->press_count++;
                    printf("[%lld] ✓ %s button PRESSED (count: %lu)\n",
                           now, btn->name, btn->press_count);
                } else {
                    // Button released
                    btn->release_time = now;
                    int64_t duration = btn->release_time - btn->press_time;
                    printf("[%lld] ✗ %s button RELEASED (duration: %lldms)\n",
                           now, btn->name, duration);
                }
            }
        }

        // Small delay to prevent CPU hogging
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief Print test instructions
 */
static void print_instructions(void) {
    printf("\n");
    printf("========================================\n");
    printf("Peristaltic Pump System - Test 01\n");
    printf("Push Button Test\n");
    printf("========================================\n");
    printf("Hardware Configuration:\n");
    printf("  START button: GPIO %d\n", START_BUTTON_PIN);
    printf("  MODE button:  GPIO %d\n", MODE_BUTTON_PIN);
    printf("  STOP button:  GPIO %d\n", STOP_BUTTON_PIN);
    printf("\n");
    printf("All buttons use internal pull-up resistors\n");
    printf("Active LOW: Pressed = LOW, Released = HIGH\n");
    printf("========================================\n");
    printf("Test Instructions:\n");
    printf("1. Press and release START button\n");
    printf("2. Press and release MODE button\n");
    printf("3. Press and release STOP button\n");
    printf("4. Try holding buttons for different durations\n");
    printf("5. Try rapid presses to test debouncing\n");
    printf("========================================\n\n");
}

/**
 * @brief Main application entry point
 */
void app_main(void) {
    // Print instructions
    print_instructions();

    // Initialize buttons
    init_buttons();

    // Create button monitoring task
    xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "All systems ready. Monitoring buttons...");
}
