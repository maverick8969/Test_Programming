/**
 * @file test_04_lcd.cpp
 * @brief Phase 2 - Test 04: LCD Display (Arduino)
 *
 * OBJECTIVE:
 * - Initialize 1602 LCD with I2C interface
 * - Display text on 16×2 character display
 * - Test backlight control
 * - Verify display updates
 *
 * SUCCESS CRITERIA:
 * - LCD initializes successfully
 * - Text displays on Line 1 and Line 2
 * - Characters are readable (adjust contrast if needed)
 * - Display updates with changing information
 * - Backlight control works
 *
 * HARDWARE REQUIRED:
 * - ESP32 development board
 * - 1602 LCD with I2C backpack (PCF8574 or similar)
 * - Found address from Test 03 (typically 0x27 or 0x3F)
 *
 * WIRING:
 *   LCD SDA → GPIO 21
 *   LCD SCL → GPIO 22
 *   LCD VCC → 5V
 *   LCD GND → GND
 *
 * USAGE:
 * 1. Run Test 03 first to find LCD address
 * 2. Update LCD_I2C_ADDR in pin_definitions.h if needed
 * 3. Run: pio run -e test_04_lcd -t upload -t monitor
 * 4. Adjust contrast potentiometer on LCD module if text not visible
 * 5. Observe text display and updates
 */

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "pin_definitions.h"

// Initialize LCD (address from pin_definitions.h, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 16, 2);

// Display update interval
const unsigned long UPDATE_INTERVAL = 2000; // 2 seconds
unsigned long lastUpdateTime = 0;
int displayMode = 0;
const int NUM_DISPLAY_MODES = 4;

/**
 * @brief Display mode 0: Welcome message
 */
void displayWelcome() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Pump Controller");
    lcd.setCursor(0, 1);
    lcd.print("LCD Test OK!");
}

/**
 * @brief Display mode 1: System info
 */
void displaySystemInfo() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ESP32 Ready");
    lcd.setCursor(0, 1);
    lcd.print("CPU:");
    lcd.print(ESP.getCpuFreqMHz());
    lcd.print("MHz");
}

/**
 * @brief Display mode 2: Memory info
 */
void displayMemoryInfo() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Free Heap:");
    lcd.setCursor(0, 1);
    lcd.print(ESP.getFreeHeap() / 1024);
    lcd.print(" KB");
}

/**
 * @brief Display mode 3: Uptime
 */
void displayUptime() {
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    seconds = seconds % 60;
    minutes = minutes % 60;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Uptime:");
    lcd.setCursor(0, 1);

    if (hours > 0) {
        lcd.print(hours);
        lcd.print("h ");
    }
    lcd.print(minutes);
    lcd.print("m ");
    lcd.print(seconds);
    lcd.print("s");
}

/**
 * @brief Test backlight on/off
 */
void testBacklight() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Backlight Test");

    // Blink backlight 3 times
    for (int i = 0; i < 3; i++) {
        lcd.setCursor(0, 1);
        lcd.print("OFF in ");
        lcd.print(3 - i);
        lcd.print("...");
        delay(1000);
    }

    lcd.noBacklight();
    delay(2000);
    lcd.backlight();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Backlight: OK");
    delay(2000);
}

/**
 * @brief Test all characters
 */
void testCharacters() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("All Characters:");

    delay(2000);

    // Display ASCII characters
    for (char c = 32; c < 127; c++) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ASCII: ");
        lcd.print((int)c);
        lcd.setCursor(0, 1);

        // Show character multiple times
        for (int i = 0; i < 16; i++) {
            lcd.print(c);
        }

        delay(200);
    }
}

void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(100);

    Serial.println("\n========================================");
    Serial.println("Peristaltic Pump System - Test 04");
    Serial.println("LCD Display Test");
    Serial.println("========================================");
    Serial.println("Hardware Configuration:");
    Serial.print("  LCD Address: 0x");
    Serial.print(LCD_I2C_ADDR, HEX);
    Serial.print(" (");
    Serial.print(LCD_I2C_ADDR);
    Serial.println(")");
    Serial.print("  SDA: GPIO ");
    Serial.println(LCD_SDA_PIN);
    Serial.print("  SCL: GPIO ");
    Serial.println(LCD_SCL_PIN);
    Serial.println("  Size: 16×2 characters");
    Serial.println("========================================\n");

    // Initialize I2C
    Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN, LCD_I2C_FREQ);
    Serial.println("I2C initialized");

    // Initialize LCD
    Serial.println("Initializing LCD...");
    lcd.init();
    lcd.backlight();

    Serial.println("✓ LCD initialized successfully!");
    Serial.println();
    Serial.println("If text is not visible:");
    Serial.println("  - Adjust contrast potentiometer on LCD module");
    Serial.println("  - Usually located on I2C backpack");
    Serial.println("  - Turn slowly until text appears clearly");
    Serial.println();
    Serial.println("Display will cycle through modes:");
    Serial.println("  1. Welcome message");
    Serial.println("  2. System info");
    Serial.println("  3. Memory info");
    Serial.println("  4. Uptime");
    Serial.println("========================================\n");

    // Display welcome
    displayWelcome();
    delay(3000);

    // Test backlight
    Serial.println("Testing backlight...");
    testBacklight();
    Serial.println("✓ Backlight test complete\n");

    // Start cycling modes
    lastUpdateTime = millis();

    Serial.println("Starting display cycling...");
    Serial.println("Display updates every 2 seconds\n");
}

void loop() {
    // Cycle through display modes
    if (millis() - lastUpdateTime >= UPDATE_INTERVAL) {
        lastUpdateTime = millis();

        switch (displayMode) {
            case 0:
                Serial.println("Display: Welcome message");
                displayWelcome();
                break;
            case 1:
                Serial.println("Display: System info");
                displaySystemInfo();
                break;
            case 2:
                Serial.println("Display: Memory info");
                displayMemoryInfo();
                break;
            case 3:
                Serial.println("Display: Uptime");
                displayUptime();
                break;
        }

        // Move to next mode
        displayMode = (displayMode + 1) % NUM_DISPLAY_MODES;
    }

    // Small delay
    delay(100);
}
