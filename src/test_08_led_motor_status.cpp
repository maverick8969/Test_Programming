/**
 * Test 08: LED Motor Status Display
 *
 * Combines LED control with RS485 status monitoring to show which motors are active.
 * When a motor is jogging, its corresponding LED strip will light up brightly.
 *
 * Hardware:
 * - 32 WS2812B LEDs (4 strips × 8 LEDs each) on GPIO 25
 * - RS485 connection to BTT Rodent Board (TX: GPIO 17, RX: GPIO 16)
 * - BTT Rodent running FluidNC
 *
 * LED to Motor Mapping:
 * - Strip 0 (LEDs 0-7):   X-axis (Pump 1) → Cyan
 * - Strip 1 (LEDs 8-15):  Y-axis (Pump 2) → Magenta
 * - Strip 2 (LEDs 16-23): Z-axis (Pump 3) → Yellow
 * - Strip 3 (LEDs 24-31): A-axis (Pump 4) → White
 *
 * Status Parsing:
 * - Parses FluidNC status messages like: <Jog|MPos:89.000,83.703,78.008,39.000|FS:4,0>
 * - Detects position changes to determine which motor is moving
 * - Updates LED brightness and color based on motor activity
 *
 * Build command:
 *   pio run -e test_08_led_motor_status -t upload -t monitor
 */

#include <Arduino.h>
#include <FastLED.h>
#include "pin_definitions.h"

// LED Configuration
#define NUM_LEDS        32
#define LEDS_PER_STRIP  8
#define NUM_STRIPS      4
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB
#define BRIGHTNESS      128      // Mid-brightness for active motors

// RS485 Configuration
#define RODENT_BAUD     115200
#define RODENT_CONFIG   SERIAL_8N1
#define RodentSerial    Serial1
#define USE_DIRECTION_CONTROL   false

// Buffer for incoming data
#define RX_BUFFER_SIZE  512
char rxBuffer[RX_BUFFER_SIZE];
uint16_t rxIndex = 0;

// LED array
CRGB leds[NUM_LEDS];

// Motor position tracking
#define NUM_AXES 4
float currentPos[NUM_AXES] = {0, 0, 0, 0};
float previousPos[NUM_AXES] = {0, 0, 0, 0};
bool motorActive[NUM_AXES] = {false, false, false, false};
unsigned long lastMovementTime[NUM_AXES] = {0, 0, 0, 0};
#define MOVEMENT_THRESHOLD  0.001  // Minimum position change to detect movement (mm)
#define ACTIVE_TIMEOUT      500    // Time to keep LED lit after movement stops (ms)

// Status query timing
unsigned long lastStatusQuery = 0;
#define STATUS_QUERY_INTERVAL  100  // Query status every 100ms

// Motor colors (active state)
const CRGB motorColors[NUM_STRIPS] = {
    CRGB::Cyan,      // X-axis (Pump 1)
    CRGB::Magenta,   // Y-axis (Pump 2)
    CRGB::Yellow,    // Z-axis (Pump 3)
    CRGB::White      // A-axis (Pump 4)
};

const char* axisNames[NUM_AXES] = {"X", "Y", "Z", "A"};

// Statistics
unsigned long totalStatusMessages = 0;
unsigned long successfulParses = 0;
unsigned long failedParses = 0;

/**
 * Set color for specific LED strip (0-3)
 */
void setStripColor(uint8_t stripNum, CRGB color) {
    if (stripNum >= NUM_STRIPS) return;

    uint16_t startLED = stripNum * LEDS_PER_STRIP;
    uint16_t endLED = startLED + LEDS_PER_STRIP;

    for (uint16_t i = startLED; i < endLED; i++) {
        leds[i] = color;
    }
}

/**
 * Set all LEDs to one color
 */
void setAllColor(CRGB color) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        leds[i] = color;
    }
}

/**
 * Update LED display based on motor activity
 */
