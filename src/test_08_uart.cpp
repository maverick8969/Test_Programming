/**
 * Test 08: Direct UART Communication with BTT Rodent Board
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (configured for UART, not RS485)
 * - ESP32 Dev Module (standard ESP32 development board)
 * - Direct UART connection (no RS485 transceivers)
 * - TX pin: GPIO 17 (D17)
 * - RX pin: GPIO 16 (D16)
 * - GND: Common ground between ESP32 and Rodent
 *
 * UART Wiring (Direct Connection - No RS485):
 * ESP32 Dev Module Side:
 * - ESP32 TX (D17 = GPIO 17) → BTT Rodent RX (GPIO 14)
 * - ESP32 RX (D16 = GPIO 16) ← BTT Rodent TX (GPIO 15)
 * - ESP32 GND → BTT Rodent GND (CRITICAL!)
 *
 * IMPORTANT:
 * - This test uses direct UART without RS485 transceivers
 * - Both boards must share a common ground
 * - Cable length should be very short (< 1 meter) for reliable communication
 * - For longer distances, use RS485 (see test_07_rs485.cpp)
 * - The BTT Rodent must be configured for UART mode (use btt_rodent_uart.yaml)
 *
 * BTT Rodent FluidNC Configuration:
 * - Upload btt_rodent_uart.yaml to the Rodent board
 * - FluidNC uses custom uart1 configuration with GPIO 15 (TX) and GPIO 14 (RX)
 * - Baud rate: 115200
 * - Message level: Verbose (for detailed debugging output)
 *
 * FluidNC Commands:
 * - $I = System info
 * - ? = Status query
 * - $$ = List all settings
 * - $H = Home all axes
 * - G0 X10 = Rapid move to X=10mm
 * - ! = Feed hold
 * - ~ = Resume
 * - Ctrl-X = Reset
 *
 * Build command:
 *   pio run -e test_08_uart -t upload -t monitor
 */

#include <Arduino.h>
#include "pin_definitions.h"

// Serial port configuration
// FluidNC typically uses 115200 baud
#define UART_TEST_BAUD     115200
#define UART_TEST_CONFIG   SERIAL_8N1

// Use Serial2 for UART communication (ESP32 Dev Module has Serial0/Serial1/Serial2)
#define UartSerial         Serial2

// Buffer for incoming data
#define RX_BUFFER_SIZE     512
char rxBuffer[RX_BUFFER_SIZE];
uint16_t rxIndex = 0;

// Statistics
unsigned long lastDataTime = 0;
unsigned long totalBytesSent = 0;
unsigned long totalBytesReceived = 0;
unsigned long commandCount = 0;

// Test state
enum TestPhase {
    PHASE_INIT,
    PHASE_INFO,
    PHASE_STATUS,
    PHASE_INTERACTIVE
};
TestPhase currentPhase = PHASE_INIT;
unsigned long phaseStartTime = 0;

/**
 * Send command to Rodent board
 */
void sendCommand(const char* cmd) {
    commandCount++;

    Serial.print("\n→ Sending: \"");
    Serial.print(cmd);
    Serial.println("\"");

    UartSerial.print(cmd);
    UartSerial.print("\n");
    UartSerial.flush();  // Ensure all data is sent
    totalBytesSent += strlen(cmd) + 1;

    lastDataTime = millis();
}

/**
 * Send command and wait for response
 */
bool sendCommandAndWait(const char* cmd, unsigned long timeout = 2000) {
    rxIndex = 0;
    sendCommand(cmd);

    unsigned long startTime = millis();
    bool gotResponse = false;

    while (millis() - startTime < timeout) {
        while (UartSerial.available()) {
            char c = UartSerial.read();
            totalBytesReceived++;

            if (c >= 32 || c == '\n' || c == '\r' || c == '\t') {
                Serial.print(c);
            }

            gotResponse = true;
            lastDataTime = millis();
        }
        delay(10);
    }

    if (gotResponse) {
        Serial.println();
    } else {
        Serial.println("⚠️  No response received");
    }

    return gotResponse;
}

