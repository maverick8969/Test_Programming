/**
 * Test 17: Emergency Stop and Safety Features
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - Emergency stop button
 * - LED indicators
 * - Direct UART connection (GPIO 16/17)
 *
 * Purpose:
 * - Test emergency stop functionality
 * - Verify safety interlocks
 * - Test alarm conditions
 * - Validate safety response times
 *
 * Safety Features:
 * - Hardware emergency stop button
 * - Software e-stop command
 * - Timeout protection
 * - Alarm state detection
 * - Visual/audio feedback
 *
 * Build command:
 *   pio run -e test_17_safety_features -t upload -t monitor
 */

#include <Arduino.h>
#include <FastLED.h>
#include "pin_definitions.h"

#define UartSerial         Serial2

CRGB leds[LED_TOTAL_COUNT];

enum SafetyState {
    SAFE_NORMAL,
    SAFE_WARNING,
    SAFE_ESTOP,
    SAFE_ALARM
};

SafetyState safetyState = SAFE_NORMAL;
unsigned long lastHeartbeat = 0;
unsigned long lastCommandTime = 0;
const unsigned long HEARTBEAT_TIMEOUT = 5000;  // 5 seconds
const unsigned long COMMAND_TIMEOUT = 30000;   // 30 seconds max run time
bool systemRunning = false;

void sendCommand(const char* cmd) {
    Serial.print("→ ");
    Serial.println(cmd);
    UartSerial.println(cmd);
    UartSerial.flush();
    lastCommandTime = millis();
}

void updateSafetyLEDs() {
    CRGB color;
    switch (safetyState) {
        case SAFE_NORMAL:
            color = CRGB::Green;
            break;
        case SAFE_WARNING:
            color = CRGB::Yellow;
            break;
        case SAFE_ESTOP:
            color = CRGB::Red;
            break;
        case SAFE_ALARM:
            // Flashing red
            color = (millis() / 500) % 2 ? CRGB::Red : CRGB::Black;
            break;
    }
    fill_solid(leds, LED_TOTAL_COUNT, color);
    FastLED.show();
}

void triggerEmergencyStop(const char* reason) {
    Serial.println("\n!!! EMERGENCY STOP !!!");
    Serial.print("Reason: ");
    Serial.println(reason);

    sendCommand("!");  // Feed hold
    delay(100);
    sendCommand("\x18");  // Soft reset (Ctrl-X)

    safetyState = SAFE_ESTOP;
    systemRunning = false;
    updateSafetyLEDs();
}

void checkSafety() {
    // Check heartbeat timeout
    if (systemRunning && (millis() - lastHeartbeat > HEARTBEAT_TIMEOUT)) {
        triggerEmergencyStop("Heartbeat timeout");
        return;
    }

    // Check command timeout
    if (systemRunning && (millis() - lastCommandTime > COMMAND_TIMEOUT)) {
        triggerEmergencyStop("Command timeout - max run time exceeded");
        return;
    }

    // Check hardware e-stop button
    if (digitalRead(STOP_BUTTON_PIN) == LOW) {
        triggerEmergencyStop("Hardware E-Stop button pressed");
        return;
    }
}

void resetSafety() {
    Serial.println("Resetting safety system...");
    sendCommand("$X");  // Unlock alarm
    delay(500);
    safetyState = SAFE_NORMAL;
    systemRunning = false;
    updateSafetyLEDs();
    Serial.println("✓ Safety system reset - SAFE to operate");
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║        Test 17: Emergency Stop & Safety Features          ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");

    // Initialize LEDs
    FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, LED_TOTAL_COUNT);
    FastLED.setBrightness(50);
    updateSafetyLEDs();
    Serial.println("✓ Safety LEDs initialized");

    // Initialize buttons
    pinMode(START_BUTTON_PIN, INPUT_PULLUP);
    pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
    pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);
    Serial.println("✓ Safety buttons initialized");

    // Initialize UART
    UartSerial.begin(115200, SERIAL_8N1, UART_TEST_RX_PIN, UART_TEST_TX_PIN);
    Serial.println("✓ UART initialized\n");

    Serial.println("Safety Features:");
    Serial.println("  • Hardware E-Stop button (STOP)");
    Serial.println("  • Heartbeat timeout (5s)");
    Serial.println("  • Command timeout (30s max run)");
    Serial.println("  • Alarm state detection");
    Serial.println("  • Visual LED feedback");
    Serial.println("\nLED Codes:");
    Serial.println("  Green  - Normal operation");
    Serial.println("  Yellow - Warning");
    Serial.println("  Red    - Emergency stop");
    Serial.println("  Flash  - Alarm state");
    Serial.println("\nCommands:");
    Serial.println("  t - Test run (5 second move)");
    Serial.println("  e - Software e-stop");
    Serial.println("  r - Reset safety system");
    Serial.println("  h - Send heartbeat\n");
}

void loop() {
    // Continuous safety check
    checkSafety();
    updateSafetyLEDs();

    // Handle user commands
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        if (input == "t") {
            if (safetyState == SAFE_NORMAL) {
                Serial.println("Starting test run...");
                systemRunning = true;
                lastHeartbeat = millis();
                sendCommand("G92 X0");
                delay(100);
                sendCommand("G1 X10 F200");
            } else {
                Serial.println("Cannot run - system not in SAFE state!");
            }
        } else if (input == "e") {
            triggerEmergencyStop("Software e-stop command");
        } else if (input == "r") {
            resetSafety();
        } else if (input == "h") {
            lastHeartbeat = millis();
            Serial.println("Heartbeat updated");
        }
    }

    // Process responses
    if (UartSerial.available()) {
        String response = UartSerial.readStringUntil('\n');
        response.trim();
        Serial.print("← ");
        Serial.println(response);

        // Check for alarm state
        if (response.indexOf("ALARM") >= 0) {
            safetyState = SAFE_ALARM;
            systemRunning = false;
            Serial.println("⚠️  ALARM detected!");
        }

        // Check for idle (task complete)
        if (response.indexOf("Idle") >= 0 && systemRunning) {
            Serial.println("✓ Task completed safely");
            systemRunning = false;
        }
    }

    delay(50);
}
