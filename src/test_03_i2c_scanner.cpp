/**
 * @file test_03_i2c_scanner.cpp
 * @brief Phase 2 - Test 03: I2C Scanner (Arduino)
 *
 * OBJECTIVE:
 * - Scan I2C bus for connected devices
 * - Detect LCD display I2C address
 * - Verify I2C communication is working
 *
 * SUCCESS CRITERIA:
 * - I2C bus scan completes successfully
 * - LCD display detected (typically 0x27 or 0x3F)
 * - Address reported in both hex and decimal
 * - No bus errors
 *
 * HARDWARE REQUIRED:
 * - ESP32 development board
 * - 1602 LCD with I2C backpack (PCF8574 or similar)
 * - Optional: Other I2C devices
 *
 * WIRING:
 *   LCD SDA → GPIO 21 (ESP32 SDA)
 *   LCD SCL → GPIO 22 (ESP32 SCL)
 *   LCD VCC → 5V (or 3.3V depending on module)
 *   LCD GND → GND
 *
 * USAGE:
 * 1. Wire LCD I2C module as shown above
 * 2. Run: pio run -e test_03_i2c_scanner -t upload -t monitor
 * 3. Observe I2C scan results
 * 4. Note the LCD address for Test 04
 *
 * Expected Output:
 * ========================================
 * I2C Scanner
 * Scanning...
 * I2C device found at address 0x27  (39)
 * Done. 1 device(s) found.
 * ========================================
 */

#include <Arduino.h>
#include <Wire.h>
#include "pin_definitions.h"

// Scan interval (milliseconds)
const unsigned long SCAN_INTERVAL = 5000; // 5 seconds
unsigned long lastScanTime = 0;
int scanCount = 0;

/**
 * @brief Scan I2C bus for devices
 * @return Number of devices found
 */
int scanI2C() {
    byte error, address;
    int deviceCount = 0;

    Serial.println("\n========================================");
    Serial.print("I2C Scan #");
    Serial.println(scanCount + 1);
    Serial.println("========================================");
    Serial.println("Scanning I2C bus (0x00 - 0x7F)...");
    Serial.println();

    for (address = 1; address < 127; address++) {
        // Try to communicate with device at this address
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            // Device found
            deviceCount++;

            Serial.print("✓ I2C device found at address 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            Serial.print("  (");
            Serial.print(address);
            Serial.print(")");

            // Identify common devices
            if (address == 0x27 || address == 0x3F) {
                Serial.print("  ← Likely LCD display!");
            } else if (address == 0x68) {
                Serial.print("  ← Likely RTC (DS1307/DS3231)");
            } else if (address == 0x76 || address == 0x77) {
                Serial.print("  ← Likely BME280/BMP280");
            } else if (address == 0x48 || address == 0x49) {
                Serial.print("  ← Likely ADS1115 ADC");
            }
            Serial.println();

        } else if (error == 4) {
            // Unknown error
            Serial.print("✗ Unknown error at address 0x");
            if (address < 16) Serial.print("0");
            Serial.println(address, HEX);
        }
    }

    Serial.println();
    Serial.println("========================================");
    if (deviceCount == 0) {
        Serial.println("⚠️  No I2C devices found!");
        Serial.println();
        Serial.println("Troubleshooting:");
        Serial.println("  1. Check wiring (SDA=GPIO21, SCL=GPIO22)");
        Serial.println("  2. Verify device power (VCC and GND)");
        Serial.println("  3. Check for loose connections");
        Serial.println("  4. Try external pull-up resistors (4.7kΩ)");
        Serial.println("     SDA ──[4.7kΩ]── 3.3V");
        Serial.println("     SCL ──[4.7kΩ]── 3.3V");
    } else {
        Serial.print("✓ Scan complete. ");
        Serial.print(deviceCount);
        Serial.print(" device");
        if (deviceCount != 1) Serial.print("s");
        Serial.println(" found.");
    }
    Serial.println("========================================\n");

    return deviceCount;
}

void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(100);

    // Print header
    Serial.println("\n========================================");
    Serial.println("Peristaltic Pump System - Test 03");
    Serial.println("I2C Scanner");
    Serial.println("========================================");
    Serial.println("Hardware Configuration:");
    Serial.print("  SDA: GPIO ");
    Serial.println(LCD_SDA_PIN);
    Serial.print("  SCL: GPIO ");
    Serial.println(LCD_SCL_PIN);
    Serial.print("  Frequency: ");
    Serial.print(LCD_I2C_FREQ / 1000);
    Serial.println(" kHz");
    Serial.println("========================================");
    Serial.println("Purpose:");
    Serial.println("  - Scan I2C bus for devices");
    Serial.println("  - Find LCD display address");
    Serial.println("  - Verify I2C communication");
    Serial.println();
    Serial.println("Common LCD Addresses:");
    Serial.println("  0x27 (39) - Most common");
    Serial.println("  0x3F (63) - Alternative");
    Serial.println("========================================\n");

    // Initialize I2C
    Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN, LCD_I2C_FREQ);
    Serial.println("I2C initialized");
    Serial.print("Bus frequency: ");
    Serial.print(LCD_I2C_FREQ / 1000);
    Serial.println(" kHz");

    // Wait for I2C to stabilize
    delay(100);

    // Perform initial scan
    scanI2C();
    scanCount++;
    lastScanTime = millis();

    Serial.println("Will rescan every 5 seconds...");
    Serial.println("Connect/disconnect devices to test detection.\n");
}

void loop() {
    // Perform periodic scans
    if (millis() - lastScanTime >= SCAN_INTERVAL) {
        scanI2C();
        scanCount++;
        lastScanTime = millis();

        // Print memory status
        Serial.print("Free heap: ");
        Serial.print(ESP.getFreeHeap());
        Serial.println(" bytes\n");
    }

    // Small delay
    delay(100);
}
