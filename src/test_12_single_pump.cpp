/**
 * Test 12: Single Pump Controlled Flow Rate
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - Single peristaltic pump on X-axis
 * - Direct UART connection (GPIO 16/17)
 *
 * Purpose:
 * - Test precise flow rate control for a single pump
 * - Verify G-code feedrate to flow rate conversion
 * - Measure dispensing accuracy
 *
 * Functionality:
 * - Set target flow rate (ml/min)
 * - Calculate required G-code feedrate
 * - Dispense specific volume
 * - Monitor and report actual dispensing
 *
 * Conversion:
 * - steps_per_mm = 80 (from FluidNC config)
 * - Calibrate ml/mm ratio for your pump/tubing
 *
 * Build command:
 *   pio run -e test_12_single_pump -t upload -t monitor
 */

#include <Arduino.h>
#include "pin_definitions.h"

#define UartSerial         Serial2

// Pump calibration (adjust based on actual pump)
const float ML_PER_MM = 0.05; // ml dispensed per mm of motor travel
const float STEPS_PER_MM = 80.0;

struct PumpCommand {
    float volumeMl;
    float flowRateMlMin;
    float feedRateMmMin;
    float distanceMm;
};

void sendCommand(const char* cmd) {
    Serial.print("→ ");
    Serial.println(cmd);
    UartSerial.println(cmd);
    UartSerial.flush();
}

PumpCommand calculatePumpCommand(float volumeMl, float flowRateMlMin) {
    PumpCommand cmd;
    cmd.volumeMl = volumeMl;
    cmd.flowRateMlMin = flowRateMlMin;
    cmd.distanceMm = volumeMl / ML_PER_MM;
    cmd.feedRateMmMin = flowRateMlMin / ML_PER_MM;
    return cmd;
}

void dispenseVolume(PumpCommand cmd) {
    char gcodeCmd[64];

    Serial.println("\n[Dispensing]");
    Serial.print("Target volume: ");
    Serial.print(cmd.volumeMl);
    Serial.println(" ml");
    Serial.print("Flow rate: ");
    Serial.print(cmd.flowRateMlMin);
    Serial.println(" ml/min");
    Serial.print("Calculated distance: ");
    Serial.print(cmd.distanceMm);
    Serial.println(" mm");
    Serial.print("Calculated feedrate: ");
    Serial.print(cmd.feedRateMmMin);
    Serial.println(" mm/min");

    // Reset position
    sendCommand("G92 X0");
    delay(100);

    // Dispense
    snprintf(gcodeCmd, sizeof(gcodeCmd), "G1 X%.2f F%.1f", cmd.distanceMm, cmd.feedRateMmMin);
    sendCommand(gcodeCmd);

    Serial.println("Dispensing...");
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║          Test 12: Single Pump Flow Rate Control           ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");

    // Initialize UART
    UartSerial.begin(115200, SERIAL_8N1, UART_TEST_RX_PIN, UART_TEST_TX_PIN);
    Serial.println("✓ UART initialized\n");

    Serial.println("Pump Calibration:");
    Serial.print("  ml per mm: ");
    Serial.println(ML_PER_MM, 4);
    Serial.print("  steps per mm: ");
    Serial.println(STEPS_PER_MM, 1);

    Serial.println("\nCommands:");
    Serial.println("  d <volume> <flowrate> - Dispense volume at flow rate");
    Serial.println("  Example: d 5.0 10.0 (dispense 5ml at 10ml/min)");
    Serial.println("  s - Query status");
    Serial.println("  h - Home pump\n");

    delay(1000);
    sendCommand("?");
}

void loop() {
    // Handle user commands
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        if (input.startsWith("d ")) {
            float volume, flowrate;
            if (sscanf(input.c_str(), "d %f %f", &volume, &flowrate) == 2) {
                PumpCommand cmd = calculatePumpCommand(volume, flowrate);
                dispenseVolume(cmd);
            } else {
                Serial.println("Usage: d <volume_ml> <flowrate_ml/min>");
            }
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
