/**
 * Test 11: UART Communication with LED Status Indicators + Encoder Control
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - 32 WS2812B LEDs (4 strips × 8 LEDs)
 * - Rotary encoder with button
 * - Direct UART connection (GPIO 16/17)
 *
 * Purpose:
 * - Test UART communication with visual LED feedback
 * - Show pump activity with LED colors
 * - Indicate system status visually
 * - Use encoder to control LED brightness
 *
 * LED Indicators:
 * - Strip 0 (Pump X): Cyan when active
 * - Strip 1 (Pump Y): Magenta when active
 * - Strip 2 (Pump Z): Yellow when active
 * - Strip 3 (Pump A): White when active
 * - All: Green = Idle, Red = Error, Blue = Running
 *
 * Encoder Controls:
 * - Rotate: Adjust LED brightness (0-255)
 * - Press: Toggle LED test pattern
 *
 * Build command:
 *   pio run -e test_11_uart_leds -t upload -t monitor
 */

#include <Arduino.h>
#include <FastLED.h>
#include "pin_definitions.h"

#define UartSerial         Serial2

CRGB leds[LED_TOTAL_COUNT];
enum SystemState { IDLE, RUNNING, ERROR };
SystemState currentState = IDLE;

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
    } else {
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
    }

    // Check encoder button press - toggle test pattern
    if (readEncoderButton() && encButton.pressed) {
        testPatternActive = !testPatternActive;
        Serial.print("Encoder: Test pattern ");
        Serial.println(testPatternActive ? "ON" : "OFF");
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║      Test 11: UART Communication + LED + Encoder          ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");

    // Initialize LEDs
    FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, LED_TOTAL_COUNT);
    FastLED.setBrightness(ledBrightness);
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
    Serial.println("  Green  = System IDLE");
    Serial.println("  Blue   = System RUNNING");
    Serial.println("  Red    = ERROR state");
    Serial.println("  Cyan   = Pump X active");
    Serial.println("  Magenta= Pump Y active");
    Serial.println("  Yellow = Pump Z active");
    Serial.println("  White  = Pump A active\n");

    Serial.println("Controls:");
    Serial.println("  ENCODER rotate  - Adjust brightness (0-255)");
    Serial.println("  ENCODER button  - Toggle test pattern\n");

    delay(1000);
    sendCommand("?");
}

void loop() {
    // Handle encoder input
    handleEncoder();

    // Process received data
    if (UartSerial.available()) {
        String response = UartSerial.readStringUntil('\n');
        response.trim();

        if (response.length() > 0) {
            Serial.print("← ");
            Serial.println(response);

            // Parse state and update LEDs
            if (response.indexOf("Idle") >= 0) {
                currentState = IDLE;
            } else if (response.indexOf("Run") >= 0) {
                currentState = RUNNING;
            } else if (response.indexOf("error") >= 0 || response.indexOf("ALARM") >= 0) {
                currentState = ERROR;
            }
        }
    }

    // Update LEDs
    updateLEDs();
    FastLED.show();

    // Periodically query status
    static unsigned long lastQuery = 0;
    if (millis() - lastQuery > 1000) {
        sendCommand("?");
        lastQuery = millis();
    }

    delay(10);
}
