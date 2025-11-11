/**
 * Test 06: Digital Scale via RS232
 *
 * Hardware:
 * - Digital scale with RS232 output
 * - MAX3232 or similar RS232 to TTL level converter (required!)
 * - RX pin: GPIO 35 (input-only, perfect for RX)
 * - TX pin: GPIO 32
 *
 * WARNING: RS232 uses ±12V logic levels. You MUST use a level converter
 * (MAX3232, SP3232, etc.) to convert to 3.3V TTL. Direct connection
 * will damage the ESP32!
 *
 * Common Scale Settings:
 * - Baud rate: 9600 (most common), 4800, 2400, or 19200
 * - Data bits: 7 or 8
 * - Parity: None, Even, or Odd
 * - Stop bits: 1 or 2
 *
 * This test will:
 * 1. Read raw data from the scale
 * 2. Display data in both HEX and ASCII
 * 3. Attempt to parse common weight formats
 * 4. Show continuous readings
 *
 * Build command:
 *   pio run -e test_06_scale -t upload -t monitor
 */

#include <Arduino.h>
#include "pin_definitions.h"

// Serial port configuration
// Start with most common settings: 9600 8N1
#define SCALE_BAUD      9600
#define SCALE_CONFIG    SERIAL_8N1

// Use Serial2 for the scale (ESP32 has 3 hardware serial ports)
#define ScaleSerial     Serial2

// Buffer for incoming data
#define RX_BUFFER_SIZE  256
char rxBuffer[RX_BUFFER_SIZE];
uint16_t rxIndex = 0;

// Statistics
unsigned long lastDataTime = 0;
unsigned long totalBytes = 0;
unsigned long totalLines = 0;

/**
 * Print data in HEX format for debugging
 */
