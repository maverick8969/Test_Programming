/**
 * @file test_00_blink.cpp
 * @brief Phase 1 - Test 00: Blink and Serial Output (Arduino)
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
 */

#include <Arduino.h>

// Pin definition
#define BLINK_GPIO 2

// Counter
uint32_t counter = 0;

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(100);

    // Print system information
    Serial.println("\n========================================");
    Serial.println("Peristaltic Pump System - Test 00");
    Serial.println("Blink Test (Arduino Framework)");
    Serial.println("========================================");
    Serial.print("ESP32 Chip Model: ");
    Serial.println(ESP.getChipModel());
    Serial.print("Chip Revision: ");
    Serial.println(ESP.getChipRevision());
    Serial.print("CPU Frequency: ");
    Serial.print(ESP.getCpuFreqMHz());
    Serial.println(" MHz");
    Serial.print("Flash Size: ");
    Serial.print(ESP.getFlashChipSize() / (1024 * 1024));
    Serial.println(" MB");
    Serial.println("========================================");
    Serial.println("Press Ctrl+] to exit monitor");
    Serial.println();

    // Configure GPIO for LED
    pinMode(BLINK_GPIO, OUTPUT);

    Serial.println("Blink test started. LED on GPIO 2");
    Serial.println();
}

void loop() {
    // Toggle LED
    digitalWrite(BLINK_GPIO, HIGH);
    Serial.print("[");
    Serial.print(counter);
    Serial.print("] LED: ON  | Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");

    delay(1000);

    digitalWrite(BLINK_GPIO, LOW);
    Serial.print("[");
    Serial.print(counter);
    Serial.print("] LED: OFF | Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");

    delay(1000);

    counter++;
}
