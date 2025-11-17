/**
 * Test 13: Multi-Pump Sequential Operation
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - 4 peristaltic pumps (X, Y, Z, A axes)
 * - Direct UART connection (GPIO 16/17)
 *
 * Purpose:
 * - Test sequential operation of multiple pumps
 * - Verify accurate multi-step dispensing sequences
 * - Test pump coordination and timing
 *
 * Functionality:
 * - Define dispensing recipes (pump sequence)
 * - Execute recipes step-by-step
 * - Wait for completion between steps
 * - Report progress
 *
 * Build command:
 *   pio run -e test_13_multi_sequential -t upload -t monitor
 */

#include <Arduino.h>
#include "pin_definitions.h"

#define UartSerial         Serial2

struct RecipeStep {
    char pump;        // 'X', 'Y', 'Z', or 'A'
    float volumeMl;
    float flowRateMlMin;
};

const float ML_PER_MM = 0.05;

// Example recipe: Mix of 4 components
RecipeStep recipe[] = {
    {'X', 10.0, 20.0},  // Pump X: 10ml at 20ml/min
    {'Y', 5.0, 15.0},   // Pump Y: 5ml at 15ml/min
    {'Z', 7.5, 10.0},   // Pump Z: 7.5ml at 10ml/min
    {'A', 2.5, 5.0}     // Pump A: 2.5ml at 5ml/min
};
const int recipeSteps = sizeof(recipe) / sizeof(RecipeStep);
int currentStep = 0;
bool recipeRunning = false;
bool waitingForCompletion = false;

void sendCommand(const char* cmd) {
    Serial.print("→ ");
    Serial.println(cmd);
    UartSerial.println(cmd);
    UartSerial.flush();
}

void executeRecipeStep(int step) {
    if (step >= recipeSteps) {
        Serial.println("\n✓ Recipe complete!");
        recipeRunning = false;
        return;
    }

    RecipeStep s = recipe[step];
    float distanceMm = s.volumeMl / ML_PER_MM;
    float feedRateMmMin = s.flowRateMlMin / ML_PER_MM;

    Serial.println("\n[Step " + String(step + 1) + "/" + String(recipeSteps) + "]");
    Serial.print("Pump ");
    Serial.print(s.pump);
    Serial.print(": ");
    Serial.print(s.volumeMl);
    Serial.print("ml at ");
    Serial.print(s.flowRateMlMin);
    Serial.println("ml/min");

    // Reset position for this pump
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "G92 %c0", s.pump);
    sendCommand(cmd);
    delay(100);

    // Execute move
    snprintf(cmd, sizeof(cmd), "G1 %c%.2f F%.1f", s.pump, distanceMm, feedRateMmMin);
    sendCommand(cmd);

    waitingForCompletion = true;
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║        Test 13: Multi-Pump Sequential Operation           ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");

    // Initialize UART
    UartSerial.begin(115200, SERIAL_8N1, UART_TEST_RX_PIN, UART_TEST_TX_PIN);
    Serial.println("✓ UART initialized\n");

    Serial.println("Recipe:");
    for (int i = 0; i < recipeSteps; i++) {
        Serial.print("  Step ");
        Serial.print(i + 1);
        Serial.print(": Pump ");
        Serial.print(recipe[i].pump);
        Serial.print(" - ");
        Serial.print(recipe[i].volumeMl);
        Serial.print("ml @ ");
        Serial.print(recipe[i].flowRateMlMin);
        Serial.println("ml/min");
    }

    Serial.println("\nCommands:");
    Serial.println("  r - Run recipe");
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

        if (input == "r") {
            currentStep = 0;
            recipeRunning = true;
            Serial.println("\nStarting recipe...");
            executeRecipeStep(currentStep);
        } else if (input == "s") {
            sendCommand("?");
        } else if (input == "h") {
            sendCommand("$H");
        }
    }

    // Process responses
    if (UartSerial.available()) {
        String response = UartSerial.readStringUntil('\n');
        response.trim();
        Serial.print("← ");
        Serial.println(response);

        // Check if move is complete (Idle state)
        if (waitingForCompletion && response.indexOf("Idle") >= 0) {
            waitingForCompletion = false;
            currentStep++;
            delay(500); // Brief pause between steps
            if (recipeRunning) {
                executeRecipeStep(currentStep);
            }
        }
    }

    delay(100);
}