void updateLEDs() {
    unsigned long now = millis();

    for (uint8_t axis = 0; axis < NUM_AXES; axis++) {
        // Check if motor is still considered active
        if (motorActive[axis] && (now - lastMovementTime[axis] > ACTIVE_TIMEOUT)) {
            motorActive[axis] = false;
        }

        // Set LED color based on activity
        if (motorActive[axis]) {
            // Active: bright assigned color
            setStripColor(axis, motorColors[axis]);
        } else {
            // Idle: dim assigned color (10% brightness)
            CRGB dimColor = motorColors[axis];
            dimColor.nscale8(25);  // Scale to ~10% brightness
            setStripColor(axis, dimColor);
        }
    }

    FastLED.show();
}

/**
 * Parse MPos values from status message
 * Example: <Jog|MPos:89.000,83.703,78.008,39.000|FS:4,0>
 */
bool parseStatusMessage(const char* msg) {
    // Look for "MPos:" in the message
    const char* mposStart = strstr(msg, "MPos:");
    if (!mposStart) {
        return false;
    }

    // Move past "MPos:"
    mposStart += 5;

    // Parse up to 4 axis positions
    float positions[NUM_AXES];
    int axisCount = 0;

    char* endPtr;
    const char* ptr = mposStart;

    while (axisCount < NUM_AXES) {
        positions[axisCount] = strtof(ptr, &endPtr);

        if (ptr == endPtr) {
            // No valid number found
            break;
        }

        axisCount++;
        ptr = endPtr;

        // Check for comma (more values) or end of MPos section
        if (*ptr == ',') {
            ptr++;  // Skip comma
        } else {
            break;  // End of MPos values
        }
    }

    if (axisCount == 0) {
        return false;
    }

    // Update positions and detect movement
    unsigned long now = millis();
    bool anyMovement = false;

    for (int i = 0; i < axisCount && i < NUM_AXES; i++) {
        previousPos[i] = currentPos[i];
        currentPos[i] = positions[i];

        // Detect movement
        float delta = abs(currentPos[i] - previousPos[i]);
        if (delta >= MOVEMENT_THRESHOLD) {
            if (!motorActive[i]) {
                Serial.print("→ ");
                Serial.print(axisNames[i]);
                Serial.print("-axis ACTIVE (");
                Serial.print(currentPos[i], 3);
                Serial.println(" mm)");
                motorActive[i] = true;
            }
            lastMovementTime[i] = now;
            anyMovement = true;
        }
    }

    return true;
}

/**
 * Process received line from RS485
 */
void processLine(const char* line, size_t len) {
    if (len == 0) return;

    // Check if it's a status message (starts with '<')
    if (line[0] == '<') {
        totalStatusMessages++;

        if (parseStatusMessage(line)) {
            successfulParses++;
        } else {
            failedParses++;
        }
    }
}

/**
 * Send command to Rodent board
 */
