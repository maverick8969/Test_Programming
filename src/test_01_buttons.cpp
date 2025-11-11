/**
 * @file test_01_buttons.cpp
 * @brief Phase 1 - Test 01: Push Button Testing (Arduino)
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
 */

#include <Arduino.h>
#include "pin_definitions.h"

// Button state structure
struct Button {
    uint8_t pin;
    const char* name;
    bool currentState;
    bool lastState;
    unsigned long pressTime;
    unsigned long releaseTime;
    uint32_t pressCount;
};

// Button instances
Button buttons[] = {
    {START_BUTTON_PIN, "START", HIGH, HIGH, 0, 0, 0},
    {MODE_BUTTON_PIN,  "MODE",  HIGH, HIGH, 0, 0, 0},
    {STOP_BUTTON_PIN,  "STOP",  HIGH, HIGH, 0, 0, 0}
};

const int NUM_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);

/**
 * @brief Read and debounce button state
 * @param btn Pointer to button structure
 * @return true if state changed, false otherwise
 */
bool readButton(Button* btn) {
    // Read raw GPIO state (LOW = pressed due to pull-up)
    bool pressed = (digitalRead(btn->pin) == LOW);

    // Update current state
    btn->currentState = pressed;

    // Check for state change
    if (btn->currentState != btn->lastState) {
        // Simple debouncing: wait and re-read
        delay(BUTTON_DEBOUNCE_MS);
        pressed = (digitalRead(btn->pin) == LOW);

        if (pressed == btn->currentState) {
            // State change confirmed
            btn->lastState = btn->currentState;
            return true;
        }
    }

    return false;
}

void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(100);

    // Print instructions
    Serial.println("\n========================================");
    Serial.println("Peristaltic Pump System - Test 01");
    Serial.println("Push Button Test");
    Serial.println("========================================");
    Serial.println("Hardware Configuration:");
    Serial.print("  START button: GPIO ");
    Serial.println(START_BUTTON_PIN);
    Serial.print("  MODE button:  GPIO ");
    Serial.println(MODE_BUTTON_PIN);
    Serial.print("  STOP button:  GPIO ");
    Serial.println(STOP_BUTTON_PIN);
    Serial.println();
    Serial.println("All buttons use internal pull-up resistors");
    Serial.println("Active LOW: Pressed = LOW, Released = HIGH");
    Serial.println("========================================");
    Serial.println("Test Instructions:");
    Serial.println("1. Press and release START button");
    Serial.println("2. Press and release MODE button");
    Serial.println("3. Press and release STOP button");
    Serial.println("4. Try holding buttons for different durations");
    Serial.println("5. Try rapid presses to test debouncing");
    Serial.println("========================================\n");

    // Initialize buttons
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(buttons[i].pin, INPUT_PULLUP);
        Serial.print("Configured ");
        Serial.print(buttons[i].name);
        Serial.print(" button on GPIO ");
        Serial.println(buttons[i].pin);
    }

    Serial.println("\nButton monitoring started");
    Serial.println("Press buttons to test...\n");
}

void loop() {
    // Check each button
    for (int i = 0; i < NUM_BUTTONS; i++) {
        Button* btn = &buttons[i];

        if (readButton(btn)) {
            unsigned long now = millis();

            if (btn->currentState) {
                // Button pressed
                btn->pressTime = now;
                btn->pressCount++;
                Serial.print("[");
                Serial.print(now);
                Serial.print("] ✓ ");
                Serial.print(btn->name);
                Serial.print(" button PRESSED (count: ");
                Serial.print(btn->pressCount);
                Serial.println(")");
            } else {
                // Button released
                btn->releaseTime = now;
                unsigned long duration = btn->releaseTime - btn->pressTime;
                Serial.print("[");
                Serial.print(now);
                Serial.print("] ✗ ");
                Serial.print(btn->name);
                Serial.print(" button RELEASED (duration: ");
                Serial.print(duration);
                Serial.println("ms)");
            }
        }
    }

    // Small delay to prevent CPU hogging
    delay(10);
}
