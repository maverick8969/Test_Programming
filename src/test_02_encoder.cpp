/**
 * @file test_02_encoder.cpp
 * @brief Phase 1 - Test 02: Rotary Encoder Testing (Arduino)
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
 * - 10kΩ resistor (for GPIO 34 pull-up)
 *
 * WIRING:
 *   Encoder CLK: GPIO 26 (internal pull-up)
 *   Encoder DT:  GPIO 27 (internal pull-up)
 *   Encoder SW:  GPIO 34 ──[10kΩ]── 3.3V (EXTERNAL PULL-UP REQUIRED!)
 *   Encoder GND: GND
 *   Encoder VCC: 3.3V (if needed)
 *
 * ⚠️ IMPORTANT: GPIO 34 is input-only and has NO internal pull-up!
 *    You MUST add external 10kΩ resistor from GPIO 34 to 3.3V
 *    OR use encoder module with built-in pull-up resistor
 *
 * USAGE:
 * 1. Wire encoder as shown above (don't forget pull-up!)
 * 2. Run: pio run -e test_02_encoder -t upload -t monitor
 * 3. Rotate encoder clockwise and counter-clockwise
 * 4. Press encoder button (SELECT function)
 * 5. Observe position counter and button events
 */

#include <Arduino.h>
#include "pin_definitions.h"

// Encoder state structure
struct EncoderState {
    int32_t position;
    int32_t lastPosition;
    bool clkState;
    bool dtState;
    bool lastClkState;
};

// Button state for encoder switch
struct EncoderButton {
    bool pressed;
    bool lastPressed;
    unsigned long pressTime;
    uint32_t pressCount;
};

// Global state
EncoderState encoder = {0, 0, false, false, false};
EncoderButton encButton = {false, false, 0, 0};

// Status update timer
unsigned long lastStatusTime = 0;
const unsigned long STATUS_INTERVAL = 10000; // 10 seconds

/**
 * @brief Initialize encoder GPIOs
 */
void initEncoder() {
    // Configure CLK and DT pins with internal pull-ups
    pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(ENCODER_DT_PIN, INPUT_PULLUP);

    // Configure SW (button) pin - GPIO 34 is input-only, NO internal pull-up!
    pinMode(ENCODER_SW_PIN, INPUT);

    // Read initial states
    encoder.clkState = digitalRead(ENCODER_CLK_PIN);
    encoder.dtState = digitalRead(ENCODER_DT_PIN);
    encoder.lastClkState = encoder.clkState;
    encoder.position = 0;
    encoder.lastPosition = 0;

    Serial.println("Encoder configured:");
    Serial.print("  CLK: GPIO ");
    Serial.print(ENCODER_CLK_PIN);
    Serial.println(" (internal pull-up)");
    Serial.print("  DT:  GPIO ");
    Serial.print(ENCODER_DT_PIN);
    Serial.println(" (internal pull-up)");
    Serial.print("  SW:  GPIO ");
    Serial.print(ENCODER_SW_PIN);
    Serial.println(" (SELECT button) ⚠️ NEEDS EXTERNAL PULL-UP!");
}

/**
 * @brief Read encoder rotation
 * @return 1 for CW, -1 for CCW, 0 for no change
 */
int readEncoder() {
    // Read current CLK state
    encoder.clkState = digitalRead(ENCODER_CLK_PIN);

    // Check if CLK changed
    if (encoder.clkState != encoder.lastClkState) {
        // CLK changed - read DT to determine direction
        encoder.dtState = digitalRead(ENCODER_DT_PIN);

        int direction = 0;
        if (encoder.clkState == LOW) {  // Falling edge of CLK
            if (encoder.dtState != encoder.clkState) {
                // DT is HIGH when CLK falls = Clockwise
                direction = 1;
                encoder.position++;
            } else {
                // DT is LOW when CLK falls = Counter-clockwise
                direction = -1;
                encoder.position--;
            }
        }

        encoder.lastClkState = encoder.clkState;
        return direction;
    }

    return 0;
}

/**
 * @brief Read encoder button (SELECT function)
 * @return true if button state changed, false otherwise
 */
