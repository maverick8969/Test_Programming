/**
 * Test 10: UART Communication with LCD Display
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - 1602 LCD with I2C backpack
 * - Direct UART connection (GPIO 16/17)
 *
 * Purpose:
 * - Test UART communication with LCD status display
 * - Show real-time status from FluidNC on LCD
 * - Display current position and state
 *
 * LCD Display:
 * - Line 1: Connection status / Current state
 * - Line 2: Position or command feedback
 *
 * Build command:
 *   pio run -e test_10_uart_lcd -t upload -t monitor
 */

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "pin_definitions.h"

#define UartSerial         Serial2

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 16, 2);
String lastResponse = "";
unsigned long lastQueryTime = 0;

void sendCommand(const char* cmd) {
    Serial.print("→ ");
    Serial.println(cmd);
    UartSerial.println(cmd);
    UartSerial.flush();
}

void updateLCD(const char* line1, const char* line2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║          Test 10: UART Communication + LCD Display        ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");

    // Initialize I2C LCD
    Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN);
    lcd.init();
    lcd.backlight();
    updateLCD("FluidNC UART", "Connecting...");
    Serial.println("✓ LCD initialized");

    // Initialize UART
    UartSerial.begin(115200, SERIAL_8N1, UART_TEST_RX_PIN, UART_TEST_TX_PIN);
    Serial.println("✓ UART initialized\n");

    delay(1000);
    sendCommand("?"); // Query status
}

void loop() {
    // Query status every 2 seconds
    if (millis() - lastQueryTime > 2000) {
        sendCommand("?");
        lastQueryTime = millis();
    }

    // Process received data
    if (UartSerial.available()) {
        String response = UartSerial.readStringUntil('\n');
        response.trim();

        if (response.length() > 0) {
            Serial.print("← ");
            Serial.println(response);

            // Update LCD with response
            if (response.startsWith("<Idle")) {
                updateLCD("Status: IDLE", response.substring(0, 16).c_str());
            } else if (response.startsWith("<Run")) {
                updateLCD("Status: RUNNING", response.substring(0, 16).c_str());
            } else if (response.startsWith("ok")) {
                updateLCD("FluidNC Ready", "Status: OK");
            } else {
                updateLCD("FluidNC", response.substring(0, 16).c_str());
            }
        }
    }

    delay(100);
}