/**
 * Process received line
 */
void processLine(const char* line, size_t len) {
    if (len == 0) return;

    Serial.print("← ");
    for (size_t i = 0; i < len; i++) {
        if (line[i] >= 32 || line[i] == '\t') {
            Serial.print(line[i]);
        }
    }
    Serial.println();
}

/**
 * Print menu
 */
void printMenu() {
    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║                     Interactive Menu                       ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("System Commands:");
    Serial.println("  i  - Get system info ($I)");
    Serial.println("  ?  - Get status query");
    Serial.println("  s  - List all settings ($$)");
    Serial.println("  r  - Reset controller (Ctrl-X)");
    Serial.println();
    Serial.println("Motion Commands:");
    Serial.println("  h  - Home all axes ($H)");
    Serial.println("  0  - Move X to 0 (G0 X0)");
    Serial.println("  1  - Move X to 10mm (G0 X10)");
    Serial.println("  2  - Move X to -10mm (G0 X-10)");
    Serial.println();
    Serial.println("Control:");
    Serial.println("  !  - Feed hold (pause)");
    Serial.println("  ~  - Resume");
    Serial.println("  m  - Show this menu");
    Serial.println();
    Serial.println("Or type any G-code command directly");
    Serial.println("============================================================\n");
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║   Test 08: UART Communication (ESP32 Dev ↔ Rodent)        ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");

    // Hardware configuration
    Serial.println("\n[Hardware Configuration]");
    Serial.println("Board:            ESP32 Dev Module");
    Serial.print("TX Pin:           GPIO "); Serial.print(UART_TEST_TX_PIN); Serial.println(" (D17)");
    Serial.print("RX Pin:           GPIO "); Serial.print(UART_TEST_RX_PIN); Serial.println(" (D16)");
    Serial.print("Baud Rate:        "); Serial.println(UART_TEST_BAUD);
    Serial.print("Data Format:      ");
    if (UART_TEST_CONFIG == SERIAL_8N1) Serial.println("8N1");

    Serial.println("\n[UART WIRING - Direct Connection (No RS485)]");
    Serial.println("ESP32 Dev Module Side:");
    Serial.print("  ESP32 TX (D17 = GPIO "); Serial.print(UART_TEST_TX_PIN);
    Serial.println(") → Rodent RX (GPIO 14)");
    Serial.print("  ESP32 RX (D16 = GPIO "); Serial.print(UART_TEST_RX_PIN);
    Serial.println(") ← Rodent TX (GPIO 15)");
    Serial.println("  ESP32 GND → Rodent GND (CRITICAL!)");
    Serial.println();
    Serial.println("IMPORTANT NOTES:");
    Serial.println("  ✓ No RS485 transceivers needed");
    Serial.println("  ✓ Direct TX→RX, RX→TX connection (crossover)");
    Serial.println("  ✓ Common ground is CRITICAL");
    Serial.println("  ✓ Keep cable length < 1 meter for reliability");
    Serial.println("  ✓ For longer distances, use RS485 (test_07)");
    Serial.println();
    Serial.println("BTT Rodent Configuration:");
    Serial.println("  GPIO 15 (TX) → ESP32 RX (D16)");
    Serial.println("  GPIO 14 (RX) ← ESP32 TX (D17)");
    Serial.println("  Uses uart1 and uart_channel1 configuration");
    Serial.println("  Message level: Verbose (detailed output)");
    Serial.println("  Must upload btt_rodent_uart.yaml to Rodent!");

    // Initialize serial port
    Serial.println("\n[Initializing UART]");
    UartSerial.begin(UART_TEST_BAUD, UART_TEST_CONFIG, UART_TEST_RX_PIN, UART_TEST_TX_PIN);
    UartSerial.setRxBufferSize(512);
    delay(100);
    Serial.println("✓ UART port initialized");

    Serial.println("\n[Starting Communication Test]");
    Serial.println("Attempting to communicate with BTT Rodent...\n");

    phaseStartTime = millis();
    currentPhase = PHASE_INFO;
}

