/**
 * Test 09: UART Communication with Button and Encoder Control
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - 3 control buttons (Start, Mode, Stop)
 * - Rotary encoder with button
 * - Direct UART connection (GPIO 16/17)
 *
 * Purpose:
 * - Test UART communication controlled by physical buttons and encoder
 * - Send G-code commands when buttons are pressed
 * - Verify button debouncing with motor control
 * - Use encoder for pump selection
 *
 * Functionality:
 * - START button: Start selected pump movement (G0 X/Y/Z/A 10 F200)
 * - MODE button: Cycle through pumps (X→Y→Z→A)
 * - ENCODER rotation: Select pump (CW/CCW)
 * - ENCODER button: Confirm selection and start pump
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

// Encoder state
struct EncoderState {
    int32_t position;
    int32_t lastPosition;
    bool clkState;
    bool dtState;
    bool lastClkState;
};

struct EncoderButton {
    bool pressed;
    bool lastPressed;
};

EncoderState encoder = {0, 0, false, false, false};
EncoderButton encButton = {false, false};

// Current pump selection
int currentPump = 0;  // 0=X, 1=Y, 2=Z, 3=A
const char* pumpNames[] = {"X", "Y", "Z", "A"};

void sendCommand(const char* cmd) {
    Serial.print("→ Sending: ");
    Serial.println(cmd);
    UartSerial.println(cmd);
    UartSerial.flush();
}

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

bool readEncoderButton() {
    // Read button state (active LOW)
    bool pressed = (digitalRead(ENCODER_SW_PIN) == LOW);

    // Check for state change with debounce
    if (pressed != encButton.lastPressed) {
        delay(50);  // Debounce
        pressed = (digitalRead(ENCODER_SW_PIN) == LOW);

        if (pressed != encButton.lastPressed) {
            encButton.lastPressed = pressed;
            encButton.pressed = pressed;
            return true;
        }
    }

    return false;
}

void handleEncoder() {
    // Check for rotation
    int direction = readEncoder();
    if (direction != 0) {
        // Update pump selection based on encoder position
        currentPump = ((encoder.position % 4) + 4) % 4;
        Serial.print("Encoder: Selected pump ");
        Serial.println(pumpNames[currentPump]);
    }

    // Check encoder button press
    if (readEncoderButton() && encButton.pressed) {
        // Encoder button pressed - start selected pump
        char cmd[32];
        snprintf(cmd, sizeof(cmd), "G0 %s10 F200", pumpNames[currentPump]);
        sendCommand(cmd);
        Serial.print("Encoder SELECT: Started pump ");
        Serial.println(pumpNames[currentPump]);
    }
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
        Serial.print("START button: Started pump ");
        Serial.println(pumpNames[currentPump]);
    }

    // MODE button - cycle pumps
    if (modePressed && !lastModeState) {
        currentPump = (currentPump + 1) % 4;
        // Sync encoder position with button selection
        encoder.position = currentPump;
        Serial.print("MODE button: Selected pump ");
        Serial.println(pumpNames[currentPump]);
    }

    // STOP button - emergency stop
    if (stopPressed && !lastStopState) {
        sendCommand("!");
        Serial.println("STOP button: EMERGENCY STOP!");
    }

    lastStartState = startPressed;
    lastModeState = modePressed;
    lastStopState = stopPressed;
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║    Test 09: UART Communication + Button/Encoder Control   ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");

    // Initialize buttons
    pinMode(START_BUTTON_PIN, INPUT_PULLUP);
    pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
    pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);
    Serial.println("✓ Buttons initialized");

    // Initialize encoder
    pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(ENCODER_DT_PIN, INPUT_PULLUP);
    pinMode(ENCODER_SW_PIN, INPUT);  // GPIO 34 is input-only, needs external pull-up
    encoder.clkState = digitalRead(ENCODER_CLK_PIN);
    encoder.dtState = digitalRead(ENCODER_DT_PIN);
    encoder.lastClkState = encoder.clkState;
    encoder.position = 0;
    Serial.println("✓ Encoder initialized");

    // Initialize UART
    UartSerial.begin(115200, SERIAL_8N1, UART_TEST_RX_PIN, UART_TEST_TX_PIN);
    Serial.println("✓ UART initialized\n");

    Serial.println("Controls:");
    Serial.println("  START button    - Start selected pump");
    Serial.println("  MODE button     - Cycle pump (X→Y→Z→A)");
    Serial.println("  ENCODER rotate  - Select pump (CW/CCW)");
    Serial.println("  ENCODER button  - Start selected pump");
    Serial.println("  STOP button     - Emergency stop");
    Serial.println("\nReady! Current pump: X\n");
}

void loop() {
    handleEncoder();
    handleButtons();

    // Echo received data
    if (UartSerial.available()) {
        Serial.print("← ");
        while (UartSerial.available()) {
            Serial.write(UartSerial.read());
        }
    }

    delay(1); // Small delay for responsiveness
}
