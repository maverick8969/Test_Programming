/**
 * Test 13: Multi-Pump Sequential Operation + Encoder Control
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - 4 peristaltic pumps (X, Y, Z, A axes)
 * - Rotary encoder with button
 * - Direct UART connection (GPIO 16/17)
 *
 * Purpose:
 * - Test sequential operation of multiple pumps
 * - Verify accurate multi-step dispensing sequences
 * - Test pump coordination and timing
 * - Use encoder to navigate and control recipe
 *
 * Functionality:
 * - Define dispensing recipes (pump sequence)
 * - Execute recipes step-by-step
 * - Wait for completion between steps
 * - Report progress
 *
 * Encoder Controls:
 * - Rotate: Navigate recipe steps
 * - Press: Start/pause recipe
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
const float SAFE_TEST_FEEDRATE = 300.0; // Max feedrate for testing safety

// Example recipe: Mix of 4 components (flow rates limited to 300 mm/min feedrate)
RecipeStep recipe[] = {
    {'X', 10.0, 7.5},  // Pump X: 10ml at 7.5ml/min (150 mm/min feedrate)
    {'Y', 5.0, 6.0},   // Pump Y: 5ml at 6.0ml/min (120 mm/min feedrate)
    {'Z', 7.5, 4.5},   // Pump Z: 7.5ml at 4.5ml/min (90 mm/min feedrate)
    {'A', 2.5, 3.0}    // Pump A: 2.5ml at 3.0ml/min (60 mm/min feedrate)
};
const int recipeSteps = sizeof(recipe) / sizeof(RecipeStep);
int currentStep = 0;
bool recipeRunning = false;
bool waitingForCompletion = false;

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

int selectedStep = 0;

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

    // Constrain feedrate to max safe value for testing
    if (feedRateMmMin > SAFE_TEST_FEEDRATE) {
        feedRateMmMin = SAFE_TEST_FEEDRATE;
    }

    Serial.println("\n[Step " + String(step + 1) + "/" + String(recipeSteps) + "]");
    Serial.print("Pump ");
    Serial.print(s.pump);
    Serial.print(": ");
    Serial.print(s.volumeMl);
    Serial.print("ml at ");
    Serial.print(s.flowRateMlMin);
    Serial.print("ml/min (");
    Serial.print(feedRateMmMin, 1);
    Serial.println(" mm/min)");

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
    if (!recipeRunning) {
        // Navigate steps when not running
        int direction = readEncoder();
        if (direction != 0) {
            selectedStep = ((encoder.position % recipeSteps) + recipeSteps) % recipeSteps;
            Serial.print("Encoder: Step ");
            Serial.print(selectedStep + 1);
            Serial.print(" - Pump ");
            Serial.print(recipe[selectedStep].pump);
            Serial.print(": ");
            Serial.print(recipe[selectedStep].volumeMl);
            Serial.println("ml");
        }
    }

    // Encoder button press - start/pause recipe
    if (readEncoderButton() && encButton.pressed) {
        if (!recipeRunning) {
            Serial.println("Encoder: START recipe");
            currentStep = 0;
            recipeRunning = true;
            executeRecipeStep(currentStep);
        } else {
            Serial.println("Encoder: PAUSE recipe");
            sendCommand("!");
            recipeRunning = false;
            waitingForCompletion = false;
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║    Test 13: Multi-Pump Sequential Operation + Encoder     ║");
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

    Serial.println("\nControls:");
    Serial.println("  ENCODER rotate  - Navigate recipe steps");
    Serial.println("  ENCODER button  - Start/pause recipe");
    Serial.println("\nCommands:");
    Serial.println("  r - Run recipe");
    Serial.println("  ! or x - EMERGENCY STOP (stop all pumps immediately)");
    Serial.println("  ~ or c - Resume from HOLD (after emergency stop)");
    Serial.println("  $ - Reset system (Ctrl-X + unlock)");
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

        if (input == "r") {
            currentStep = 0;
            recipeRunning = true;
            Serial.println("\nStarting recipe...");
            executeRecipeStep(currentStep);
        } else if (input == "!" || input == "x") {
            Serial.println("\n⚠ EMERGENCY STOP!");
            sendCommand("!");
            recipeRunning = false;
            waitingForCompletion = false;
            Serial.println("All pumps stopped (HOLD state)");
            Serial.println("Type '~' to resume or '$' to reset");
        } else if (input == "~" || input == "c") {
            Serial.println("\nResuming from HOLD...");
            sendCommand("~");
            Serial.println("System resumed");
        } else if (input == "$") {
            Serial.println("\nResetting system...");
            UartSerial.write(0x18);  // Ctrl-X soft reset
            UartSerial.flush();
            delay(100);
            sendCommand("$X");  // Unlock
            Serial.println("System reset and unlocked");
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

    delay(1);
}
