/**
 * @file test_02_encoder.c
 * @brief Phase 1 - Test 02: Rotary Encoder Testing
 *
 * OBJECTIVE:
 * - Verify rotary encoder rotation detection
 * - Test encoder direction (CW/CCW)
 * - Test encoder button (also serves as SELECT)
 * - Implement position tracking
 *
 * SUCCESS CRITERIA:
 * - Clockwise rotation increases counter
 * - Counter-clockwise rotation decreases counter
 * - Encoder button press detected (dual function as SELECT)
 * - Smooth operation without skipping or false triggers
 *
 * HARDWARE REQUIRED:
 * - ESP32 development board
 * - KY-040 or similar rotary encoder with button
 *
 * WIRING:
 *   Encoder CLK: GPIO 26
 *   Encoder DT:  GPIO 27
 *   Encoder SW:  GPIO 12 (SELECT button)
 *   Encoder GND: GND
 *   Encoder VCC: 3.3V (if needed)
 *
 * USAGE:
 * 1. Wire encoder as shown above
 * 2. Run: pio run -e test_02_encoder -t upload -t monitor
 * 3. Rotate encoder clockwise and counter-clockwise
 * 4. Press encoder button (SELECT function)
 * 5. Observe position counter and button events
 *
 * Expected Output:
 * ========================================
 * [12345] Position: 1 (CW →)
 * [12456] Position: 2 (CW →)
 * [12789] Position: 1 (CCW ←)
 * [15632] SELECT button PRESSED
 * [15901] SELECT button RELEASED (duration: 269ms)
 * [18450] Position: 2 (CW →)
 * ========================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "../common/pin_definitions.h"

static const char *TAG = "ENCODER_TEST";

// Encoder state structure
typedef struct {
    int32_t position;
    int32_t last_position;
    bool clk_state;
    bool dt_state;
    bool last_clk_state;
} encoder_state_t;

// Button state for encoder switch
typedef struct {
    bool pressed;
    bool last_pressed;
    int64_t press_time;
    uint32_t press_count;
} encoder_button_t;

// Global state
static encoder_state_t encoder = {0};
static encoder_button_t enc_button = {0};

// Queue for encoder events (optional, for ISR-based approach)
static QueueHandle_t encoder_event_queue = NULL;

/**
 * @brief Initialize encoder GPIOs
 */
static void init_encoder(void) {
    // Configure CLK pin
    configure_button_gpio(ENCODER_CLK_PIN);

    // Configure DT pin
    configure_button_gpio(ENCODER_DT_PIN);

    // Configure SW (button) pin
    configure_button_gpio(ENCODER_SW_PIN);

    // Read initial states
    encoder.clk_state = gpio_get_level(ENCODER_CLK_PIN);
    encoder.dt_state = gpio_get_level(ENCODER_DT_PIN);
    encoder.last_clk_state = encoder.clk_state;
    encoder.position = 0;
    encoder.last_position = 0;

    ESP_LOGI(TAG, "Encoder configured:");
    ESP_LOGI(TAG, "  CLK: GPIO %d", ENCODER_CLK_PIN);
    ESP_LOGI(TAG, "  DT:  GPIO %d", ENCODER_DT_PIN);
    ESP_LOGI(TAG, "  SW:  GPIO %d (SELECT button)", ENCODER_SW_PIN);
}

/**
 * @brief Read encoder rotation
 * @return 1 for CW, -1 for CCW, 0 for no change
 */
static int read_encoder(void) {
    // Read current CLK state
    encoder.clk_state = gpio_get_level(ENCODER_CLK_PIN);

    // Check if CLK changed
    if (encoder.clk_state != encoder.last_clk_state) {
        // CLK changed - read DT to determine direction
        encoder.dt_state = gpio_get_level(ENCODER_DT_PIN);

        int direction = 0;
        if (encoder.clk_state == 0) {  // Falling edge of CLK
            if (encoder.dt_state != encoder.clk_state) {
                // DT is HIGH when CLK falls = Clockwise
                direction = 1;
                encoder.position++;
            } else {
                // DT is LOW when CLK falls = Counter-clockwise
                direction = -1;
                encoder.position--;
            }
        }

        encoder.last_clk_state = encoder.clk_state;
        return direction;
    }

    return 0;
}

/**
 * @brief Read encoder button (SELECT function)
 * @return true if button state changed, false otherwise
 */
