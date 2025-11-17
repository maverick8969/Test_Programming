/**
 * Test 15: Scale Integration for Weight-Based Dispensing
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - Digital scale with RS232 output (via MAX3232 converter)
 * - Direct UART connection to Rodent (GPIO 16/17)
 * - Scale connection (GPIO 35 RX, GPIO 32 TX)
 *
 * Purpose:
 * - Test weight-based dispensing control
 * - Read scale values during dispensing
 * - Stop when target weight is reached
 * - Verify closed-loop dispensing accuracy
 *
 * Functionality:
 * - Set target weight
 * - Start dispensing
 * - Monitor scale readings in real-time
 * - Stop when target reached
 * - Report actual weight vs. target
 *
 * Build command:
 *   pio run -e test_15_scale_integration -t upload -t monitor
 */

#include <Arduino.h>
#include "pin_definitions.h"

#define RodentSerial       Serial2  // To FluidNC
#define ScaleSerial        Serial1  // To digital scale

float currentWeight = 0.0;
float targetWeight = 0.0;
bool dispensing = false;
unsigned long lastScaleRead = 0;

void sendRodentCommand(const char* cmd) {
    Serial.print("→ Rodent: ");
    Serial.println(cmd);
    RodentSerial.println(cmd);
    RodentSerial.flush();
}

void readScale() {
    if (ScaleSerial.available()) {
        String scaleData = ScaleSerial.readStringUntil('\n');
        scaleData.trim();

        // Parse scale reading (format varies by scale model)
        // Common format: "  +012.34 g"
        float weight;
        if (sscanf(scaleData.c_str(), "%f", &weight) == 1) {
            currentWeight = weight;
            Serial.print("Scale: ");
            Serial.print(currentWeight, 2);
            Serial.println(" g");

            // Check if target reached
            if (dispensing && currentWeight >= targetWeight) {
                Serial.println("Target weight reached!");
                sendRodentCommand("!");  // Stop
                dispensing = false;
            }
        }
    }
}

void dispenseToWeight(char pump, float targetGrams, float flowRateMlMin) {
    Serial.println("\n[Weight-Based Dispensing]");
    Serial.print("Pump: ");
    Serial.println(pump);
    Serial.print("Target weight: ");
    Serial.print(targetGrams, 2);
    Serial.println(" g");
    Serial.print("Flow rate: ");
    Serial.print(flowRateMlMin);
    Serial.println(" ml/min");

    targetWeight = currentWeight + targetGrams;
    dispensing = true;

    // Reset pump position
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "G92 %c0", pump);
    sendRodentCommand(cmd);
    delay(100);

    // Start continuous dispensing
    float feedRate = flowRateMlMin / 0.05;  // Convert ml/min to mm/min
    snprintf(cmd, sizeof(cmd), "G1 %c1000 F%.1f", pump, feedRate);
    sendRodentCommand(cmd);

    Serial.println("Dispensing... monitoring scale");
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║      Test 15: Scale Integration - Weight-Based Control    ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");

    // Initialize UART to Rodent
    RodentSerial.begin(115200, SERIAL_8N1, UART_TEST_RX_PIN, UART_TEST_TX_PIN);
    Serial.println("✓ Rodent UART initialized");

    // Initialize UART to Scale
    ScaleSerial.begin(SCALE_BAUD_RATE, SERIAL_8N1, SCALE_RX_PIN, SCALE_TX_PIN);
    Serial.println("✓ Scale UART initialized\n");

    Serial.println("Commands:");
    Serial.println("  w <pump> <grams> <flowrate> - Dispense to weight");
    Serial.println("  Example: w X 10.5 15.0 (dispense 10.5g via pump X @ 15ml/min)");
    Serial.println("  t - Tare scale (zero)");
    Serial.println("  r - Read scale");
    Serial.println("  s - Stop dispensing\n");

    delay(1000);
}

void loop() {
    // Read scale continuously
    if (millis() - lastScaleRead > 500) {
        readScale();
        lastScaleRead = millis();
    }

    // Handle user commands
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        if (input.startsWith("w ")) {
            char pump;
            float grams, flowrate;
            if (sscanf(input.c_str(), "w %c %f %f", &pump, &grams, &flowrate) == 3) {
                dispenseToWeight(pump, grams, flowrate);
            }
        } else if (input == "t") {
            // Tare command (varies by scale - this is generic)
            ScaleSerial.println("T");
            Serial.println("Taring scale...");
        } else if (input == "r") {
            Serial.print("Current weight: ");
            Serial.print(currentWeight, 2);
            Serial.println(" g");
        } else if (input == "s") {
            sendRodentCommand("!");
            dispensing = false;
            Serial.println("Stopped");
        }
    }

    // Echo Rodent responses
    if (RodentSerial.available()) {
        while (RodentSerial.available()) {
            Serial.write(RodentSerial.read());
        }
    }

    delay(10);
}
