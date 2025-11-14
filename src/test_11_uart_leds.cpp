/**
 * Test 11: UART Communication with LED Status Indicators
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - 32 WS2812B LEDs (4 strips × 8 LEDs)
 * - Direct UART connection (GPIO 16/17)
 *
 * Purpose:
 * - Test UART communication with visual LED feedback
 * - Show pump activity with LED colors
 * - Indicate system status visually
 *
 * LED Indicators:
 * - Strip 0 (Pump X): Cyan when active
 * - Strip 1 (Pump Y): Magenta when active
 * - Strip 2 (Pump Z): Yellow when active
 * - Strip 3 (Pump A): White when active
 * - All: Green = Idle, Red = Error, Blue = Running
 *
 * Build command:
 *   pio run -e test_11_uart_leds -t upload -t monitor
 */

#include <Arduino.h>
#include <FastLED.h>
#include "pin_definitions.h"

#define UartSerial         Serial2

CRGB leds[LED_TOTAL_COUNT];
enum SystemState { IDLE, RUNNING, ERROR };
SystemState currentState = IDLE;

void setStripColor(int strip, CRGB color) {
    int start = strip * LED_PER_STRIP;
    for (int i = 0; i < LED_PER_STRIP; i++) {
        leds[start + i] = color;
    }
    FastLED.show();
}

void setAllStrips(CRGB color) {
    fill_solid(leds, LED_TOTAL_COUNT, color);
    FastLED.show();
}

void sendCommand(const char* cmd) {
    Serial.print("→ ");
    Serial.println(cmd);
    UartSerial.println(cmd);
    UartSerial.flush();
}

void updateLEDs() {
    switch (currentState) {
        case IDLE:
            setAllStrips(CRGB::Green);
            break;
        case RUNNING:
            setAllStrips(CRGB::Blue);
            break;
        case ERROR:
            setAllStrips(CRGB::Red);
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║        Test 11: UART Communication + LED Indicators       ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");

    // Initialize LEDs
    FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, LED_TOTAL_COUNT);
    FastLED.setBrightness(50);
    setAllStrips(CRGB::Green);
    Serial.println("✓ LEDs initialized (Green = IDLE)");

    // Initialize UART
    UartSerial.begin(115200, SERIAL_8N1, UART_TEST_RX_PIN, UART_TEST_TX_PIN);
    Serial.println("✓ UART initialized\n");

    Serial.println("LED Status Codes:");
    Serial.println("  Green  = System IDLE");
    Serial.println("  Blue   = System RUNNING");
    Serial.println("  Red    = ERROR state");
    Serial.println("  Cyan   = Pump X active");
    Serial.println("  Magenta= Pump Y active");
    Serial.println("  Yellow = Pump Z active");
    Serial.println("  White  = Pump A active\n");

    delay(1000);
    sendCommand("?");
}

void loop() {
    // Process received data
    if (UartSerial.available()) {
        String response = UartSerial.readStringUntil('\n');
        response.trim();

        if (response.length() > 0) {
            Serial.print("← ");
            Serial.println(response);

            // Parse state and update LEDs
            if (response.indexOf("Idle") >= 0) {
                currentState = IDLE;
                updateLEDs();
            } else if (response.indexOf("Run") >= 0) {
                currentState = RUNNING;
                updateLEDs();
            } else if (response.indexOf("error") >= 0 || response.indexOf("ALARM") >= 0) {
                currentState = ERROR;
                updateLEDs();
            }
        }
    }

    // Periodically query status
    static unsigned long lastQuery = 0;
    if (millis() - lastQuery > 1000) {
        sendCommand("?");
        lastQuery = millis();
    }

    delay(100);
}
