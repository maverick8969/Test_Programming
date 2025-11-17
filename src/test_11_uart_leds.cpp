/**
 * Test 11: UART Communication with LED + LCD Status + Encoder Control
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - 32 WS2812B LEDs (4 strips × 8 LEDs)
 * - 1602 LCD with I2C backpack
 * - Rotary encoder with button
 * - Direct UART connection (GPIO 16/17)
 *
 * Purpose:
 * - Test UART communication with visual LED + LCD feedback
 * - Automated pump cycling to demonstrate LED color mapping
 * - Show individual pump activity with per-strip LED colors
 * - Display test status and pump info on LCD
 * - Indicate system status visually
 * - Use encoder to control brightness and start/stop tests
 *
 * LED Indicators:
 * - During automated test:
 *   - Strip 0 (Pump X): Cyan when active, dim cyan when idle
 *   - Strip 1 (Pump Y): Magenta when active, dim magenta when idle
 *   - Strip 2 (Pump Z): Yellow when active, dim yellow when idle
 *   - Strip 3 (Pump A): White when active, dim white when idle
 * - System status mode:
 *   - All strips: Green = Idle, Blue = Running, Red = Error
 *
 * Encoder Controls:
 * - Rotate: Adjust LED brightness (0-255)
 * - Press: Start/stop automated pump test cycle
 *
 * Automated Test:
 * - Cycles through all 4 pumps sequentially
 * - Each pump runs for 3 seconds with 10mm movement @ F150
 * - LED shows active pump with bright color, others dim
 * - Automatically loops through all pumps continuously
 *
 * Build command:
 *   pio run -e test_11_uart_leds -t upload -t monitor
 */

#include <Arduino.h>
#include <FastLED.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include "esp_bt.h"
#include "pin_definitions.h"

#define UartSerial         Serial2

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 16, 2);

CRGB leds[LED_TOTAL_COUNT];
enum SystemState { IDLE, RUNNING, ERROR };
SystemState currentState = IDLE;

// Automated test mode
bool autoTestActive = false;
int currentPump = 0;  // 0=X, 1=Y, 2=Z, 3=A
unsigned long lastPumpChange = 0;
const unsigned long PUMP_TEST_DURATION = 3000;  // 3 seconds per pump
bool waitingForIdle = false;

// Pump colors
const CRGB pumpColors[4] = {
    CRGB::Cyan,    // Pump X
    CRGB::Magenta, // Pump Y
    CRGB::Yellow,  // Pump Z
    CRGB::White    // Pump A
};

const char* pumpNames[4] = {"X", "Y", "Z", "A"};
const char* colorNames[4] = {"Cyan", "Magenta", "Yellow", "White"};

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

// LED control
int ledBrightness = 50;  // 0-255
bool testPatternActive = false;

// Forward declarations
void startPumpTest(int pump);

void setStripColor(int strip, CRGB color) {
    int start = strip * LED_PER_STRIP;
    for (int i = 0; i < LED_PER_STRIP; i++) {
        leds[start + i] = color;
    }
    FastLED.show();
}

void setAllStrips(CRGB color) {
    fill_solid(leds, LED_TOTAL_COUNT, color);
    FastLED.show();
}

void updateLCD(const char* line1, const char* line2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
}

void sendCommand(const char* cmd) {
    Serial.print("→ ");
    Serial.println(cmd);
    UartSerial.println(cmd);
    UartSerial.flush();
}

