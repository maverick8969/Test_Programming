/**
 * Test 10: UART Communication with LCD Display + Encoder Control
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - 1602 LCD with I2C backpack
 * - Rotary encoder with button
 * - Direct UART connection (GPIO 16/17)
 *
 * Purpose:
 * - Test UART communication with LCD status display
 * - Show real-time status from FluidNC on LCD
 * - Display current position and state
 * - Use encoder to navigate pump control menu
 *
 * LCD Display:
 * - Line 1: Menu selection / Current state
 * - Line 2: Position or command feedback
 *
 * Encoder Controls:
 * - Rotate: Navigate menu (pumps X/Y/Z/A)
 * - Press: Start/stop selected pump
 *
 * Build command:
 *   pio run -e test_10_uart_lcd -t upload -t monitor
 */

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "pin_definitions.h"

#define UartSerial         Serial2

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 16, 2);
String lastResponse = "";
unsigned long lastQueryTime = 0;

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

// Menu state
int currentPump = 0;  // 0=X, 1=Y, 2=Z, 3=A
const char* pumpNames[] = {"X", "Y", "Z", "A"};
bool pumpRunning = false;

void sendCommand(const char* cmd) {
    Serial.print("→ ");
    Serial.println(cmd);
    UartSerial.println(cmd);
    UartSerial.flush();
}

void updateLCD(const char* line1, const char* line2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
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

void updateMenu() {
    char line1[17], line2[17];
    snprintf(line1, sizeof(line1), "Pump: %s %s", pumpNames[currentPump],
             pumpRunning ? "RUN" : "IDLE");
    snprintf(line2, sizeof(line2), "Rotate=Nav Btn=Go");
    updateLCD(line1, line2);
}

void handleEncoder() {
    // Check for rotation
    int direction = readEncoder();
    if (direction != 0) {
        currentPump = ((encoder.position % 4) + 4) % 4;
        Serial.print("Encoder: Selected pump ");
        Serial.println(pumpNames[currentPump]);
        updateMenu();
    }

    // Check encoder button press
    if (readEncoderButton() && encButton.pressed) {
        if (pumpRunning) {
            // Stop pump
            sendCommand("!");
            pumpRunning = false;
            Serial.println("Encoder: STOP");
        } else {
            // Start pump
            char cmd[32];
            snprintf(cmd, sizeof(cmd), "G91 G1 %s10 F200", pumpNames[currentPump]);
            sendCommand(cmd);
            pumpRunning = true;
            Serial.print("Encoder: START pump ");
            Serial.println(pumpNames[currentPump]);
        }
        updateMenu();
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║      Test 10: UART Communication + LCD + Encoder          ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");

    // Initialize I2C LCD
    Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN);
    lcd.init();
    lcd.backlight();
    updateLCD("FluidNC UART", "Connecting...");
    Serial.println("✓ LCD initialized");

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

    Serial.println("Controls:");
    Serial.println("  ENCODER rotate  - Select pump (X/Y/Z/A)");
    Serial.println("  ENCODER button  - Start/stop pump\n");

    delay(1000);
    updateMenu();
    sendCommand("?"); // Query status
}

void loop() {
    // Handle encoder input
    handleEncoder();

    // Query status every 2 seconds
    if (millis() - lastQueryTime > 2000) {
        sendCommand("?");
        lastQueryTime = millis();
    }

    // Process received data
    if (UartSerial.available()) {
        String response = UartSerial.readStringUntil('\n');
        response.trim();

        if (response.length() > 0) {
            Serial.print("← ");
            Serial.println(response);

            // Check if pump finished
            if (response.indexOf("Idle") >= 0 && pumpRunning) {
                pumpRunning = false;
                updateMenu();
            }
        }
    }

    delay(1);
}