bool readEncoderButton() {
    // Read button state (active LOW)
    bool pressed = (digitalRead(ENCODER_SW_PIN) == LOW);

    // Check for state change
    if (pressed != encButton.lastPressed) {
        // Debounce
        delay(ENCODER_DEBOUNCE_MS);
        pressed = (digitalRead(ENCODER_SW_PIN) == LOW);

        if (pressed != encButton.lastPressed) {
            encButton.lastPressed = pressed;
            encButton.pressed = pressed;
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
    Serial.println("Peristaltic Pump System - Test 02");
    Serial.println("Rotary Encoder Test");
    Serial.println("========================================");
    Serial.println("Hardware Configuration:");
    Serial.print("  Encoder CLK: GPIO ");
    Serial.print(ENCODER_CLK_PIN);
    Serial.println(" (internal pull-up)");
    Serial.print("  Encoder DT:  GPIO ");
    Serial.print(ENCODER_DT_PIN);
    Serial.println(" (internal pull-up)");
    Serial.print("  Encoder SW:  GPIO ");
    Serial.print(ENCODER_SW_PIN);
    Serial.println(" (SELECT) ⚠️ NEEDS EXTERNAL PULL-UP!");
    Serial.println();
    Serial.println("⚠️  GPIO 34 is input-only with NO internal pull-up!");
    Serial.println("    Add 10kΩ resistor: GPIO 34 ──[10kΩ]── 3.3V");
    Serial.println("    Or use encoder module with built-in pull-up");
    Serial.println("========================================");
    Serial.println("Test Instructions:");
    Serial.println("1. Rotate encoder clockwise (CW)");
    Serial.println("   - Position should increase: 0 → 1 → 2 → 3...");
    Serial.println("2. Rotate encoder counter-clockwise (CCW)");
    Serial.println("   - Position should decrease: 3 → 2 → 1 → 0...");
    Serial.println("3. Press encoder button (SELECT function)");
    Serial.println("   - Should show PRESSED and RELEASED events");
    Serial.println("4. Try rotating while holding button");
    Serial.println("5. Test rapid rotation for smoothness");
    Serial.println("========================================");
    Serial.println("Note: Encoder button serves dual purpose:");
    Serial.println("  - Navigation: Rotates through menu items");
    Serial.println("  - Selection: Press to confirm (SELECT)");
    Serial.println("========================================\n");

    // Initialize encoder
    initEncoder();

    Serial.println("\nAll systems ready. Rotate encoder and press button...\n");
}

void loop() {
    // Check for rotation
    int direction = readEncoder();
    if (direction != 0) {
        unsigned long now = millis();

        if (direction > 0) {
            Serial.print("[");
            Serial.print(now);
            Serial.print("] Position: ");
            Serial.print(encoder.position);
            Serial.println(" (CW →)");
        } else {
            Serial.print("[");
            Serial.print(now);
            Serial.print("] Position: ");
            Serial.print(encoder.position);
            Serial.println(" (CCW ←)");
        }
    }

    // Check for button press
    if (readEncoderButton()) {
        unsigned long now = millis();

        if (encButton.pressed) {
            // Button pressed
            encButton.pressTime = now;
            encButton.pressCount++;
            Serial.print("[");
            Serial.print(now);
            Serial.print("] ✓ SELECT button PRESSED (count: ");
            Serial.print(encButton.pressCount);
            Serial.print(") [Position: ");
            Serial.print(encoder.position);
            Serial.println("]");
        } else {
            // Button released
            unsigned long duration = now - encButton.pressTime;
            Serial.print("[");
            Serial.print(now);
            Serial.print("] ✗ SELECT button RELEASED (duration: ");
            Serial.print(duration);
            Serial.println("ms)");
        }
    }

    // Print periodic status summary
    if (millis() - lastStatusTime >= STATUS_INTERVAL) {
        lastStatusTime = millis();

        if (encoder.position != encoder.lastPosition || encButton.pressCount > 0) {
            Serial.println("\n--- Status Summary ---");
            Serial.print("Current Position: ");
            Serial.println(encoder.position);
            Serial.print("Button Presses: ");
            Serial.println(encButton.pressCount);
            Serial.print("Free Heap: ");
            Serial.print(ESP.getFreeHeap());
            Serial.println(" bytes");
            Serial.println("----------------------\n");

            encoder.lastPosition = encoder.position;
        }
    }

    // Small delay
    delay(1);
}
