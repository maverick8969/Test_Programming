/**
 * Test 14: Multi-Pump Simultaneous Operation
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - 4 peristaltic pumps (X, Y, Z, A axes)
 * - Direct UART connection (GPIO 16/17)
 *
 * Purpose:
 * - Test simultaneous operation of multiple pumps
 * - Verify synchronized multi-axis movement
 * - Test complex dispensing patterns
 *
 * Functionality:
 * - Dispense from multiple pumps at once
 * - Coordinate different flow rates
 * - Execute synchronized patterns
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

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║       Test 14: Multi-Pump Simultaneous Operation          ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");

    // Initialize UART
    UartSerial.begin(115200, SERIAL_8N1, UART_TEST_RX_PIN, UART_TEST_TX_PIN);
    Serial.println("✓ UART initialized\n");

    Serial.println("Predefined Patterns:");
    Serial.println("  1 - Equal mix (5ml each pump)");
    Serial.println("  2 - Ratio 2:1:1:0.5 (total 9ml)");
    Serial.println("  3 - Custom ratio");
    Serial.println("\nCommands:");
    Serial.println("  1-3 - Run pattern");
    Serial.println("  s - Query status");
    Serial.println("  h - Home all pumps\n");

    delay(1000);
    sendCommand("?");
}

void loop() {
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

    delay(10);
}
