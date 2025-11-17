/**
 * Test 18: Data Logging and Monitoring
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - Direct UART connection (GPIO 16/17)
 *
 * Purpose:
 * - Test data logging capabilities
 * - Record dispensing operations
 * - Monitor system performance
 * - Generate operation reports
 *
 * Logged Data:
 * - Timestamp
 * - Command sent
 * - Response received
 * - Duration
 * - Success/failure status
 *
 * Features:
 * - Intelligent logging: Only logs actual commands, not status queries
 * - Verbose mode: Toggle to see all status updates
 * - Rate limiting: Status queries every 5 seconds (not logged unless verbose)
 * - Response timeout: 2 second timeout for command responses
 * - Circular buffer: Stores last 50 log entries
 * - Statistics: Command success rate, uptime, memory usage
 *
 * Build command:
 *   pio run -e test_18_data_logging -t upload -t monitor
 */

#include <Arduino.h>
#include "pin_definitions.h"

#define UartSerial         Serial2

struct LogEntry {
    unsigned long timestamp;
    String command;
    String response;
    unsigned long duration;
    bool success;
};

#define MAX_LOG_ENTRIES 50
LogEntry logBuffer[MAX_LOG_ENTRIES];
int logIndex = 0;
int totalCommands = 0;
int successfulCommands = 0;
int failedCommands = 0;

unsigned long commandStartTime = 0;
String lastCommand = "";
bool waitingForResponse = false;
bool verboseLogging = false;  // Only log actual commands, not status updates
unsigned long lastStatusQuery = 0;
const unsigned long STATUS_QUERY_INTERVAL = 5000;  // Query status every 5 seconds

void logCommand(const char* cmd, bool isStatusQuery = false) {
    lastCommand = String(cmd);
    commandStartTime = millis();
    waitingForResponse = true;

    // Only print non-status queries or when verbose logging is enabled
    if (!isStatusQuery || verboseLogging) {
        Serial.print("[");
        Serial.print(millis());
        Serial.print("] → ");
        Serial.println(cmd);
    }

    UartSerial.println(cmd);
    UartSerial.flush();
}

void logResponse(String response, bool isStatusResponse = false) {
    if (!waitingForResponse) return;

    unsigned long duration = millis() - commandStartTime;
    bool success = (response.indexOf("ok") >= 0) || (response.indexOf("Idle") >= 0) ||
                   (response.indexOf("Run") >= 0) || (response.indexOf("Jog") >= 0);

    // Only log actual commands, not status queries (unless verbose mode)
    bool shouldLog = !isStatusResponse || verboseLogging;

    if (shouldLog) {
        // Add to log buffer
        LogEntry entry;
        entry.timestamp = commandStartTime;
        entry.command = lastCommand;
        entry.response = response;
        entry.duration = duration;
        entry.success = success;

        logBuffer[logIndex % MAX_LOG_ENTRIES] = entry;
        logIndex++;

        totalCommands++;
        if (success) {
            successfulCommands++;
        } else {
            failedCommands++;
        }

        Serial.print("[");
        Serial.print(millis());
        Serial.print("] ← ");
        Serial.print(response);
        Serial.print(" (");
        Serial.print(duration);
        Serial.print("ms) ");
        Serial.println(success ? "✓" : "✗");
    }

    waitingForResponse = false;
}

void printStatistics() {
    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║                    Operation Statistics                    ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.print("Total commands:      ");
    Serial.println(totalCommands);
    Serial.print("Successful:          ");
    Serial.print(successfulCommands);
    Serial.print(" (");
    Serial.print(totalCommands > 0 ? (successfulCommands * 100.0 / totalCommands) : 0, 1);
    Serial.println("%)");
    Serial.print("Failed:              ");
    Serial.print(failedCommands);
    Serial.print(" (");
    Serial.print(totalCommands > 0 ? (failedCommands * 100.0 / totalCommands) : 0, 1);
    Serial.println("%)");
    Serial.print("Uptime:              ");
    Serial.print(millis() / 1000);
    Serial.println(" seconds");
    Serial.print("Free heap:           ");
    Serial.print(ESP.getFreeHeap() / 1024.0, 1);
    Serial.println(" KB");
    Serial.println();
}

