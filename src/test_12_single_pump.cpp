/**
 * Test 12: Single Pump Controlled Flow Rate + Encoder Control
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - Single peristaltic pump on X-axis
 * - Rotary encoder with button
 * - Direct UART connection (GPIO 16/17)
 *
 * Purpose:
 * - Test precise flow rate control for a single pump
 * - Verify G-code feedrate to flow rate conversion
 * - Measure dispensing accuracy
 * - Use encoder to adjust flow rate
 *
 * Functionality:
 * - Set target flow rate (ml/min)
 * - Calculate required G-code feedrate
 * - Dispense specific volume
 * - Monitor and report actual dispensing
 *
 * Encoder Controls:
 * - Rotate: Adjust flow rate (1-15 ml/min, max 300 mm/min feedrate)
 * - Press: Start dispensing
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
const float SAFE_TEST_FEEDRATE = 300.0; // Max feedrate for safe testing (mm/min)

struct PumpCommand {
    float volumeMl;
    float flowRateMlMin;
    float feedRateMmMin;
    float distanceMm;
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

// User adjustable parameters
float targetVolume = 5.0;   // ml
float targetFlowRate = 7.5; // ml/min (default for testing, gives 150 mm/min feedrate)

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

    // Constrain feedrate to max safe value for testing
    if (cmd.feedRateMmMin > SAFE_TEST_FEEDRATE) {
        cmd.feedRateMmMin = SAFE_TEST_FEEDRATE;
        cmd.flowRateMlMin = SAFE_TEST_FEEDRATE * ML_PER_MM;
    }

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
    // Check for rotation - adjust flow rate
    int direction = readEncoder();
    if (direction != 0) {
        targetFlowRate += direction * 0.5; // Smaller increments for better control
        // Max flow rate of 15 ml/min gives max feedrate of 300 mm/min
        targetFlowRate = constrain(targetFlowRate, 1.0, 15.0);
        Serial.print("Encoder: Flow rate = ");
        Serial.print(targetFlowRate, 1);
        Serial.print(" ml/min (feedrate: ");
        Serial.print(targetFlowRate / ML_PER_MM, 1);
        Serial.println(" mm/min)");
    }

    // Check encoder button press - start dispensing
    if (readEncoderButton() && encButton.pressed) {
        Serial.println("Encoder: START dispensing");
        PumpCommand cmd = calculatePumpCommand(targetVolume, targetFlowRate);
        dispenseVolume(cmd);
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║      Test 12: Single Pump Flow Rate Control + Encoder     ║");
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

    Serial.println("Pump Calibration:");
    Serial.print("  ml per mm: ");
    Serial.println(ML_PER_MM, 4);
    Serial.print("  steps per mm: ");
    Serial.println(STEPS_PER_MM, 1);

    Serial.println("\nControls:");
    Serial.println("  ENCODER rotate  - Adjust flow rate (1-15 ml/min, max 300 mm/min)");
    Serial.println("  ENCODER button  - Start dispensing");
    Serial.print("\nCurrent settings: ");
    Serial.print(targetVolume, 1);
    Serial.print("ml @ ");
    Serial.print(targetFlowRate, 1);
    Serial.println("ml/min\n");

    Serial.println("Commands:");
    Serial.println("  d <volume> <flowrate> - Dispense volume at flow rate");
    Serial.println("  Example: d 5.0 10.0 (dispense 5ml at 10ml/min)");
    Serial.println("  s - Query status");
    Serial.println("  h - Home pump\n");

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

    delay(1);
}
