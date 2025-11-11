/**
 * @file pin_definitions.h
 * @brief Hardware pin definitions for Peristaltic Pump Control System
 * @version 1.1 - SAFE BOOT PINS
 * @date 2025-11-11
 *
 * REVISION NOTES:
 * - Moved away from strapping pins (GPIO 0, 2, 5, 12, 15)
 * - All pins now safe for boot sequence
 * - No conflicts with ESP32 boot mode selection
 *
 * Complete pin mapping for ESP32 controlling 4-pump peristaltic system
 * with BTT Rodent board communication via RS485
 */

#ifndef PIN_DEFINITIONS_H
#define PIN_DEFINITIONS_H

#include "driver/gpio.h"

// ============================================================================
// RS485 COMMUNICATION TO BTT RODENT BOARD
// ============================================================================
#define RODENT_TX_PIN       GPIO_NUM_17     // UART2 TX -> RS485 DI (was GPIO 2)
#define RODENT_RX_PIN       GPIO_NUM_16     // UART2 RX -> RS485 RO (was GPIO 4)
#define RODENT_RTS_PIN      GPIO_NUM_4      // RS485 Direction Control (was GPIO 15)
#define RODENT_UART_NUM     UART_NUM_2      // Changed to UART2 (was UART1)
#define RODENT_BAUD_RATE    115200          // Baud rate for RS485/FluidNC

// Note: RS485 requires MAX485 or similar transceiver module
// AUTO-DIRECTION MODULE: Set RS485_AUTO_DIRECTION to skip RTS control
// #define RS485_AUTO_DIRECTION

// ============================================================================
// RS232 COMMUNICATION TO DIGITAL SCALE
// ============================================================================
#define SCALE_RX_PIN        GPIO_NUM_35     // UART1 RX <- MAX3232 R1OUT (was GPIO 16)
#define SCALE_TX_PIN        GPIO_NUM_32     // UART1 TX -> MAX3232 T1IN (was GPIO 17)
#define SCALE_UART_NUM      UART_NUM_1      // Changed to UART1 (was UART2)
#define SCALE_BAUD_RATE     9600            // Baud rate for scale

// Note: Scale uses RS232 (±12V) - MAX3232 converter module REQUIRED
// NEVER connect RS232 directly to ESP32!
// GPIO 35 is INPUT ONLY - perfect for RX, cannot be used for TX

// ============================================================================
// I2C BUS - LCD DISPLAY
// ============================================================================
#define LCD_SDA_PIN         GPIO_NUM_21     // I2C Data (safe, no change)
#define LCD_SCL_PIN         GPIO_NUM_22     // I2C Clock (safe, no change)
#define LCD_I2C_NUM         I2C_NUM_0       // I2C Port 0
#define LCD_I2C_ADDR        0x27            // LCD I2C Address (or 0x3F)
#define LCD_I2C_FREQ        100000          // 100kHz I2C frequency

// ============================================================================
// WS2812B LED STRIPS
// ============================================================================
#define LED_DATA_PIN        GPIO_NUM_25     // LED Data (RMT channel) (safe, no change)
#define LED_STRIP_COUNT     4               // 4 LED strips (one per pump)
#define LED_PER_STRIP       8               // 8 LEDs per strip
#define LED_TOTAL_COUNT     32              // Total 32 LEDs

// LED Strip Mapping (in series):
// Strip 0 (LEDs 0-7):   Pump 1 - DMDEE  (Cyan)
// Strip 1 (LEDs 8-15):  Pump 2 - T-12   (Magenta)
// Strip 2 (LEDs 16-23): Pump 3 - T-9    (Yellow)
// Strip 3 (LEDs 24-31): Pump 4 - L25B   (White)

// ============================================================================
// CONTROL BUTTONS (Active LOW with Internal Pull-up)
// ============================================================================
#define START_BUTTON_PIN    GPIO_NUM_13     // Start operation (safe, no change)
#define MODE_BUTTON_PIN     GPIO_NUM_14     // Mode selection (safe, no change)
#define STOP_BUTTON_PIN     GPIO_NUM_33     // Emergency stop (safe, no change)

// All buttons: Normally Open (NO), one side to GPIO, other to GND
// Internal pull-up enabled, read LOW when pressed

// ============================================================================
// ROTARY ENCODER
// ============================================================================
#define ENCODER_CLK_PIN     GPIO_NUM_26     // Encoder Clock (A) (safe, no change)
#define ENCODER_DT_PIN      GPIO_NUM_27     // Encoder Data (B) (safe, no change)
#define ENCODER_SW_PIN      GPIO_NUM_34     // Encoder Switch/SELECT (was GPIO 12)

// Encoder switch also serves as SELECT button
// GPIO 34 is INPUT ONLY - perfect for button, no boot interference
// Internal pull-up enabled, read LOW when pressed

