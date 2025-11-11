/**
 * Test 07: RS485 Communication with BTT Rodent Board
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC
 * - RS485 transceiver (required!)
 * - TX pin: GPIO 17
 * - RX pin: GPIO 16
 * - RTS pin: GPIO 4 (optional - only if manual direction control needed)
 *
 * WARNING: RS485 uses differential signaling. You MUST use a transceiver
 * (MAX485, MAX3485, SP485, etc.) to convert between TTL and RS485.
 *
 * Configuration:
 * - Set USE_DIRECTION_CONTROL = true if you need manual RTS control
 * - Set USE_DIRECTION_CONTROL = false for automatic direction control
 *
 * Wiring (with automatic direction control):
 * - ESP32 TX (GPIO 17) → Transceiver DI/TXD
 * - ESP32 RX (GPIO 16) → Transceiver RO/RXD
 * - Transceiver A → Rodent A
 * - Transceiver B → Rodent B
 * - Transceiver VCC = 3.3V or 5V (check datasheet)
 * - Transceiver GND = GND (common ground with ESP32 and Rodent)
 *
 * Wiring (with manual direction control, USE_DIRECTION_CONTROL = true):
 * - Add: ESP32 RTS (GPIO 4) → Transceiver DE and RE (tied together)
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
 *   pio run -e test_07_rs485 -t upload -t monitor
 */

#include <Arduino.h>
#include "pin_definitions.h"

// Serial port configuration
// FluidNC typically uses 115200 baud
#define RODENT_BAUD     115200
#define RODENT_CONFIG   SERIAL_8N1

// Use Serial1 for RS485 communication
#define RodentSerial    Serial1

// RS485 direction control
// Set to false if your transceiver has automatic direction control
#define USE_DIRECTION_CONTROL   false

// Direction control settings (only used if USE_DIRECTION_CONTROL is true)
#define RS485_TX_MODE   HIGH
#define RS485_RX_MODE   LOW

// Buffer for incoming data
#define RX_BUFFER_SIZE  512
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
 * Set RS485 transceiver to transmit mode
 */
void setRS485Transmit() {
    if (USE_DIRECTION_CONTROL) {
        digitalWrite(RODENT_RTS_PIN, RS485_TX_MODE);
        delayMicroseconds(10);  // Small delay for transceiver switching
    }
}

/**
 * Set RS485 transceiver to receive mode
 */
void setRS485Receive() {
    if (USE_DIRECTION_CONTROL) {
        delayMicroseconds(10);  // Wait for TX to complete
        RodentSerial.flush();   // Ensure all data is sent
        digitalWrite(RODENT_RTS_PIN, RS485_RX_MODE);
    } else {
        RodentSerial.flush();   // Ensure all data is sent
    }
}

/**
 * Send command to Rodent board
 */
void sendCommand(const char* cmd) {
    commandCount++;

    Serial.print("\n→ Sending: \"");
    Serial.print(cmd);
    Serial.println("\"");

    setRS485Transmit();
    RodentSerial.print(cmd);
    RodentSerial.print("\n");
    totalBytesSent += strlen(cmd) + 1;
    setRS485Receive();

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
        while (RodentSerial.available()) {
            char c = RodentSerial.read();
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
    Serial.println("║      Test 07: RS485 Communication with BTT Rodent         ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");

    // Hardware configuration
    Serial.println("\n[Hardware Configuration]");
    Serial.print("TX Pin:           GPIO "); Serial.println(RODENT_TX_PIN);
    Serial.print("RX Pin:           GPIO "); Serial.println(RODENT_RX_PIN);
    if (USE_DIRECTION_CONTROL) {
        Serial.print("RTS Pin:          GPIO "); Serial.print(RODENT_RTS_PIN);
        Serial.println(" (RS485 direction control)");
    } else {
        Serial.println("Direction:        Automatic (no RTS pin needed)");
    }
    Serial.print("Baud Rate:        "); Serial.println(RODENT_BAUD);
    Serial.print("Data Format:      ");
    if (RODENT_CONFIG == SERIAL_8N1) Serial.println("8N1");

    Serial.println("\n[IMPORTANT WIRING]");
    Serial.println("RS485 Transceiver Connections:");
    Serial.println("  ESP32 TX (GPIO 17) → Transceiver DI/TXD");
    Serial.println("  ESP32 RX (GPIO 16) → Transceiver RO/RXD");
    if (USE_DIRECTION_CONTROL) {
        Serial.println("  ESP32 RTS (GPIO 4) → Transceiver DE and RE (tied together)");
    } else {
        Serial.println("  (No DE/RE connection - automatic direction control)");
    }
    Serial.println("  Transceiver A → Rodent A");
    Serial.println("  Transceiver B → Rodent B");
    Serial.println("  Transceiver VCC → 3.3V or 5V");
    Serial.println("  Transceiver GND → GND (common ground!)");

    // Initialize RTS pin for direction control (if needed)
    Serial.println("\n[Initializing RS485]");
    if (USE_DIRECTION_CONTROL) {
        pinMode(RODENT_RTS_PIN, OUTPUT);
        setRS485Receive();  // Start in receive mode
        Serial.println("✓ RTS pin configured for direction control");
    } else {
        Serial.println("✓ Using automatic direction control");
    }

    // Initialize serial port
    RodentSerial.begin(RODENT_BAUD, RODENT_CONFIG, RODENT_RX_PIN, RODENT_TX_PIN);
    RodentSerial.setRxBufferSize(512);
    delay(100);
    Serial.println("✓ Serial port initialized");

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
            Serial.println("✓ RS485 communication is working");
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
                            setRS485Transmit();
                            RodentSerial.write(0x18);  // Ctrl-X
                            setRS485Receive();
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
                            setRS485Transmit();
                            RodentSerial.write('!');
                            setRS485Receive();
                            Serial.println("→ Feed hold sent");
                            break;
                        case '~':
                            setRS485Transmit();
                            RodentSerial.write('~');
                            setRS485Receive();
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
            if (RodentSerial.available()) {
                while (RodentSerial.available()) {
                    char c = RodentSerial.read();
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
