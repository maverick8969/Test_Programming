/**
 * Test 11: UART Communication with LED Scrolling + LCD Status + Encoder Control
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
 * - Automated pump cycling with directional LED scrolling effects
 * - Show individual pump activity with scrolling LEDs matching pump direction
 * - Display detailed test status and pump info on LCD
 * - Emergency stop testing with visual LED feedback
 * - Use encoder to control brightness and start/stop tests
 *
 * LED Scrolling Indicators:
 * - During automated test:
 *   - Strip 0 (Pump X): Cyan scrolling in pump movement direction
 *   - Strip 1 (Pump Y): Magenta scrolling in pump movement direction
 *   - Strip 2 (Pump Z): Yellow scrolling in pump movement direction
 *   - Strip 3 (Pump A): White scrolling in pump movement direction
 *   - Forward movement: LEDs scroll left to right
 *   - Reverse movement: LEDs scroll right to left
 * - Emergency/Error states:
 *   - Emergency Stop: Flashing red on all strips
 *   - Error: Solid red on all strips
 *   - Warning: Blinking orange
 *
 * Encoder Controls:
 * - Rotate: Adjust LED brightness (0-255)
 * - Press: Start/stop automated pump test cycle
 *
 * Automated Test Modes:
 * 1. Normal cycle - Each pump runs forward then reverse with scrolling LEDs
 * 2. Emergency test - Triggers emergency stop to test LED feedback
 * 3. Error simulation - Tests error state LED patterns
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
enum SystemState { IDLE, RUNNING, ERROR, EMERGENCY };
SystemState currentState = IDLE;

// Automated test mode
bool autoTestActive = false;
int currentPump = 0;  // 0=X, 1=Y, 2=Z, 3=A
unsigned long lastPumpChange = 0;
const unsigned long PUMP_TEST_DURATION = 3000;  // 3 seconds per pump
bool waitingForIdle = false;
bool pumpDirection = true;  // true = forward/positive, false = reverse/negative
int testPhase = 0;  // 0=forward, 1=reverse, 2=emergency test

// LED scrolling animation
unsigned long lastScrollUpdate = 0;
const unsigned long SCROLL_INTERVAL = 80;  // ms between scroll steps
int scrollPosition = 0;

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

void scrollStripLEDs(int strip, CRGB color, bool forward) {
    int start = strip * LED_PER_STRIP;

    // Create scrolling pattern
    for (int i = 0; i < LED_PER_STRIP; i++) {
        int pos = forward ? (i + scrollPosition) % LED_PER_STRIP :
                           (LED_PER_STRIP - 1 - ((i + scrollPosition) % LED_PER_STRIP));

        // Create a wave pattern with 3 bright LEDs
        if (pos < 3) {
            leds[start + i] = color;
        } else if (pos < 5) {
            CRGB dimColor = color;
            dimColor.nscale8(100);
            leds[start + i] = dimColor;
        } else {
            CRGB veryDimColor = color;
            veryDimColor.nscale8(30);
            leds[start + i] = veryDimColor;
        }
    }
}

void flashAllStrips(CRGB color, bool on) {
    if (on) {
        fill_solid(leds, LED_TOTAL_COUNT, color);
    } else {
        fill_solid(leds, LED_TOTAL_COUNT, CRGB::Black);
    }
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
    // Handle emergency state with flashing
    if (currentState == EMERGENCY) {
        static unsigned long lastFlash = 0;
        static bool flashState = false;
        if (millis() - lastFlash > 250) {  // Flash every 250ms
            flashState = !flashState;
            flashAllStrips(CRGB::Red, flashState);
            lastFlash = millis();
        }
        return;
    }

    // Update scroll position for animations
    if (millis() - lastScrollUpdate > SCROLL_INTERVAL) {
        scrollPosition++;
        if (scrollPosition >= LED_PER_STRIP) scrollPosition = 0;
        lastScrollUpdate = millis();
    }

    if (testPatternActive) {
        // Rainbow test pattern
        static uint8_t hue = 0;
        for (int i = 0; i < LED_TOTAL_COUNT; i++) {
            leds[i] = CHSV(hue + (i * 8), 255, ledBrightness);
        }
        hue++;
    } else if (autoTestActive) {
        // Show per-pump feedback with scrolling during automated test
        for (int strip = 0; strip < 4; strip++) {
            CRGB color = pumpColors[strip];
            if (strip == currentPump && currentState == RUNNING) {
                // Active pump - scrolling LEDs in direction of movement
                scrollStripLEDs(strip, color, pumpDirection);
            } else {
                // Inactive pumps - very dim static (10%)
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
            case EMERGENCY:
                // Handled above
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
            testPhase = 0;
            lastPumpChange = millis();
            Serial.println("\n=== AUTOMATED PUMP TEST STARTED ===");
            Serial.println("Phase 1: Testing all pumps FORWARD with scrolling LEDs\n");
            updateLCD("AUTO TEST", "Phase 1: FWD");
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
        // Move to next phase
        testPhase++;
        if (testPhase == 1) {
            Serial.println("\n=== PHASE 2: REVERSE MOVEMENT ===");
            Serial.println("Testing all pumps REVERSE with scrolling LEDs\n");
            updateLCD("Phase 2: REV", "Starting...");
            delay(1500);
        } else if (testPhase == 2) {
            Serial.println("\n=== PHASE 3: EMERGENCY STOP TEST ===");
            Serial.println("Testing emergency stop with LED feedback\n");
            updateLCD("Phase 3: E-Stop", "Starting...");
            delay(1500);
        } else if (testPhase > 2) {
            // All tests complete - restart cycle
            Serial.println("\n✓ All 3 phases complete - restarting cycle\n");
            updateLCD("All Tests Done", "Restarting...");
            delay(2000);
            testPhase = 0;
        }
        currentPump = 0;
        pump = 0;
    }

    char line1[17], line2[17];

    if (testPhase == 0) {
        // Forward movement test
        pumpDirection = true;
        Serial.print("Testing Pump ");
        Serial.print(pumpNames[pump]);
        Serial.print(" FORWARD (LED: ");
        Serial.print(colorNames[pump]);
        Serial.println(" scrolling →)");

        snprintf(line1, sizeof(line1), "P%s FWD (%d/4)", pumpNames[pump], pump + 1);
        snprintf(line2, sizeof(line2), "%s scroll ->", colorNames[pump]);
        updateLCD(line1, line2);

        // Reset position and move pump forward
        char cmd[32];
        snprintf(cmd, sizeof(cmd), "G92 %c0", pumpNames[pump][0]);
        sendCommand(cmd);
        delay(100);
        snprintf(cmd, sizeof(cmd), "G1 %c10 F150", pumpNames[pump][0]);
        sendCommand(cmd);

    } else if (testPhase == 1) {
        // Reverse movement test
        pumpDirection = false;
        Serial.print("Testing Pump ");
        Serial.print(pumpNames[pump]);
        Serial.print(" REVERSE (LED: ");
        Serial.print(colorNames[pump]);
        Serial.println(" scrolling ←)");

        snprintf(line1, sizeof(line1), "P%s REV (%d/4)", pumpNames[pump], pump + 1);
        snprintf(line2, sizeof(line2), "%s scroll <-", colorNames[pump]);
        updateLCD(line1, line2);

        // Move pump in reverse
        char cmd[32];
        snprintf(cmd, sizeof(cmd), "G1 %c-5 F150", pumpNames[pump][0]);
        sendCommand(cmd);

    } else if (testPhase == 2) {
        // Emergency stop test
        if (pump == 0) {
            Serial.println("\n=== EMERGENCY STOP TEST ===");
            Serial.println("Testing emergency stop with LED feedback...");
            updateLCD("EMERGENCY TEST", "Starting pump...");

            // Start a pump movement
            char cmd[32];
            snprintf(cmd, sizeof(cmd), "G92 %c0", pumpNames[pump][0]);
            sendCommand(cmd);
            delay(100);
            snprintf(cmd, sizeof(cmd), "G1 %c20 F100", pumpNames[pump][0]);
            sendCommand(cmd);
            delay(500);

            // Trigger emergency stop after brief delay
            Serial.println("⚠ TRIGGERING EMERGENCY STOP!");
            updateLCD("⚠ EMERGENCY!", "Stop triggered");
            sendCommand("!");
            currentState = EMERGENCY;
            delay(2000);

            // Resume
            Serial.println("Resuming from emergency stop...");
            updateLCD("Resuming", "from E-Stop");
            sendCommand("~");
            delay(1000);
            currentState = IDLE;
            updateLCD("E-Stop Test", "Complete!");
            delay(1500);
        }
    }

    waitingForIdle = true;
    if (currentState != EMERGENCY) {
        currentState = RUNNING;
    }
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
    Serial.println("  Green        = System IDLE");
    Serial.println("  Blue         = System RUNNING");
    Serial.println("  Red (solid)  = ERROR state");
    Serial.println("  Red (flash)  = EMERGENCY STOP");
    Serial.println("\nLED Scrolling Effects (during automated test):");
    Serial.println("  Cyan scroll    = Pump X active (→ fwd, ← rev)");
    Serial.println("  Magenta scroll = Pump Y active (→ fwd, ← rev)");
    Serial.println("  Yellow scroll  = Pump Z active (→ fwd, ← rev)");
    Serial.println("  White scroll   = Pump A active (→ fwd, ← rev)\n");

    Serial.println("Automated Test Sequence:");
    Serial.println("  Phase 1: All pumps forward with LED scrolling →");
    Serial.println("  Phase 2: All pumps reverse with LED scrolling ←");
    Serial.println("  Phase 3: Emergency stop test with LED feedback\n");

    Serial.println("Controls:");
    Serial.println("  ENCODER rotate  - Adjust brightness (0-255)");
    Serial.println("  ENCODER button  - Start/stop automated pump test");
    Serial.println("\nCommands:");
    Serial.println("  a - Start automated test (3-phase cycle)");
    Serial.println("  s - Stop automated test");
    Serial.println("  ! or e - Emergency stop");
    Serial.println("  ~ or r - Resume from emergency stop");
    Serial.println("  $ - Reset system (Ctrl-X + unlock)");
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
                testPhase = 0;
                lastPumpChange = millis();
                Serial.println("\n=== AUTOMATED PUMP TEST STARTED ===");
                Serial.println("Phase 1: Testing all pumps FORWARD with scrolling LEDs\n");
                updateLCD("AUTO TEST", "Phase 1: FWD");
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
        } else if (input == "!" || input == "e") {
            Serial.println("\n⚠ EMERGENCY STOP!");
            sendCommand("!");
            currentState = EMERGENCY;
            autoTestActive = false;
            Serial.println("Pump stopped (HOLD state)");
            Serial.println("Type '~' to resume or '$' to reset");
            updateLCD("⚠ EMERGENCY!", "Stopped");
        } else if (input == "~" || input == "r") {
            if (currentState == EMERGENCY) {
                Serial.println("\nResuming from EMERGENCY STOP...");
                sendCommand("~");
                currentState = IDLE;
                Serial.println("System resumed");
                updateLCD("Resumed", "System IDLE");
                delay(1000);
                updateLCD("System IDLE", "Press to start");
            }
        } else if (input == "$") {
            Serial.println("\nResetting system...");
            UartSerial.write(0x18);  // Ctrl-X soft reset
            UartSerial.flush();
            delay(100);
            sendCommand("$X");  // Unlock
            currentState = IDLE;
            autoTestActive = false;
            Serial.println("System reset and unlocked");
            updateLCD("System Reset", "Unlocked");
            delay(1000);
            updateLCD("System IDLE", "Press to start");
        } else if (input == "x" || input == "y" || input == "z" || input == "a") {
            // Manual pump test
            autoTestActive = false;
            testPatternActive = false;
            int pump = (input == "x") ? 0 : (input == "y") ? 1 : (input == "z") ? 2 : 3;
            currentPump = pump;
            testPhase = 0;
            pumpDirection = true;
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
