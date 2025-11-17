/**
 * Test 19: Full System Integration Test
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - 4 peristaltic pumps
 * - LCD display (1602 I2C)
 * - 32 WS2812B LEDs
 * - 3 control buttons
 * - Rotary encoder
 * - Digital scale (optional)
 * - Direct UART connection (GPIO 16/17)
 *
 * Purpose:
 * - Test complete integrated system
 * - Verify all subsystems working together
 * - Full user interface integration
 * - Production-ready functionality
 *
 * Features:
 * - Recipe selection via encoder
 * - LCD status display
 * - LED visual feedback
 * - Button control
 * - Safety monitoring
 * - Data logging
 *
 * Build command:
 *   pio run -e test_19_full_integration -t upload -t monitor
 */

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <FastLED.h>
#include <WiFi.h>
#include "esp_bt.h"
#include "pin_definitions.h"

#define UartSerial         Serial2

// Peripherals
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 16, 2);
CRGB leds[LED_TOTAL_COUNT];

// Encoder variables
volatile int encoderPos = 0;
int lastEncoderPos = 0;

// System state
enum SystemMode { MODE_IDLE, MODE_SELECT, MODE_RUNNING, MODE_COMPLETE, MODE_ERROR };
SystemMode currentMode = MODE_IDLE;

// Recipe structure
struct Recipe {
    const char* name;
    float volumes[4];  // X, Y, Z, A
    float flowRate;
};

Recipe recipes[] = {
    {"Water Flush", {10, 10, 10, 10}, 30.0},
    {"Color Mix A", {5, 3, 2, 0}, 15.0},
    {"Color Mix B", {3, 5, 2, 0}, 15.0},
    {"Nutrient 1:1", {10, 10, 0, 0}, 20.0}
};
const int recipeCount = 4;
int selectedRecipe = 0;
int currentStep = 0;

const float ML_PER_MM = 0.05;
const float MAX_FEEDRATE_MM_MIN = 300.0; // Max feedrate for testing safety
bool waitingForIdle = false;

void IRAM_ATTR encoderISR() {
    static unsigned long lastInterrupt = 0;
    unsigned long now = millis();
    if (now - lastInterrupt > 5) {
        if (digitalRead(ENCODER_DT_PIN) == HIGH) {
            encoderPos++;
        } else {
            encoderPos--;
        }
        lastInterrupt = now;
    }
}

void sendCommand(const char* cmd) {
    Serial.print("→ ");
    Serial.println(cmd);
    UartSerial.println(cmd);
    UartSerial.flush();
}

void updateDisplay() {
    lcd.clear();
    char line1[17], line2[17];

    switch (currentMode) {
        case MODE_IDLE:
            strcpy(line1, "Pump System");
            strcpy(line2, "Press SELECT");
            fill_solid(leds, LED_TOTAL_COUNT, CRGB::Green);
            break;

        case MODE_SELECT:
            snprintf(line1, sizeof(line1), "Recipe %d/%d", selectedRecipe + 1, recipeCount);
            snprintf(line2, sizeof(line2), "%.14s", recipes[selectedRecipe].name);
            fill_solid(leds, LED_TOTAL_COUNT, CRGB::Blue);
            break;

        case MODE_RUNNING:
            snprintf(line1, sizeof(line1), "Running %d/4", currentStep + 1);
            snprintf(line2, sizeof(line2), "%.14s", recipes[selectedRecipe].name);
            // Show progress on LEDs
            int litLEDs = (currentStep + 1) * 8;
            fill_solid(leds, litLEDs, CRGB::Cyan);
            fill_solid(leds + litLEDs, LED_TOTAL_COUNT - litLEDs, CRGB::Black);
            break;

        case MODE_COMPLETE:
            strcpy(line1, "Complete!");
            snprintf(line2, sizeof(line2), "%.14s", recipes[selectedRecipe].name);
            fill_solid(leds, LED_TOTAL_COUNT, CRGB::Green);
            break;

        case MODE_ERROR:
            strcpy(line1, "ERROR!");
            strcpy(line2, "Press STOP");
            fill_solid(leds, LED_TOTAL_COUNT, CRGB::Red);
            break;
    }

    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
    FastLED.show();
}