void loop() {
    // Run test phases
    switch (currentPhase) {
        case PHASE_INIT:
            // Already done in setup
            currentPhase = PHASE_INFO;
            phaseStartTime = millis();
            break;

        case PHASE_INFO:
            Serial.println("\n[Phase 1: Getting System Info]");
            if (sendCommandAndWait("$I", 3000)) {
                Serial.println("✓ System info received");
            }
            delay(1000);

            currentPhase = PHASE_STATUS;
            phaseStartTime = millis();
            break;

        case PHASE_STATUS:
            Serial.println("\n[Phase 2: Getting Status]");
            if (sendCommandAndWait("?", 2000)) {
                Serial.println("✓ Status received");
            }
            delay(1000);

            Serial.println("\n[Phase 3: Communication Test Complete!]");
            Serial.println("✓ UART communication is working");
            Serial.println();
            Serial.print("Commands sent: "); Serial.println(commandCount);
            Serial.print("Bytes sent: "); Serial.println(totalBytesSent);
            Serial.print("Bytes received: "); Serial.println(totalBytesReceived);

            currentPhase = PHASE_INTERACTIVE;
            phaseStartTime = millis();
            printMenu();
            break;

        case PHASE_INTERACTIVE:
            // Handle user input from Serial monitor
            if (Serial.available()) {
                String input = Serial.readStringUntil('\n');
                input.trim();

                if (input.length() > 0) {
                    char cmd = input.charAt(0);

                    switch (cmd) {
                        case 'i':
                            sendCommandAndWait("$I", 2000);
                            break;
                        case '?':
                            sendCommandAndWait("?", 1000);
                            break;
                        case 's':
                            Serial.println("Listing all settings (this may take a moment)...");
                            sendCommandAndWait("$$", 5000);
                            break;
                        case 'r':
                            Serial.println("Sending reset (Ctrl-X)...");
                            UartSerial.write(0x18);  // Ctrl-X
                            UartSerial.flush();
                            delay(2000);
                            break;
                        case 'h':
                            Serial.println("Homing all axes...");
                            sendCommandAndWait("$H", 10000);
                            break;
                        case '0':
                            sendCommandAndWait("G0 X0", 2000);
                            break;
                        case '1':
                            sendCommandAndWait("G0 X10", 2000);
                            break;
                        case '2':
                            sendCommandAndWait("G0 X-10", 2000);
                            break;
                        case '!':
                            UartSerial.write('!');
                            UartSerial.flush();
                            Serial.println("→ Feed hold sent");
                            break;
                        case '~':
                            UartSerial.write('~');
                            UartSerial.flush();
                            Serial.println("→ Resume sent");
                            break;
                        case 'm':
                            printMenu();
                            break;
                        default:
                            // Send as raw G-code
                            sendCommandAndWait(input.c_str(), 3000);
                            break;
                    }
                }
            }

            // Check for unsolicited data from Rodent
            if (UartSerial.available()) {
                while (UartSerial.available()) {
                    char c = UartSerial.read();
                    totalBytesReceived++;

                    if (rxIndex < RX_BUFFER_SIZE - 1) {
                        rxBuffer[rxIndex++] = c;
                    }

                    if (c == '\n') {
                        rxBuffer[rxIndex] = '\0';
                        processLine(rxBuffer, rxIndex);
                        rxIndex = 0;
                    }

                    lastDataTime = millis();
                }
            }
            break;
    }

    // Memory monitoring every 30 seconds
    static unsigned long lastMemCheck = 0;
    if (millis() - lastMemCheck >= 30000 && currentPhase == PHASE_INTERACTIVE) {
        Serial.print("\n[Memory] Free heap: ");
        Serial.print(ESP.getFreeHeap() / 1024.0, 1);
        Serial.print(" KB | Commands: ");
        Serial.print(commandCount);
        Serial.print(" | RX: ");
        Serial.print(totalBytesReceived);
        Serial.println(" bytes");
        lastMemCheck = millis();
    }

    delay(10);
}
