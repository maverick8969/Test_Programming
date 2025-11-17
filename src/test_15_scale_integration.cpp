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
float targetWeight = 10.0;  // Default target
bool dispensing = false;
String lastWeightStr = "";  // For change detection
unsigned long lastScaleRead = 0;

// Scale protocol parameters (based on working Python code)
const char SCALE_CMD[] = "@P<CR><LF>";  // Command to request weight (literal text, not control chars)
const int REPEATS_PER_BURST = 13;
const int CHAR_DELAY_MS = 7;
const int LINE_DELAY_MS = 9;
const int READ_WINDOW_MS = 160;

// Encoder state
struct EncoderState {
    int32_t position;
    bool clkState;
    bool dtState;
    bool lastClkState;
};

struct EncoderButton {
    bool pressed;
    bool lastPressed;
};

EncoderState encoder = {0, false, false, false};
EncoderButton encButton = {false, false};

// Forward declarations
void dispenseToWeight(char pump, float targetGrams, float flowRateMlMin);

void sendRodentCommand(const char* cmd) {
    Serial.print("→ Rodent: ");
    Serial.println(cmd);
    RodentSerial.println(cmd);
    RodentSerial.flush();
}

void sendScaleCommandBurst() {
    // Send burst of commands with character-level delays
    for (int repeat = 0; repeat < REPEATS_PER_BURST; repeat++) {
        // Send each character with delay
        for (int i = 0; i < strlen(SCALE_CMD); i++) {
            ScaleSerial.write(SCALE_CMD[i]);
            delay(CHAR_DELAY_MS);
        }
        // Delay between commands
        delay(LINE_DELAY_MS);
    }
    ScaleSerial.flush();
}

bool parseWeight(String line, float &weight, String &unit) {
    // Parse weight from scale response
    // Expected format: "  +012.34 g" or similar
    line.trim();
    if (line.length() == 0) return false;

    // Extract number (including sign and decimal point)
    int startIdx = -1;
    int endIdx = -1;

    for (int i = 0; i < line.length(); i++) {
        char c = line.charAt(i);
        if ((c == '+' || c == '-' || c == '.' || (c >= '0' && c <= '9')) && startIdx == -1) {
            startIdx = i;
        }
        if (startIdx != -1 && (c < '0' || c > '9') && c != '.' && c != '+' && c != '-') {
            endIdx = i;
            break;
        }
    }

    if (startIdx == -1) return false;
    if (endIdx == -1) endIdx = line.length();

    String weightStr = line.substring(startIdx, endIdx);
    weight = weightStr.toFloat();

    // Extract unit (everything after the number)
    unit = line.substring(endIdx);
    unit.trim();

    return true;
}

void readScaleWithBurst() {
    // 1. Send burst of commands
    sendScaleCommandBurst();

    // 2. Read responses during the window
    unsigned long windowEnd = millis() + READ_WINDOW_MS;
    String lastReading = "";
    float lastWeight = 0.0;
    String lastUnit = "";

    while (millis() < windowEnd) {
        if (ScaleSerial.available()) {
            String line = ScaleSerial.readStringUntil('\n');
            line.trim();

            if (line.length() > 0) {
                float weight;
                String unit;
                if (parseWeight(line, weight, unit)) {
                    lastWeight = weight;
                    lastUnit = unit;
                    lastReading = line;
                }
            }
        }
        delay(2);  // Small delay to avoid tight loop
    }

    // 3. Process last valid reading (if changed)
    if (lastReading.length() > 0) {
        String weightStr = String(lastWeight, 2);

        if (weightStr != lastWeightStr) {
            currentWeight = lastWeight;
            lastWeightStr = weightStr;

            Serial.print("Scale: ");
            Serial.print(currentWeight, 2);
            Serial.print(" ");
            Serial.print(lastUnit);
            Serial.print("   (raw: ");
            Serial.print(lastReading);
            Serial.println(")");

            // Check if target reached during dispensing
            if (dispensing && currentWeight >= targetWeight) {
                Serial.println("✓ Target weight reached!");
                sendRodentCommand("!");  // Stop
                dispensing = false;
            }
        }
    }
}

int readEncoder() {
    encoder.clkState = digitalRead(ENCODER_CLK_PIN);
    if (encoder.clkState != encoder.lastClkState && encoder.clkState == LOW) {
        encoder.dtState = digitalRead(ENCODER_DT_PIN);
        encoder.position += (encoder.dtState != encoder.clkState) ? 1 : -1;
        encoder.lastClkState = encoder.clkState;
        return (encoder.dtState != encoder.clkState) ? 1 : -1;
    }
    encoder.lastClkState = encoder.clkState;
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
    int direction = readEncoder();
    if (direction != 0 && !dispensing) {
        targetWeight += direction * 0.5;
        targetWeight = constrain(targetWeight, 0.5, 100.0);
        Serial.print("Encoder: Target weight = ");
        Serial.print(targetWeight, 1);
        Serial.println(" g");
    }

    if (readEncoderButton() && encButton.pressed && !dispensing) {
        Serial.println("Encoder: START weight-based dispense");
        // Flow rate of 7.5 ml/min gives 150 mm/min feedrate (safe default)
        dispenseToWeight('X', targetWeight, 7.5);
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
    // Constrain feedrate to max safe value for testing (300 mm/min)
    if (feedRate > 300.0) {
        feedRate = 300.0;
    }
    snprintf(cmd, sizeof(cmd), "G1 %c1000 F%.1f", pump, feedRate);
    sendRodentCommand(cmd);

    Serial.print("Dispensing... monitoring scale (feedrate: ");
    Serial.print(feedRate, 1);
    Serial.println(" mm/min)");
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

    // Initialize encoder
    pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(ENCODER_DT_PIN, INPUT_PULLUP);
    pinMode(ENCODER_SW_PIN, INPUT);
    encoder.clkState = digitalRead(ENCODER_CLK_PIN);
    encoder.dtState = digitalRead(ENCODER_DT_PIN);
    encoder.lastClkState = encoder.clkState;
    encoder.position = 0;
    Serial.println("✓ Encoder initialized");

    // Initialize UART to Scale
    ScaleSerial.begin(SCALE_BAUD_RATE, SERIAL_8N1, SCALE_RX_PIN, SCALE_TX_PIN);
    Serial.println("✓ Scale UART initialized\n");

    Serial.println("Controls:");
    Serial.println("  ENCODER rotate  - Adjust target weight (0.5-100g)");
    Serial.println("  ENCODER button  - Start dispensing to target weight");
    Serial.println("\nCommands:");
    Serial.println("  w <pump> <grams> <flowrate> - Dispense to weight");
    Serial.println("  Example: w X 10.5 15.0 (dispense 10.5g via pump X @ 15ml/min)");
    Serial.println("  t - Tare scale (zero)");
    Serial.println("  r - Read scale");
    Serial.println("  s - Stop dispensing\n");

    delay(1000);
}

void loop() {
    // Handle encoder (runs fast for good responsiveness)
    handleEncoder();

    // Scale polling with smart timing:
    // - When dispensing: poll frequently (every 200ms) for quick response
    // - When idle: poll slowly (every 2 seconds) to keep encoder responsive
    unsigned long now = millis();
    unsigned long scaleInterval = dispensing ? 200 : 2000;  // 200ms when dispensing, 2s when idle

    if (now - lastScaleRead >= scaleInterval) {
        readScaleWithBurst();
        lastScaleRead = now;
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
            Serial.println("Reading scale...");
            readScaleWithBurst();
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