void printLog(int count) {
    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║                      Command Log                           ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");

    int startIdx = max(0, logIndex - count);
    int entries = min(count, logIndex);

    for (int i = startIdx; i < startIdx + entries; i++) {
        LogEntry entry = logBuffer[i % MAX_LOG_ENTRIES];
        Serial.print("[");
        Serial.print(entry.timestamp / 1000);
        Serial.print("s] ");
        Serial.print(entry.command);
        Serial.print(" → ");
        Serial.print(entry.response.substring(0, 30));
        Serial.print(" (");
        Serial.print(entry.duration);
        Serial.print("ms) ");
        Serial.println(entry.success ? "✓" : "✗");
    }
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║          Test 18: Data Logging and Monitoring             ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");

    // Initialize UART
    UartSerial.begin(115200, SERIAL_8N1, UART_TEST_RX_PIN, UART_TEST_TX_PIN);
    Serial.println("✓ UART initialized");
    Serial.println("✓ Logging system active\n");

    Serial.println("Commands:");
    Serial.println("  x <gcode> - Execute G-code (logged)");
    Serial.println("  ! or e - EMERGENCY STOP (stop all pumps immediately)");
    Serial.println("  ~ or r - Resume from HOLD (after emergency stop)");
    Serial.println("  $ - Reset system (Ctrl-X + unlock)");
    Serial.println("  l [count] - Show log (default: 10 entries)");
    Serial.println("  s - Show statistics");
    Serial.println("  c - Clear log");
    Serial.println("  v - Toggle verbose logging (status updates)");
    Serial.println("  ? - Query status");
    Serial.println("\nExamples:");
    Serial.println("  x G92 X0");
    Serial.println("  x G1 X10 F150");
    Serial.println("  l 20\n");

    Serial.print("Verbose logging: ");
    Serial.println(verboseLogging ? "ON" : "OFF");
    Serial.println("(Status queries are sent every 5s but not logged unless verbose)\n");

    delay(1000);
    logCommand("?", true);
}

void loop() {
    // Handle user commands
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        if (input.startsWith("x ")) {
            String cmd = input.substring(2);
            logCommand(cmd.c_str());
        } else if (input == "!" || input == "e") {
            Serial.println("\n⚠ EMERGENCY STOP!");
            logCommand("!");
            Serial.println("All pumps stopped (HOLD state)");
            Serial.println("Type '~' to resume or '$' to reset");
        } else if (input == "~" || input == "r") {
            Serial.println("\nResuming from HOLD...");
            logCommand("~");
            Serial.println("System resumed");
        } else if (input == "$") {
            Serial.println("\nResetting system...");
            UartSerial.write(0x18);  // Ctrl-X soft reset
            UartSerial.flush();
            delay(100);
            logCommand("$X");
            Serial.println("System reset and unlocked");
        } else if (input.startsWith("l")) {
            int count = 10;
            if (input.length() > 2) {
                count = input.substring(2).toInt();
            }
            printLog(count);
        } else if (input == "s") {
            printStatistics();
        } else if (input == "c") {
            logIndex = 0;
            totalCommands = 0;
            successfulCommands = 0;
            failedCommands = 0;
            Serial.println("Log cleared");
        } else if (input == "v") {
            verboseLogging = !verboseLogging;
            Serial.print("Verbose logging: ");
            Serial.println(verboseLogging ? "ON" : "OFF");
        } else if (input == "?") {
            logCommand("?", true);
        }
    }

    // Periodic status query (rate limited)
    if (millis() - lastStatusQuery >= STATUS_QUERY_INTERVAL) {
        if (!waitingForResponse) {  // Only query if not waiting for another response
            logCommand("?", true);
            lastStatusQuery = millis();
        }
    }

    // Timeout for waiting responses (2 seconds)
    if (waitingForResponse && (millis() - commandStartTime > 2000)) {
        if (verboseLogging) {
            Serial.println("[TIMEOUT] No response received");
        }
        waitingForResponse = false;
    }

    // Process responses
    if (UartSerial.available()) {
        String response = UartSerial.readStringUntil('\n');
        response.trim();
        if (response.length() > 0) {
            // Determine if this is a status response (contains machine state)
            bool isStatus = (response.indexOf("<Idle") >= 0) ||
                           (response.indexOf("<Run") >= 0) ||
                           (response.indexOf("<Jog") >= 0) ||
                           (response.indexOf("<Hold") >= 0) ||
                           (response.indexOf("<Alarm") >= 0);

            logResponse(response, isStatus);
        }
    }

    delay(10);
}