static bool read_encoder_button(void) {
    // Read button state (active LOW)
    bool pressed = (gpio_get_level(ENCODER_SW_PIN) == 0);

    // Check for state change
    if (pressed != enc_button.last_pressed) {
        // Debounce
        vTaskDelay(pdMS_TO_TICKS(ENCODER_DEBOUNCE_MS));
        pressed = (gpio_get_level(ENCODER_SW_PIN) == 0);

        if (pressed != enc_button.last_pressed) {
            enc_button.last_pressed = pressed;
            enc_button.pressed = pressed;
            return true;
        }
    }

    return false;
}

/**
 * @brief Encoder monitoring task
 */
static void encoder_task(void *pvParameters) {
    ESP_LOGI(TAG, "Encoder monitoring started\n");

    while (1) {
        // Check for rotation
        int direction = read_encoder();
        if (direction != 0) {
            int64_t now = esp_timer_get_time() / 1000;  // Convert to ms

            if (direction > 0) {
                printf("[%lld] Position: %ld (CW →)\n", now, encoder.position);
            } else {
                printf("[%lld] Position: %ld (CCW ←)\n", now, encoder.position);
            }
        }

        // Check for button press
        if (read_encoder_button()) {
            int64_t now = esp_timer_get_time() / 1000;

            if (enc_button.pressed) {
                // Button pressed
                enc_button.press_time = now;
                enc_button.press_count++;
                printf("[%lld] ✓ SELECT button PRESSED (count: %lu) [Position: %ld]\n",
                       now, enc_button.press_count, encoder.position);
            } else {
                // Button released
                int64_t duration = now - enc_button.press_time;
                printf("[%lld] ✗ SELECT button RELEASED (duration: %lldms)\n",
                       now, duration);
            }
        }

        // Small delay
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/**
 * @brief Print test instructions
 */
static void print_instructions(void) {
    printf("\n");
    printf("========================================\n");
    printf("Peristaltic Pump System - Test 02\n");
    printf("Rotary Encoder Test\n");
    printf("========================================\n");
    printf("Hardware Configuration:\n");
    printf("  Encoder CLK: GPIO %d\n", ENCODER_CLK_PIN);
    printf("  Encoder DT:  GPIO %d\n", ENCODER_DT_PIN);
    printf("  Encoder SW:  GPIO %d (SELECT button)\n", ENCODER_SW_PIN);
    printf("\n");
    printf("All pins use internal pull-up resistors\n");
    printf("========================================\n");
    printf("Test Instructions:\n");
    printf("1. Rotate encoder clockwise (CW)\n");
    printf("   - Position should increase: 0 → 1 → 2 → 3...\n");
    printf("2. Rotate encoder counter-clockwise (CCW)\n");
    printf("   - Position should decrease: 3 → 2 → 1 → 0...\n");
    printf("3. Press encoder button (SELECT function)\n");
    printf("   - Should show PRESSED and RELEASED events\n");
    printf("4. Try rotating while holding button\n");
    printf("5. Test rapid rotation for smoothness\n");
    printf("========================================\n");
    printf("Note: Encoder button serves dual purpose:\n");
    printf("  - Navigation: Rotates through menu items\n");
    printf("  - Selection: Press to confirm (SELECT)\n");
    printf("========================================\n\n");
}

/**
 * @brief Print periodic status summary
 */
static void status_task(void *pvParameters) {
    TickType_t last_wake_time = xTaskGetTickCount();

    while (1) {
        // Wait for 10 seconds
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(10000));

        // Print summary if position changed
        if (encoder.position != encoder.last_position || enc_button.press_count > 0) {
            printf("\n--- Status Summary ---\n");
            printf("Current Position: %ld\n", encoder.position);
            printf("Button Presses: %lu\n", enc_button.press_count);
            printf("Free Heap: %lu bytes\n", esp_get_free_heap_size());
            printf("----------------------\n\n");

            encoder.last_position = encoder.position;
        }
    }
}

/**
 * @brief Main application entry point
 */
void app_main(void) {
    // Print instructions
    print_instructions();

    // Initialize encoder
    init_encoder();

    // Create encoder monitoring task (high priority for responsiveness)
    xTaskCreate(encoder_task, "encoder_task", 4096, NULL, 10, NULL);

    // Create status task (low priority)
    xTaskCreate(status_task, "status_task", 2048, NULL, 1, NULL);

    ESP_LOGI(TAG, "All systems ready. Rotate encoder and press button...");
}