void updateLEDs() {
    if (testPatternActive) {
        // Rainbow test pattern
        static uint8_t hue = 0;
        for (int i = 0; i < LED_TOTAL_COUNT; i++) {
            leds[i] = CHSV(hue + (i * 8), 255, ledBrightness);
        }
        hue++;
    } else if (autoTestActive) {
        // Show per-pump feedback during automated test
        // All strips dim, active pump bright
        for (int strip = 0; strip < 4; strip++) {
            CRGB color = pumpColors[strip];
            if (strip == currentPump && currentState == RUNNING) {
                // Active pump - bright
                setStripColor(strip, color);
            } else {
                // Inactive pumps - dim (10%)
                CRGB dimColor = color;
                dimColor.nscale8(25);
                setStripColor(strip, dimColor);
            }
        }
    } else {
        // Default system state colors
        switch (currentState) {
            case IDLE:
                setAllStrips(CRGB::Green);
                break;
            case RUNNING:
                setAllStrips(CRGB::Blue);
                break;
            case ERROR:
                setAllStrips(CRGB::Red);
                break;
        }
    }
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
    // Check for rotation - adjust brightness
    int direction = readEncoder();
    if (direction != 0) {
        ledBrightness += direction * 5;
        ledBrightness = constrain(ledBrightness, 0, 255);
        FastLED.setBrightness(ledBrightness);
        Serial.print("Encoder: Brightness = ");
        Serial.println(ledBrightness);

        // Update LCD with brightness
        if (!autoTestActive) {
            char line1[17], line2[17];
            snprintf(line1, sizeof(line1), "LED Brightness:");
            snprintf(line2, sizeof(line2), "%d / 255", ledBrightness);
            updateLCD(line1, line2);
        }
    }

    // Check encoder button press - toggle automated test
    if (readEncoderButton() && encButton.pressed) {
        if (!autoTestActive) {
            // Start automated test
            autoTestActive = true;
            testPatternActive = false;
            currentPump = 0;
            lastPumpChange = millis();
            Serial.println("\n=== AUTOMATED PUMP TEST STARTED ===");
            Serial.println("Cycling through all 4 pumps...\n");
            updateLCD("AUTO TEST", "Starting...");
            delay(1000);
            startPumpTest(currentPump);
        } else {
            // Stop automated test
            autoTestActive = false;
            sendCommand("!");  // Stop any running pump
            currentState = IDLE;
            Serial.println("\n=== AUTOMATED TEST STOPPED ===\n");
            updateLCD("AUTO TEST", "Stopped");
            delay(1000);
            updateLCD("System IDLE", "Press to start");
        }
    }
}

void startPumpTest(int pump) {
    if (pump >= 4) {
        // Test complete - restart cycle
        Serial.println("\n✓ All pumps tested - restarting cycle\n");
        updateLCD("Cycle Complete", "Restarting...");
        delay(1000);
        currentPump = 0;
        pump = 0;
    }

    Serial.print("Testing Pump ");
    Serial.print(pumpNames[pump]);
    Serial.print(" (LED color: ");
    Serial.print(colorNames[pump]);
    Serial.println(")");

    // Update LCD with current pump info
    char line1[17], line2[17];
    snprintf(line1, sizeof(line1), "Pump %s (%d/4)", pumpNames[pump], pump + 1);
    snprintf(line2, sizeof(line2), "LED: %s", colorNames[pump]);
    updateLCD(line1, line2);

    // Reset position and move pump
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "G92 %c0", pumpNames[pump][0]);
    sendCommand(cmd);
    delay(100);

    // Move pump (F150 for safe testing)
    snprintf(cmd, sizeof(cmd), "G1 %c10 F150", pumpNames[pump][0]);
    sendCommand(cmd);

    waitingForIdle = true;
    currentState = RUNNING;
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║      Test 11: UART Communication + LED + Encoder          ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");

    // Disable WiFi and Bluetooth to prevent LED data corruption
    WiFi.mode(WIFI_OFF);
    btStop();
    Serial.println("✓ WiFi/BT disabled (prevents LED timing interference)");

    // Initialize LCD
    Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN);
    lcd.init();
    lcd.backlight();
    updateLCD("Test 11: UART", "LED + LCD Test");
    delay(1000);
    Serial.println("✓ LCD initialized");

    // Initialize LEDs
    FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, LED_TOTAL_COUNT);
    FastLED.setBrightness(50);
    FastLED.clear(true);  // Clear buffer to remove garbage data
    delay(50);  // Stabilize RMT peripheral
    setAllStrips(CRGB::Green);
    Serial.println("✓ LEDs initialized (Green = IDLE)");

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

    Serial.println("LED Status Codes:");
    Serial.println("  Green   = System IDLE");
    Serial.println("  Blue    = System RUNNING");
    Serial.println("  Red     = ERROR state");
    Serial.println("\nPer-Pump LED Colors (during automated test):");
    Serial.println("  Cyan    = Pump X active");
    Serial.println("  Magenta = Pump Y active");
    Serial.println("  Yellow  = Pump Z active");
    Serial.println("  White   = Pump A active\n");

    Serial.println("Controls:");
    Serial.println("  ENCODER rotate  - Adjust brightness (0-255)");
    Serial.println("  ENCODER button  - Start/stop automated pump test");
    Serial.println("\nCommands:");
    Serial.println("  a - Start automated test (cycles all pumps)");
    Serial.println("  s - Stop automated test");
    Serial.println("  x/y/z/a - Manually test individual pump\n");

    updateLCD("System IDLE", "Press to start");
    delay(1000);
    sendCommand("?");
}

