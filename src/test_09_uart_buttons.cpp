/**
 * Test 09: UART Communication with Button Control
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - 3 control buttons (Start, Mode, Stop)
 * - Direct UART connection (GPIO 16/17)
 *
 * Purpose:
 * - Test UART communication controlled by physical buttons
 * - Send G-code commands when buttons are pressed
 * - Verify button debouncing with motor control
 *
 * Functionality:
 * - START button: Start pump X movement (G0 X10 F200)
 * - MODE button: Cycle through pumps (X, Y, Z, A)
 * - STOP button: Emergency stop (!)
 *
 * Build command:
 *   pio run -e test_09_uart_buttons -t upload -t monitor
 */

#include <Arduino.h>
#include "pin_definitions.h"

#define UartSerial         Serial2

// Button states
bool lastStartState = HIGH;
bool lastModeState = HIGH;
bool lastStopState = HIGH;
unsigned long lastDebounceTime = 0;

// Current pump selection
enum Pump { PUMP_X, PUMP_Y, PUMP_Z, PUMP_A };
Pump currentPump = PUMP_X;
const char* pumpNames[] = {"X", "Y", "Z", "A"};

void sendCommand(const char* cmd) {
    Serial.print("→ Sending: ");
    Serial.println(cmd);
    UartSerial.println(cmd);
    UartSerial.flush();
}

void handleButtons() {
    bool startPressed = (digitalRead(START_BUTTON_PIN) == LOW);
    bool modePressed = (digitalRead(MODE_BUTTON_PIN) == LOW);
    bool stopPressed = (digitalRead(STOP_BUTTON_PIN) == LOW);

    // START button - move current pump
    if (startPressed && !lastStartState) {
        char cmd[32];
        snprintf(cmd, sizeof(cmd), "G0 %s10 F200", pumpNames[currentPump]);
        sendCommand(cmd);
        Serial.print("Started pump: ");
        Serial.println(pumpNames[currentPump]);
    }

    // MODE button - cycle pumps
    if (modePressed && !lastModeState) {
        currentPump = (Pump)((currentPump + 1) % 4);
        Serial.print("Selected pump: ");
        Serial.println(pumpNames[currentPump]);
    }

    // STOP button - emergency stop
    if (stopPressed && !lastStopState) {
        sendCommand("!");
        Serial.println("EMERGENCY STOP!");
    }

    lastStartState = startPressed;
    lastModeState = modePressed;
    lastStopState = stopPressed;
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║        Test 09: UART Communication + Button Control       ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");

    // Initialize buttons
    pinMode(START_BUTTON_PIN, INPUT_PULLUP);
    pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
    pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);
    Serial.println("✓ Buttons initialized");

    // Initialize UART
    UartSerial.begin(115200, SERIAL_8N1, UART_TEST_RX_PIN, UART_TEST_TX_PIN);
    Serial.println("✓ UART initialized\n");

    Serial.println("Controls:");
    Serial.println("  START button - Move current pump");
    Serial.println("  MODE button  - Select pump (X→Y→Z→A)");
    Serial.println("  STOP button  - Emergency stop");
    Serial.println("\nReady! Current pump: X\n");
}

void loop() {
    handleButtons();

    // Echo received data
    if (UartSerial.available()) {
        Serial.print("← ");
        while (UartSerial.available()) {
            Serial.write(UartSerial.read());
        }
    }

    delay(50); // Debounce delay
}