void executeRecipeStep() {
    if (currentStep >= 4) {
        currentMode = MODE_COMPLETE;
        updateDisplay();
        delay(3000);
        currentMode = MODE_IDLE;
        updateDisplay();
        return;
    }

    Recipe r = recipes[selectedRecipe];
    float volume = r.volumes[currentStep];

    if (volume > 0) {
        char pump = 'X' + currentStep;
        float distMm = volume / ML_PER_MM;
        float feedRate = r.flowRate / ML_PER_MM;

        // Constrain feedrate to max safe value for testing
        if (feedRate > MAX_FEEDRATE_MM_MIN) {
            feedRate = MAX_FEEDRATE_MM_MIN;
        }

        Serial.print("Step ");
        Serial.print(currentStep + 1);
        Serial.print(": Pump ");
        Serial.print(pump);
        Serial.print(" - ");
        Serial.print(volume);
        Serial.print("ml (feedrate: ");
        Serial.print(feedRate, 1);
        Serial.println(" mm/min)");

        char cmd[64];
        snprintf(cmd, sizeof(cmd), "G92 %c0", pump);
        sendCommand(cmd);
        delay(100);

        snprintf(cmd, sizeof(cmd), "G1 %c%.2f F%.1f", pump, distMm, feedRate);
        sendCommand(cmd);

        waitingForIdle = true;
    } else {
        // Skip this pump
        currentStep++;
        executeRecipeStep();
    }
}

void handleButtons() {
    // Encoder button (SELECT)
    if (digitalRead(ENCODER_SW_PIN) == LOW) {
        delay(200);  // Debounce
        if (currentMode == MODE_IDLE) {
            currentMode = MODE_SELECT;
            selectedRecipe = 0;
            encoderPos = 0;
            updateDisplay();
        } else if (currentMode == MODE_SELECT) {
            currentMode = MODE_RUNNING;
            currentStep = 0;
            updateDisplay();
            delay(1000);
            executeRecipeStep();
        }
        while (digitalRead(ENCODER_SW_PIN) == LOW) delay(10);
    }

    // START button
    if (digitalRead(START_BUTTON_PIN) == LOW) {
        delay(200);
        if (currentMode == MODE_SELECT || currentMode == MODE_COMPLETE) {
            currentMode = MODE_RUNNING;
            currentStep = 0;
            updateDisplay();
            delay(1000);
            executeRecipeStep();
        }
        while (digitalRead(START_BUTTON_PIN) == LOW) delay(10);
    }

    // STOP button
    if (digitalRead(STOP_BUTTON_PIN) == LOW) {
        delay(200);
        sendCommand("!");  // Emergency stop
        currentMode = MODE_IDLE;
        updateDisplay();
        while (digitalRead(STOP_BUTTON_PIN) == LOW) delay(10);
    }
}

void handleEncoder() {
    if (currentMode == MODE_SELECT && encoderPos != lastEncoderPos) {
        selectedRecipe = ((encoderPos % recipeCount) + recipeCount) % recipeCount;
        lastEncoderPos = encoderPos;
        updateDisplay();
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║           Test 19: Full System Integration                ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");

    // Initialize LCD
    Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN);
    lcd.init();
    lcd.backlight();
    Serial.println("✓ LCD initialized");

    // Disable WiFi and Bluetooth to prevent LED data corruption
    WiFi.mode(WIFI_OFF);
    btStop();

    // Initialize LEDs
    FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, LED_TOTAL_COUNT);
    FastLED.setBrightness(50);
    FastLED.clear(true);  // Clear buffer to remove garbage data
    delay(50);  // Stabilize RMT peripheral
    Serial.println("✓ LEDs initialized (WiFi/BT disabled)");

    // Initialize buttons and encoder
    pinMode(START_BUTTON_PIN, INPUT_PULLUP);
    pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);
    pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(ENCODER_DT_PIN, INPUT_PULLUP);
    pinMode(ENCODER_SW_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN), encoderISR, FALLING);
    Serial.println("✓ Controls initialized");

    // Initialize UART
    UartSerial.begin(115200, SERIAL_8N1, UART_TEST_RX_PIN, UART_TEST_TX_PIN);
    Serial.println("✓ UART initialized\n");

    Serial.println("Available Recipes:");
    for (int i = 0; i < recipeCount; i++) {
        Serial.print("  ");
        Serial.print(i + 1);
        Serial.print(". ");
        Serial.println(recipes[i].name);
    }

    Serial.println("\nOperation:");
    Serial.println("  1. Press SELECT to choose recipe");
    Serial.println("  2. Rotate encoder to browse");
    Serial.println("  3. Press SELECT or START to begin");
    Serial.println("  4. Press STOP for emergency stop\n");

    updateDisplay();
    delay(1000);
    sendCommand("?");
}

void loop() {
    handleButtons();
    handleEncoder();

    // Process UART responses
    if (UartSerial.available()) {
        String response = UartSerial.readStringUntil('\n');
        response.trim();
        if (response.length() > 0) {
            Serial.print("← ");
            Serial.println(response);

            if (waitingForIdle && response.indexOf("Idle") >= 0) {
                waitingForIdle = false;
                currentStep++;
                updateDisplay();
                delay(500);
                executeRecipeStep();
            }

            if (response.indexOf("error") >= 0 || response.indexOf("ALARM") >= 0) {
                currentMode = MODE_ERROR;
                updateDisplay();
            }
        }
    }

    delay(10);
}