void printHex(const char* data, size_t len) {
    Serial.print("HEX: ");
    for (size_t i = 0; i < len; i++) {
        if (data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

/**
 * Print data in ASCII with non-printable chars shown as dots
 */
void printASCII(const char* data, size_t len) {
    Serial.print("ASCII: \"");
    for (size_t i = 0; i < len; i++) {
        if (data[i] >= 32 && data[i] <= 126) {
            Serial.print(data[i]);
        } else if (data[i] == '\r') {
            Serial.print("\\r");
        } else if (data[i] == '\n') {
            Serial.print("\\n");
        } else if (data[i] == '\t') {
            Serial.print("\\t");
        } else {
            Serial.print(".");
        }
    }
    Serial.println("\"");
}

/**
 * Attempt to parse weight from common formats
 */
bool parseWeight(const char* data, size_t len, float* weight, char* unit) {
    String str = String(data);
    str.trim();

    // Pattern 1: "123.45 g" or "123.45g"
    int spaceIdx = str.indexOf(' ');
    if (spaceIdx > 0) {
        String numPart = str.substring(0, spaceIdx);
        String unitPart = str.substring(spaceIdx + 1);
        *weight = numPart.toFloat();
        unitPart.toCharArray(unit, 10);
        return true;
    }

    // Pattern 2: "123.45g" (no space)
    for (size_t i = 0; i < str.length(); i++) {
        if (isAlpha(str[i])) {
            String numPart = str.substring(0, i);
            String unitPart = str.substring(i);
            *weight = numPart.toFloat();
            unitPart.toCharArray(unit, 10);
            return true;
        }
    }

    // Pattern 3: Just a number
    if (str.length() > 0 && (isDigit(str[0]) || str[0] == '-' || str[0] == '.')) {
        *weight = str.toFloat();
        strcpy(unit, "?");
        return true;
    }

    return false;
}

/**
 * Process complete line of data
 */
void processLine(const char* line, size_t len) {
    if (len == 0) return;

    totalLines++;

    Serial.println("\n----------------------------------------");
    Serial.print("Line #"); Serial.print(totalLines);
    Serial.print(" | Length: "); Serial.print(len);
    Serial.print(" bytes | Time: "); Serial.print(millis() / 1000.0, 3);
    Serial.println(" sec");

    // Show raw data
    printHex(line, len);
    printASCII(line, len);

    // Try to parse weight
    float weight;
    char unit[10];
    if (parseWeight(line, len, &weight, unit)) {
        Serial.print("PARSED WEIGHT: ");
        Serial.print(weight, 3);
        Serial.print(" ");
        Serial.println(unit);
    } else {
        Serial.println("(Could not parse as weight value)");
    }

    Serial.println("----------------------------------------");
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║           Test 06: Digital Scale via RS232                ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");

    // Hardware configuration
    Serial.println("\n[Hardware Configuration]");
    Serial.print("RX Pin:           GPIO "); Serial.print(SCALE_RX_PIN);
    Serial.println(" (input-only, perfect for RX)");
    Serial.print("TX Pin:           GPIO "); Serial.println(SCALE_TX_PIN);
    Serial.print("Baud Rate:        "); Serial.println(SCALE_BAUD);
    Serial.print("Data Format:      ");
    if (SCALE_CONFIG == SERIAL_8N1) Serial.println("8N1 (8 data, no parity, 1 stop)");
    else if (SCALE_CONFIG == SERIAL_7E1) Serial.println("7E1 (7 data, even parity, 1 stop)");
    else if (SCALE_CONFIG == SERIAL_7O1) Serial.println("7O1 (7 data, odd parity, 1 stop)");
    else Serial.println("Custom");

    Serial.println("\n[IMPORTANT SAFETY WARNING]");
    Serial.println("⚠️  RS232 uses ±12V logic levels!");
    Serial.println("⚠️  You MUST use a MAX3232 or similar level converter");
    Serial.println("⚠️  Direct connection will DAMAGE the ESP32!");
    Serial.println("⚠️  Ensure converter is wired correctly:");
    Serial.println("     - RS232 RX → MAX3232 R1IN → T1OUT → ESP32 RX (GPIO 35)");
    Serial.println("     - RS232 TX → MAX3232 T1IN ← R1OUT ← ESP32 TX (GPIO 32)");
    Serial.println("     - MAX3232 VCC = 3.3V, GND = GND");

    // Initialize scale serial port
    Serial.println("\n[Initializing Scale Serial Port]");
    ScaleSerial.begin(SCALE_BAUD, SCALE_CONFIG, SCALE_RX_PIN, SCALE_TX_PIN);
    ScaleSerial.setRxBufferSize(512);  // Increase buffer size
    delay(100);
    Serial.println("✓ Serial port initialized");

    // Test sending commands (some scales respond to commands)
    Serial.println("\n[Sending Test Commands]");
    Serial.println("Trying common scale commands...");
    ScaleSerial.println("P");     // Print (common command)
    delay(100);
    ScaleSerial.println("W");     // Weight
    delay(100);
    ScaleSerial.write(0x05);      // ENQ (Enquiry)
    delay(100);

    Serial.println("\n[Listening for Scale Data]");
    Serial.println("Waiting for data from scale...");
    Serial.println("(If no data appears, check wiring and baud rate)");
    Serial.println("(Try placing weight on scale to trigger output)");
    Serial.println();

    rxIndex = 0;
    lastDataTime = millis();
}

void loop() {
    // Check for incoming data
    while (ScaleSerial.available()) {
        char c = ScaleSerial.read();
        totalBytes++;
        lastDataTime = millis();

        // Add to buffer
        if (rxIndex < RX_BUFFER_SIZE - 1) {
            rxBuffer[rxIndex++] = c;
        }

        // Check for line ending (CR, LF, or both)
        if (c == '\n' || c == '\r') {
            // Process the line
            rxBuffer[rxIndex] = '\0';
            processLine(rxBuffer, rxIndex);
            rxIndex = 0;

            // Skip additional line ending chars
            while (ScaleSerial.available()) {
                char peek = ScaleSerial.peek();
                if (peek == '\n' || peek == '\r') {
                    ScaleSerial.read();
                } else {
                    break;
                }
            }
        }
    }

    // Handle buffer overflow or timeout
    if (rxIndex >= RX_BUFFER_SIZE - 1) {
        Serial.println("\n⚠️  Buffer overflow! Processing partial data...");
        rxBuffer[rxIndex] = '\0';
        processLine(rxBuffer, rxIndex);
        rxIndex = 0;
    }

    // If we have data but no line ending after 1 second, process it
    if (rxIndex > 0 && (millis() - lastDataTime) > 1000) {
        Serial.println("\n⚠️  Timeout (no line ending). Processing partial data...");
        rxBuffer[rxIndex] = '\0';
        processLine(rxBuffer, rxIndex);
        rxIndex = 0;
    }

    // Status update every 10 seconds if no data
    static unsigned long lastStatusTime = 0;
    if (millis() - lastDataTime > 10000 && millis() - lastStatusTime > 10000) {
        Serial.print("\n[Status] Waiting for data... ");
        Serial.print("Total bytes: "); Serial.print(totalBytes);
        Serial.print(" | Lines: "); Serial.print(totalLines);
        Serial.print(" | Uptime: "); Serial.print(millis() / 1000);
        Serial.println(" sec");

        if (totalBytes == 0) {
            Serial.println("💡 Troubleshooting tips:");
            Serial.println("   1. Check MAX3232 wiring and power (3.3V)");
            Serial.println("   2. Verify scale is powered on");
            Serial.println("   3. Try different baud rate (edit SCALE_BAUD)");
            Serial.println("   4. Place weight on scale to trigger output");
            Serial.println("   5. Check scale settings/mode");
        }

        lastStatusTime = millis();
    }

    // Memory monitoring every 30 seconds
    static unsigned long lastMemCheck = 0;
    if (millis() - lastMemCheck >= 30000) {
        Serial.print("\n[Memory] Free heap: ");
        Serial.print(ESP.getFreeHeap() / 1024.0, 1);
        Serial.println(" KB");
        lastMemCheck = millis();
    }

    delay(10);  // Small delay to prevent tight loop
}