// ============================================================================
// BUILT-IN LED (for basic testing)
// ============================================================================
#define BUILTIN_LED_PIN     GPIO_NUM_2      // ESP32 built-in LED
// Note: ONLY use for Test 00 (blink). After that, GPIO 2 should be avoided.
// For production, use LED strips or external LED instead.

// ============================================================================
// BOOT-SAFE PIN NOTES
// ============================================================================
// SAFE INPUT PINS (no boot interference):
//   GPIO 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
//   GPIO 34, 35, 36, 39 (INPUT ONLY - cannot be outputs)
//
// STRAPPING PINS (AVOID OR USE WITH CAUTION):
//   GPIO 0  - Must be HIGH for normal boot (LOW = download mode)
//   GPIO 2  - Must be LOW/floating for normal boot
//   GPIO 5  - Timing of SDIO slave (usually safe, but can be sensitive)
//   GPIO 12 - Must be LOW for 3.3V flash (most boards)
//   GPIO 15 - Should be HIGH for normal boot (LOW = no boot messages)
//
// OUR CHOICES:
//   ✅ Moved RS485 from GPIO 2/4/15 → GPIO 17/16/4 (all safe)
//   ✅ Moved Scale from GPIO 16/17 → GPIO 35/32 (all safe)
//   ✅ Moved Encoder SW from GPIO 12 → GPIO 34 (input-only, boot-safe)
//   ✅ Kept safe GPIOs: 13, 14, 21, 22, 25, 26, 27, 33

// ============================================================================
// PUMP TO AXIS MAPPING (on BTT Rodent Board)
// ============================================================================
// Pump 1 (DMDEE) -> X-axis -> Rodent Motor Driver 1
// Pump 2 (T-12)  -> Y-axis -> Rodent Motor Driver 2
// Pump 3 (T-9)   -> Z-axis -> Rodent Motor Driver 3
// Pump 4 (L25B)  -> A-axis -> Rodent Motor Driver 4

// ============================================================================
// TIMING CONSTANTS
// ============================================================================
#define BUTTON_DEBOUNCE_MS      50          // Button debounce delay
#define ENCODER_DEBOUNCE_MS     5           // Encoder debounce delay
#define RS485_SWITCH_DELAY_US   100         // RS485 transceiver switch delay
#define UART_TIMEOUT_MS         1000        // UART read timeout

// ============================================================================
// SAFETY LIMITS
// ============================================================================
#define MIN_FLOW_RATE_ML_MIN    1.0         // Minimum flow rate
#define MAX_FLOW_RATE_ML_MIN    500.0       // Maximum flow rate
#define MIN_FEEDRATE_MM_MIN     10.0        // Minimum G-code feedrate
#define MAX_FEEDRATE_MM_MIN     5000.0      // Maximum G-code feedrate

// ============================================================================
// GPIO CONFIGURATION HELPERS
// ============================================================================

// Configure button GPIO (input with pull-up)
static inline void configure_button_gpio(gpio_num_t pin) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

// Configure input-only GPIO (for GPIO 34, 35, 36, 39)
// These pins cannot have pull-up/pull-down enabled - use external resistor if needed
static inline void configure_input_only_gpio(gpio_num_t pin) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,   // Not available on input-only pins
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

// Configure output GPIO
static inline void configure_output_gpio(gpio_num_t pin) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

// ============================================================================
// REVISION SUMMARY
// ============================================================================
/*
 * Version 1.1 Changes (Boot-Safe Pins):
 *
 * OLD PIN MAPPING (v1.0):
 *   RS485 TX:      GPIO 2  (STRAPPING PIN - boot issue!)
 *   RS485 RX:      GPIO 4
 *   RS485 RTS:     GPIO 15 (STRAPPING PIN - boot message control)
 *   Scale RX:      GPIO 16
 *   Scale TX:      GPIO 17
 *   Encoder SW:    GPIO 12 (STRAPPING PIN - flash voltage!)
 *
 * NEW PIN MAPPING (v1.1 - BOOT SAFE):
 *   RS485 TX:      GPIO 17 ✅ Safe for boot
 *   RS485 RX:      GPIO 16 ✅ Safe for boot
 *   RS485 RTS:     GPIO 4  ✅ Safe for boot
 *   Scale RX:      GPIO 35 ✅ Safe for boot (input-only, perfect for RX)
 *   Scale TX:      GPIO 32 ✅ Safe for boot
 *   Encoder SW:    GPIO 34 ✅ Safe for boot (input-only, perfect for button)
 *
 * Why These Changes:
 *   - Eliminated all strapping pin conflicts (GPIO 2, 12, 15)
 *   - Used input-only pins (34, 35) for read-only signals (buttons, RX)
 *   - Swapped UART assignments: Rodent now on UART2, Scale on UART1
 *   - All new pins have no boot sequence interference
 *   - ESP32 will boot reliably regardless of peripheral states
 */

#endif // PIN_DEFINITIONS_H