void loop() {
    // Handle encoder input
    handleEncoder();

    // Handle automated test cycling
    if (autoTestActive && !waitingForIdle) {
        if (millis() - lastPumpChange >= PUMP_TEST_DURATION) {
            currentPump++;
            if (currentPump >= 4) currentPump = 0;
            lastPumpChange = millis();
            startPumpTest(currentPump);
        }
    }

    // Handle serial commands
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        input.toLowerCase();

        if (input == "a") {
            // Start automated test
            if (!autoTestActive) {
                autoTestActive = true;
                testPatternActive = false;
                currentPump = 0;
                lastPumpChange = millis();
                Serial.println("\n=== AUTOMATED PUMP TEST STARTED ===");
                Serial.println("Cycling through all 4 pumps...\n");
                updateLCD("AUTO TEST", "Starting...");
                delay(1000);
                startPumpTest(currentPump);
            }
        } else if (input == "s") {
            // Stop automated test
            if (autoTestActive) {
                autoTestActive = false;
                sendCommand("!");
                currentState = IDLE;
                Serial.println("\n=== AUTOMATED TEST STOPPED ===\n");
                updateLCD("AUTO TEST", "Stopped");
                delay(1000);
                updateLCD("System IDLE", "Press to start");
            }
        } else if (input == "x" || input == "y" || input == "z" || input == "a") {
            // Manual pump test
            autoTestActive = false;
            testPatternActive = false;
            int pump = (input == "x") ? 0 : (input == "y") ? 1 : (input == "z") ? 2 : 3;
            currentPump = pump;
            Serial.println("\nManual pump test:");
            updateLCD("Manual Test", pumpNames[pump]);
            delay(500);
            startPumpTest(pump);
        }
    }

    // Process received data
    if (UartSerial.available()) {
        String response = UartSerial.readStringUntil('\n');
        response.trim();

        if (response.length() > 0) {
            Serial.print("← ");
            Serial.println(response);

            // Parse state and update LEDs
            if (response.indexOf("Idle") >= 0) {
                if (waitingForIdle) {
                    Serial.println("✓ Pump movement complete\n");
                    waitingForIdle = false;
                    lastPumpChange = millis();
                }
                currentState = IDLE;
                if (!autoTestActive) {
                    updateLCD("System IDLE", "Press to start");
                }
            } else if (response.indexOf("Run") >= 0 || response.indexOf("Jog") >= 0) {
                currentState = RUNNING;
            } else if (response.indexOf("error") >= 0 || response.indexOf("ALARM") >= 0) {
                currentState = ERROR;
                autoTestActive = false;
                Serial.println("⚠️  ERROR detected - stopping automated test");
                updateLCD("ERROR!", "Test stopped");
            }
        }
    }

    // Update LEDs
    updateLEDs();
    FastLED.show();

    // Periodically query status (only when not in automated test to reduce serial traffic)
    if (!autoTestActive) {
        static unsigned long lastQuery = 0;
        if (millis() - lastQuery > 1000) {
            sendCommand("?");
            lastQuery = millis();
        }
    }

    delay(10);
}
