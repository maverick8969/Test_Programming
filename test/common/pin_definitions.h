/**
 * @file pin_definitions.h
 * @brief Hardware pin definitions for Peristaltic Pump Control System
 * @version 1.0
 * @date 2025-11-11
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
#define RODENT_TX_PIN       GPIO_NUM_2      // UART1 TX -> RS485 DI
#define RODENT_RX_PIN       GPIO_NUM_4      // UART1 RX -> RS485 RO
#define RODENT_RTS_PIN      GPIO_NUM_15     // RS485 Direction Control (DE/RE)
#define RODENT_UART_NUM     UART_NUM_1      // UART1 for Rodent communication
#define RODENT_BAUD_RATE    115200          // Baud rate for RS485/FluidNC

// Note: RS485 requires MAX485 or similar transceiver module
// AUTO-DIRECTION MODULE: Set RS485_AUTO_DIRECTION to skip RTS control
// #define RS485_AUTO_DIRECTION

// ============================================================================
// RS232 COMMUNICATION TO DIGITAL SCALE
// ============================================================================
#define SCALE_RX_PIN        GPIO_NUM_16     // UART2 RX <- MAX3232 R1OUT
#define SCALE_TX_PIN        GPIO_NUM_17     // UART2 TX -> MAX3232 T1IN
#define SCALE_UART_NUM      UART_NUM_2      // UART2 for Scale communication
#define SCALE_BAUD_RATE     9600            // Baud rate for scale

// Note: Scale uses RS232 (±12V) - MAX3232 converter module REQUIRED
// NEVER connect RS232 directly to ESP32!

// ============================================================================
// I2C BUS - LCD DISPLAY
// ============================================================================
#define LCD_SDA_PIN         GPIO_NUM_21     // I2C Data
#define LCD_SCL_PIN         GPIO_NUM_22     // I2C Clock
#define LCD_I2C_NUM         I2C_NUM_0       // I2C Port 0
#define LCD_I2C_ADDR        0x27            // LCD I2C Address (or 0x3F)
#define LCD_I2C_FREQ        100000          // 100kHz I2C frequency

// ============================================================================
// WS2812B LED STRIPS
// ============================================================================
#define LED_DATA_PIN        GPIO_NUM_25     // LED Data (RMT channel)
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
#define START_BUTTON_PIN    GPIO_NUM_13     // Start operation
#define MODE_BUTTON_PIN     GPIO_NUM_14     // Mode selection
#define STOP_BUTTON_PIN     GPIO_NUM_33     // Emergency stop

// All buttons: Normally Open (NO), one side to GPIO, other to GND
// Internal pull-up enabled, read LOW when pressed

// ============================================================================
// ROTARY ENCODER
// ============================================================================
#define ENCODER_CLK_PIN     GPIO_NUM_26     // Encoder Clock (A)
#define ENCODER_DT_PIN      GPIO_NUM_27     // Encoder Data (B)
#define ENCODER_SW_PIN      GPIO_NUM_12     // Encoder Switch (SELECT button)

// Encoder switch also serves as SELECT button
// Internal pull-up enabled, read LOW when pressed

// ============================================================================
// BUILT-IN LED (for basic testing)
// ============================================================================
#define BUILTIN_LED_PIN     GPIO_NUM_2      // ESP32 built-in LED
// Note: Shared with RODENT_TX_PIN - use only for basic blink test

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

#endif // PIN_DEFINITIONS_H
