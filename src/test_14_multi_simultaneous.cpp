/**
 * Test 14: Multi-Pump Simultaneous Operation + Encoder Control
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - 4 peristaltic pumps (X, Y, Z, A axes)
 * - Rotary encoder with button
 * - Direct UART connection (GPIO 16/17)
 *
 * Purpose:
 * - Test simultaneous operation of multiple pumps
 * - Verify synchronized multi-axis movement
 * - Test complex dispensing patterns
 * - Use encoder to select patterns
 *
 * Functionality:
 * - Dispense from multiple pumps at once
 * - Coordinate different flow rates
 * - Execute synchronized patterns
 *
 * Encoder Controls:
 * - Rotate: Select pattern (1-3)
 * - Press: Execute selected pattern
 *
 * Build command:
 *   pio run -e test_14_multi_simultaneous -t upload -t monitor
 */

#include <Arduino.h>
#include "pin_definitions.h"

#define UartSerial         Serial2

const float ML_PER_MM = 0.05;

struct MultiPumpCommand {
    float volumeX, volumeY, volumeZ, volumeA;
    float flowRateMlMin;
};

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

int selectedPattern = 0;  // 0=Pattern 1, 1=Pattern 2, 2=Pattern 3
const char* patternNames[] = {"Equal mix (5ml each)", "Ratio 2:1:1:0.5", "Custom ratio"};

void sendCommand(const char* cmd) {
    Serial.print("→ ");
    Serial.println(cmd);
    UartSerial.println(cmd);
    UartSerial.flush();
}

void dispenseMultiple(MultiPumpCommand cmd) {
    float distX = cmd.volumeX / ML_PER_MM;
    float distY = cmd.volumeY / ML_PER_MM;
    float distZ = cmd.volumeZ / ML_PER_MM;
    float distA = cmd.volumeA / ML_PER_MM;
    float feedRate = cmd.flowRateMlMin / ML_PER_MM;

    Serial.println("\n[Simultaneous Dispensing]");
    Serial.println("Volumes:");
    Serial.print("  X: ");
    Serial.print(cmd.volumeX);
    Serial.println(" ml");
    Serial.print("  Y: ");
    Serial.print(cmd.volumeY);
    Serial.println(" ml");
    Serial.print("  Z: ");
    Serial.print(cmd.volumeZ);
    Serial.println(" ml");
    Serial.print("  A: ");
    Serial.print(cmd.volumeA);
    Serial.println(" ml");
    Serial.print("Flow rate: ");
    Serial.print(cmd.flowRateMlMin);
    Serial.println(" ml/min");

    // Reset all positions
    sendCommand("G92 X0 Y0 Z0 A0");
    delay(100);

    // Execute synchronized move
    char gcodeCmd[128];
    snprintf(gcodeCmd, sizeof(gcodeCmd),
             "G1 X%.2f Y%.2f Z%.2f A%.2f F%.1f",
             distX, distY, distZ, distA, feedRate);
    sendCommand(gcodeCmd);

    Serial.println("Dispensing all pumps simultaneously...");
}

int readEncoder() {
    encoder.clkState = digitalRead(ENCODER_CLK_PIN);

    if (encoder.clkState != encoder.lastClkState) {
        encoder.dtState = digitalRead(ENCODER_DT_PIN);

        int direction = 0;
        if (encoder.clkState == LOW) {
            if (encoder.dtState != encoder.clkState) {
                direction = 1;
                encoder.position++;
            } else {
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
    bool pressed = (digitalRead(ENCODER_SW_PIN) == LOW);

    if (pressed != encButton.lastPressed) {
        delay(50);
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
    // Navigate patterns
    int direction = readEncoder();
    if (direction != 0) {
        selectedPattern = ((encoder.position % 3) + 3) % 3;
        Serial.print("Encoder: Pattern ");
        Serial.print(selectedPattern + 1);
        Serial.print(" - ");
        Serial.println(patternNames[selectedPattern]);
    }

    // Execute pattern on button press
    if (readEncoderButton() && encButton.pressed) {
        Serial.println("Encoder: EXECUTE pattern");
        MultiPumpCommand cmd;

        if (selectedPattern == 0) {
            // Equal mix
            cmd = {5.0, 5.0, 5.0, 5.0, 20.0};
            dispenseMultiple(cmd);
        } else if (selectedPattern == 1) {
            // Ratio mix
            cmd = {4.0, 2.0, 2.0, 1.0, 15.0};
            dispenseMultiple(cmd);
        } else if (selectedPattern == 2) {
            // Custom
            cmd = {3.0, 2.0, 1.5, 0.5, 10.0};
            dispenseMultiple(cmd);
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║   Test 14: Multi-Pump Simultaneous Operation + Encoder    ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");

    // Initialize encoder
    pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(ENCODER_DT_PIN, INPUT_PULLUP);
    pinMode(ENCODER_SW_PIN, INPUT);
    encoder.clkState = digitalRead(ENCODER_CLK_PIN);
    encoder.dtState = digitalRead(ENCODER_DT_PIN);
    encoder.lastClkState = encoder.clkState;
    encoder.position = 0;
    Serial.println("✓ Encoder initialized");

    // Initialize UART
    UartSerial.begin(115200, SERIAL_8N1, UART_TEST_RX_PIN, UART_TEST_TX_PIN);
    Serial.println("✓ UART initialized\n");

    Serial.println("Predefined Patterns:");
    for (int i = 0; i < 3; i++) {
        Serial.print("  ");
        Serial.print(i + 1);
        Serial.print(". ");
        Serial.println(patternNames[i]);
    }

    Serial.println("\nControls:");
    Serial.println("  ENCODER rotate  - Select pattern (1-3)");
    Serial.println("  ENCODER button  - Execute selected pattern");
    Serial.println("\nCommands:");
    Serial.println("  1-3 - Run pattern");
    Serial.println("  s - Query status");
    Serial.println("  h - Home all pumps\n");

    delay(1000);
    sendCommand("?");
}

void loop() {
    // Handle encoder
    handleEncoder();

    // Handle user commands
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        MultiPumpCommand cmd;

        if (input == "1") {
            // Equal mix
            cmd = {5.0, 5.0, 5.0, 5.0, 20.0};
            dispenseMultiple(cmd);
        } else if (input == "2") {
            // Ratio mix
            cmd = {4.0, 2.0, 2.0, 1.0, 15.0};
            dispenseMultiple(cmd);
        } else if (input == "3") {
            // Custom - user can modify
            cmd = {3.0, 2.0, 1.5, 0.5, 10.0};
            dispenseMultiple(cmd);
        } else if (input == "s") {
            sendCommand("?");
        } else if (input == "h") {
            sendCommand("$H");
        }
    }

    // Echo responses
    if (UartSerial.available()) {
        while (UartSerial.available()) {
            Serial.write(UartSerial.read());
        }
    }

    delay(1);
}
