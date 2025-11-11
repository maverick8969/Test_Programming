# G-Code Command Reference for BTT Rodent Board Communication

This document explains the G-code command logic used to communicate with the BTT Rodent V1.1 board running FluidNC firmware over RS485.

## Table of Contents
1. [Overview](#overview)
2. [RS485 Communication](#rs485-communication)
3. [System Architecture](#system-architecture)
4. [G-Code Command Reference](#g-code-command-reference)
5. [Pump Control Commands](#pump-control-commands)
6. [Status and Query Commands](#status-and-query-commands)
7. [Configuration Commands](#configuration-commands)
8. [Real-Time Commands](#real-time-commands)
9. [Response Format](#response-format)
10. [Implementation Examples](#implementation-examples)
11. [Flow Rate Calculations](#flow-rate-calculations)
12. [Error Handling](#error-handling)

---

## Overview

The peristaltic pump system uses a BTT Rodent V1.1 board running FluidNC firmware to control 4 stepper motors that drive peristaltic pumps. Communication between the ESP32 controller and the BTT Rodent board is accomplished via **RS485** serial communication using the **G-code** protocol.

### Key Specifications
- **Protocol**: G-code (CNC standard)
- **Communication**: RS485 half-duplex
- **Baud Rate**: 115200
- **Data Format**: 8N1 (8 data bits, No parity, 1 stop bit)
- **Line Terminator**: `\n` (newline)

---

## RS485 Communication

### RS485 Configuration

RS485 is a differential signaling standard that allows long-distance, noise-resistant serial communication.

**Advantages over standard UART:**
- Longer cable distances (up to 1200m at 115200 baud)
- Better noise immunity
- Multi-drop capability (multiple devices on same bus)
- Differential signaling (A+/B-) provides better signal integrity

### Hardware Requirements

**RS485 Transceiver Modules:**
- MAX485, MAX3485, or similar
- One module on ESP32 side
- One module on BTT Rodent side

**Wiring:**
```
ESP32 Side:
  GPIO2  (TX)  -> RS485 Transceiver DI (Data In)
  GPIO4  (RX)  -> RS485 Transceiver RO (Receiver Out)
  GPIO15 (RTS) -> RS485 Transceiver DE/RE (Direction Enable)

BTT Rodent Side:
  GPIO17 (TX)  -> RS485 Transceiver DI (Data In)
  GPIO16 (RX)  -> RS485 Transceiver RO (Receiver Out)
  GPIO0  (RTS) -> RS485 Transceiver DE/RE (Direction Enable)

Bus Connection:
  ESP32 Transceiver A+ <-> BTT Rodent Transceiver A+
  ESP32 Transceiver B- <-> BTT Rodent Transceiver B-

Termination:
  120Ω resistor across A+/B- at both ends of bus
```

### RTS Control (Direction Control)

RS485 is **half-duplex** - only one device can transmit at a time. The RTS pin controls the direction:

- **RTS HIGH** = Transmit mode (driver enabled)
- **RTS LOW** = Receive mode (receiver enabled)

**Implementation in ESP32 code:**
```cpp
// Set to transmit mode
digitalWrite(RODENT_RTS_PIN, HIGH);
delayMicroseconds(10);  // Allow transceiver to switch
RodentSerial.println("G1 X100 F100");
RodentSerial.flush();   // Wait for transmission to complete
delayMicroseconds(10);
digitalWrite(RODENT_RTS_PIN, LOW);  // Back to receive mode
```

---

## System Architecture

### Axis Mapping

The 4 peristaltic pumps are mapped to 4 axes on the BTT Rodent board:

| Pump | Function | Axis | Stepper Driver | Step Pin | Dir Pin |
|------|----------|------|----------------|----------|---------|
| 1 | DMDEE | X | Driver 1 | GPIO26 | GPIO27 |
| 2 | T-12 | Y | Driver 2 | GPIO25 | GPIO33 |
| 3 | T-9 | Z | Driver 3 | GPIO32 | GPIO21 |
| 4 | L25B | A | Driver 4 | GPIO4 | GPIO22 |

### Motion Model

- Pumps are controlled as linear axes (not rotational)
- Position units: **millimeters (mm)**
- Speed units: **millimeters per minute (mm/min)**
- Each pump has a calibration factor: **ml/mm**

**Example:**
- If pump calibration is 1.0 ml/mm
- To dispense at 100 ml/min
- Required speed = 100 ml/min ÷ 1.0 ml/mm = **100 mm/min**

---

## G-Code Command Reference

### Command Structure

G-code commands are ASCII text strings terminated by newline (`\n`).

**Format:**
```
[Command][Parameters]\n
```

**Example:**
```
G1 X100 F50\n
```

### Command Types

FluidNC supports several command types:

1. **Motion Commands** (G-codes): Control movement
2. **Machine Commands** (M-codes): Control machine state
3. **Configuration Commands** ($-commands): Query/modify settings
4. **Real-time Commands**: Single-character, no newline needed

---

## Pump Control Commands

### Start Pump (Continuous Dispensing)

To start a pump and run it continuously at a specific flow rate:

**Command Format:**
```
G91 G1 [Axis][Distance] F[Feedrate]
```

**Parameters:**
- `G91` - Incremental positioning mode
- `G1` - Linear move
- `[Axis]` - Axis letter (X, Y, Z, or A)
- `[Distance]` - Distance to move in mm (use large value for continuous operation)
- `F[Feedrate]` - Speed in mm/min

**Examples:**

```gcode
G91 G1 X1000 F100    # Start Pump 1 (X axis) at 100 mm/min for 1000mm
G91 G1 Y500 F50      # Start Pump 2 (Y axis) at 50 mm/min for 500mm
G91 G1 Z1000 F75     # Start Pump 3 (Z axis) at 75 mm/min
G91 G1 A1000 F200    # Start Pump 4 (A axis) at 200 mm/min
```

**Multi-axis (start multiple pumps):**
```gcode
G91 G1 X1000 Y1000 Z1000 A1000 F100  # All pumps at 100 mm/min
```

### Stop Pump

**Feed Hold (Pause all motion):**
```
!
```

**Resume after Feed Hold:**
```
~
```

**Stop Specific Axis:**
Feed hold stops all axes. To stop a specific pump, you would typically:
1. Send feed hold `!`
2. Cancel the current operation
3. Restart other pumps as needed

### Emergency Stop

**Soft Reset (stops all motion immediately):**
```
0x18
```
This is the Ctrl-X character (ASCII 24). It performs a soft reset of the controller.

**Note:** After emergency stop, the system enters ALARM state and requires unlock.

---

## Status and Query Commands

### Real-time Status Query

**Command:**
```
?
```

**Response Format:**
```
<Idle|MPos:0.000,0.000,0.000,0.000|FS:0,0>
<Run|MPos:50.123,0.000,0.000,0.000|FS:100,100>
```

**Status Fields:**
- `<State|...>` - System state (Idle, Run, Hold, Alarm, etc.)
- `MPos:X,Y,Z,A` - Machine position for each axis
- `FS:feed,speed` - Current feed rate and spindle speed

**System States:**
- `Idle` - No motion, ready for commands
- `Run` - Executing motion
- `Hold` - Feed hold active (motion paused)
- `Alarm` - Error state, requires reset
- `Home` - Homing cycle in progress

### Position Query

Status query (`?`) returns current position. Parse the response:

```cpp
// Example response: <Idle|MPos:10.5,20.3,5.0,15.2|FS:0,0>
// Parse to extract X=10.5, Y=20.3, Z=5.0, A=15.2
```

---

## Configuration Commands

### Show All Settings

**Command:**
```
$$
```

**Response:** List of all configuration parameters

### Show Startup Messages

**Command:**
```
$I
```

**Response:** Firmware version, build info

### View Specific Setting

**Command:**
```
$[parameter]
```

**Example:**
```
$100    # View X-axis steps per mm
```

### Unlock After Alarm

**Command:**
```
$X
```

This clears the ALARM state after an emergency stop or limit switch trigger.

---

## Real-Time Commands

These commands are single characters sent **without** a newline terminator. They are processed immediately, even while other commands are executing.

| Command | Hex | Description |
|---------|-----|-------------|
| `?` | 0x3F | Status query |
| `!` | 0x21 | Feed hold (pause) |
| `~` | 0x7E | Cycle start/resume |
| Ctrl-X | 0x18 | Soft reset (emergency stop) |

**Usage in code:**
```cpp
rs485_println("?");      // Status query (with newline)
rs485_print("!");        // Feed hold (no newline)
rs485_print("~");        // Resume (no newline)
rs485_println("\x18");   // Soft reset
```

---

## Response Format

### OK Response

When a command is successfully processed:
```
ok
```

### Error Response

When a command fails:
```
error:X
```

Where X is an error code:
- `error:1` - Expected command letter
- `error:2` - Bad number format
- `error:3` - Invalid $ command
- `error:20` - Unsupported command
- etc.

### Alarm Response

When system enters alarm state:
```
ALARM:X
```

Where X is an alarm code:
- `ALARM:1` - Hard limit triggered
- `ALARM:2` - Soft limit triggered
- `ALARM:3` - Abort during cycle
- etc.

**Recovery:** Send `$X` to clear alarm state

---

## Implementation Examples

### Example 1: Start Single Pump

```cpp
bool rodent_start_pump(PumpID pump, float flow_rate_ml_min) {
    // Get calibration for this pump (ml/mm)
    float ml_per_mm = config.pump_ml_per_mm[pump];

    // Calculate feedrate (mm/min)
    float feedrate_mm_min = flow_rate_ml_min / ml_per_mm;

    // Limit to safe range
    feedrate_mm_min = constrain(feedrate_mm_min, 10.0f, 500.0f);

    // Map pump to axis letter
    char axes[] = {'X', 'Y', 'Z', 'A'};
    char axis = axes[pump];

    // Build G-code command
    char cmd[32];
    sprintf(cmd, "G91 G1 %c1000 F%.1f", axis, feedrate_mm_min);

    // Send via RS485
    rs485_println(cmd);

    pump_running[pump] = true;
    return true;
}
```

**Example call:**
```cpp
// Start Pump 1 (DMDEE) at 100 ml/min
rodent_start_pump(PUMP_1_DMDEE, 100.0);
// Sends: "G91 G1 X1000 F100.0"
```

### Example 2: Stop All Pumps

```cpp
bool rodent_stop_all_pumps(void) {
    // Send feed hold
    rs485_print("!");

    // Mark all pumps as stopped
    for (int i = 0; i < NUM_PUMPS; i++) {
        pump_running[i] = false;
    }

    return true;
}
```

### Example 3: Emergency Stop

```cpp
bool rodent_emergency_stop(void) {
    // Send soft reset (Ctrl-X)
    rs485_println("\x18");

    // Mark all pumps as stopped
    for (int i = 0; i < NUM_PUMPS; i++) {
        pump_running[i] = false;
    }

    // Wait for reset to complete
    delay(1000);

    // Clear alarm state
    rs485_println("$X");

    return true;
}
```

### Example 4: Check Status

```cpp
bool rodent_check_status(void) {
    // Send status query
    rs485_println("?");

    // Wait for response
    uint32_t start = millis();
    while (millis() - start < 1000) {
        if (RodentSerial.available()) {
            String response = RodentSerial.readStringUntil('\n');

            // Check if system is idle
            if (response.startsWith("<Idle")) {
                return true;  // System is ready
            }

            // Parse position if needed
            // <Run|MPos:10.5,20.3,5.0,15.2|FS:100,0>
            int mpos_idx = response.indexOf("MPos:");
            if (mpos_idx > 0) {
                // Extract and parse positions
                // ... parsing logic ...
            }
        }
        delay(10);
    }

    return false;  // No response
}
```

### Example 5: Homing Sequence

```cpp
bool rodent_home_all(void) {
    // Send homing command
    rs485_println("$H");

    // Wait for homing to complete
    delay(5000);

    // Check status
    rs485_println("?");

    // Reset position tracking
    for (int i = 0; i < NUM_PUMPS; i++) {
        pump_position[i] = 0.0f;
    }

    return true;
}
```

---

## Flow Rate Calculations

### Converting Flow Rate to Feedrate

The key to pump control is converting desired flow rate (ml/min) to G-code feedrate (mm/min).

**Formula:**
```
Feedrate (mm/min) = Flow_Rate (ml/min) / Calibration (ml/mm)
```

**Where:**
- **Flow_Rate**: Desired flow rate in ml/min
- **Calibration**: Pump-specific calibration in ml/mm
- **Feedrate**: G-code feed rate in mm/min

### Pump Calibration

Each pump must be calibrated to determine ml/mm ratio.

**Calibration Procedure:**
1. Command pump to move 100mm at known feedrate
2. Measure actual volume dispensed
3. Calculate: `ml/mm = volume_dispensed / 100`
4. Store in `config.pump_ml_per_mm[pump]`

**Example:**
```
Commanded: G1 X100 F100
Dispensed: 150 ml
Calibration: 150 ml / 100 mm = 1.5 ml/mm
```

### Example Calculations

**Given:**
- Desired flow rate: 100 ml/min
- Pump calibration: 1.5 ml/mm

**Calculate:**
```
Feedrate = 100 ml/min / 1.5 ml/mm = 66.67 mm/min
```

**G-code command:**
```gcode
G91 G1 X1000 F66.7
```

**Multiple Pumps Example:**

| Pump | Desired (ml/min) | Calibration (ml/mm) | Feedrate (mm/min) | Command |
|------|------------------|---------------------|-------------------|---------|
| 1 | 100 | 1.0 | 100.0 | `G91 G1 X1000 F100.0` |
| 2 | 50 | 1.5 | 33.3 | `G91 G1 Y1000 F33.3` |
| 3 | 75 | 2.0 | 37.5 | `G91 G1 Z1000 F37.5` |
| 4 | 200 | 0.8 | 250.0 | `G91 G1 A1000 F250.0` |

---

## Error Handling

### Communication Errors

**No Response:**
```cpp
// Timeout after 1 second
if (millis() - start > 1000) {
    Serial.println("Error: No response from Rodent board");
    return false;
}
```

**Invalid Response:**
```cpp
if (response.startsWith("error:")) {
    int error_code = response.substring(6).toInt();
    Serial.printf("G-code error: %d\n", error_code);
    return false;
}
```

### Alarm States

**Check for Alarm:**
```cpp
if (response.startsWith("ALARM:")) {
    int alarm_code = response.substring(6).toInt();
    Serial.printf("Alarm state: %d\n", alarm_code);

    // Clear alarm
    rs485_println("$X");
    return false;
}
```

### RS485 Communication Issues

**Common issues:**
1. **No response**: Check RTS timing, transceiver wiring
2. **Garbled data**: Check baud rate, termination resistors
3. **Intermittent errors**: Check cable quality, length, EMI sources

**Debugging:**
```cpp
// Add verbose logging
Serial.printf("TX: %s\n", command);
// ... wait for response ...
Serial.printf("RX: %s\n", response.c_str());
```

**RTS Timing:**
- Must set RTS HIGH before transmitting
- Must wait for transmission to complete (use `flush()`)
- Must switch back to receive mode after transmission
- Typical timing: 10-100 microseconds for transceiver switching

---

## Summary

### Quick Command Reference

| Action | Command | Example |
|--------|---------|---------|
| Start pump | `G91 G1 [Axis][Dist] F[Rate]` | `G91 G1 X1000 F100` |
| Stop all pumps | `!` | `!` |
| Resume | `~` | `~` |
| Emergency stop | `0x18` | `\x18` |
| Status query | `?` | `?` |
| Home all | `$H` | `$H` |
| Clear alarm | `$X` | `$X` |
| View settings | `$$` | `$$` |
| Reset | `0x18` | `\x18` |

### Communication Checklist

- [ ] RS485 transceivers properly wired
- [ ] Termination resistors installed (120Ω)
- [ ] RTS pin configured and controlled
- [ ] Baud rate: 115200
- [ ] Data format: 8N1
- [ ] Commands terminated with `\n`
- [ ] Wait for `ok` or status response
- [ ] Handle errors and alarms

### Best Practices

1. **Always check status** before sending motion commands
2. **Use feed hold** (`!`) for graceful pause, not emergency stop
3. **Calibrate pumps** before production use
4. **Implement timeouts** on all serial communication
5. **Log all commands** and responses for debugging
6. **Clear alarm state** after emergency stop before resuming
7. **Use incremental mode** (G91) for continuous pumping
8. **Limit feedrate** to safe range for your pumps
9. **Test communication** during initialization
10. **Monitor cable length** - shorter is better for reliability

---

## Additional Resources

- [FluidNC Documentation](https://github.com/bdring/FluidNC/wiki)
- [G-code Reference](https://linuxcnc.org/docs/html/gcode.html)
- [RS485 Standard (EIA-485)](https://en.wikipedia.org/wiki/RS-485)
- BTT Rodent V1.1 Board Schematic
- MAX485 Transceiver Datasheet

---

**Document Version:** 1.0
**Last Updated:** 2025-11-01
**Author:** Claude AI Assistant