void sendCommand(const char* cmd) {
    if (USE_DIRECTION_CONTROL) {
        digitalWrite(RODENT_RTS_PIN, HIGH);
        delayMicroseconds(10);
    }

    RodentSerial.print(cmd);
    RodentSerial.print("\n");

    if (USE_DIRECTION_CONTROL) {
        delayMicroseconds(10);
        RodentSerial.flush();
        digitalWrite(RODENT_RTS_PIN, LOW);
    } else {
        RodentSerial.flush();
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║          Test 08: LED Motor Status Display                ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");

    // Initialize LEDs
    Serial.println("\n[Initializing LEDs]");
    Serial.print("LED Count:        "); Serial.println(NUM_LEDS);
    Serial.print("Strips:           "); Serial.println(NUM_STRIPS);
    Serial.print("LEDs per Strip:   "); Serial.println(LEDS_PER_STRIP);
    Serial.print("Data Pin:         GPIO "); Serial.println(LED_DATA_PIN);

    FastLED.addLeds<LED_TYPE, LED_DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.setMaxRefreshRate(120);
    Serial.println("✓ FastLED initialized");

    // Test all LEDs
    Serial.println("\nTesting all LEDs white for 1 second...");
    setAllColor(CRGB::White);
    FastLED.show();
    delay(1000);

    // Show initial dim state
    setAllColor(CRGB::Black);
    FastLED.show();
    Serial.println("✓ LED test complete");

    // Initialize RS485
    Serial.println("\n[Initializing RS485]");
    Serial.print("TX Pin:           GPIO "); Serial.println(RODENT_TX_PIN);
    Serial.print("RX Pin:           GPIO "); Serial.println(RODENT_RX_PIN);
    Serial.print("Baud Rate:        "); Serial.println(RODENT_BAUD);

    if (USE_DIRECTION_CONTROL) {
        pinMode(RODENT_RTS_PIN, OUTPUT);
        digitalWrite(RODENT_RTS_PIN, LOW);
        Serial.println("✓ RTS pin configured");
    } else {
        Serial.println("✓ Using automatic direction control");
    }

    RodentSerial.begin(RODENT_BAUD, RODENT_CONFIG, RODENT_RX_PIN, RODENT_TX_PIN);
    RodentSerial.setRxBufferSize(512);
    delay(100);
    Serial.println("✓ Serial port initialized");

    // Motor-to-LED mapping
    Serial.println("\n[Motor-to-LED Mapping]");
    for (uint8_t i = 0; i < NUM_STRIPS; i++) {
        Serial.print("Strip "); Serial.print(i);
        Serial.print(" (LEDs "); Serial.print(i * LEDS_PER_STRIP);
        Serial.print("-"); Serial.print((i + 1) * LEDS_PER_STRIP - 1);
        Serial.print("): ");
        Serial.print(axisNames[i]);
        Serial.print("-axis → ");

        if (motorColors[i] == CRGB::Cyan) Serial.println("Cyan");
        else if (motorColors[i] == CRGB::Magenta) Serial.println("Magenta");
        else if (motorColors[i] == CRGB::Yellow) Serial.println("Yellow");
        else if (motorColors[i] == CRGB::White) Serial.println("White");
    }

    Serial.println("\n[Status]");
    Serial.println("→ Monitoring motor activity...");
    Serial.println("→ Jog motors to see LED feedback");
    Serial.println("→ Active motors will show BRIGHT color");
    Serial.println("→ Idle motors will show DIM color");
    Serial.println();

    // Initialize with dim LEDs
    updateLEDs();
    lastStatusQuery = millis();
}

void loop() {
    unsigned long now = millis();

    // Send status query periodically
    if (now - lastStatusQuery >= STATUS_QUERY_INTERVAL) {
        sendCommand("?");
        lastStatusQuery = now;
    }

    // Process incoming RS485 data
    while (RodentSerial.available()) {
        char c = RodentSerial.read();

        if (rxIndex < RX_BUFFER_SIZE - 1) {
            rxBuffer[rxIndex++] = c;
        }

        if (c == '\n' || c == '\r') {
            if (rxIndex > 1) {  // Ignore empty lines
                rxBuffer[rxIndex] = '\0';
                processLine(rxBuffer, rxIndex);
            }
            rxIndex = 0;
        }
    }

    // Update LED display
    updateLEDs();

    // Statistics every 10 seconds
    static unsigned long lastStatsTime = 0;
    if (now - lastStatsTime >= 10000) {
        Serial.println("\n[Statistics]");
        Serial.print("Status messages:  "); Serial.println(totalStatusMessages);
        Serial.print("Successful parse: "); Serial.println(successfulParses);
        Serial.print("Failed parse:     "); Serial.println(failedParses);
        Serial.print("Free heap:        "); Serial.print(ESP.getFreeHeap() / 1024.0, 1);
        Serial.println(" KB");

        Serial.println("\n[Current Positions]");
        for (uint8_t i = 0; i < NUM_AXES; i++) {
            Serial.print(axisNames[i]);
            Serial.print(": ");
            Serial.print(currentPos[i], 3);
            Serial.print(" mm  ");
            Serial.println(motorActive[i] ? "[ACTIVE]" : "[idle]");
        }
        Serial.println();

        lastStatsTime = now;
    }

    delay(10);  // Small delay to prevent overwhelming the system
}
