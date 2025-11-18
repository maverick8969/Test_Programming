# Peristaltic Pump Control System - Comprehensive Documentation

**Project**: ESP32-based Peristaltic Pump Control System
**Hardware**: ESP32 + BTT Rodent V1.1 (FluidNC) + 4 Peristaltic Pumps
**Last Updated**: 2025-11-18

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Hardware Architecture](#hardware-architecture)
3. [Communication Protocols](#communication-protocols)
4. [Test Program Breakdown](#test-program-breakdown)
5. [Features Developed](#features-developed)
6. [Critical Bug Fixes](#critical-bug-fixes)
7. [Wiring & Configuration](#wiring--configuration)
8. [Production-Ready Configuration](#production-ready-configuration)

---

## 1. System Overview

### Purpose
This is a comprehensive peristaltic pump control system designed for precise liquid dispensing. The system controls 4 independent pump channels (X, Y, Z, A axes) with features including:

- Precise volume and flow rate control
- Real-time weight-based dispensing with digital scale integration
- Multi-pump sequential and simultaneous operation
- Recipe management system
- Full safety features including emergency stop
- Visual feedback via WS2812B LED strips
- User interface via rotary encoder and LCD display
- Data logging and monitoring capabilities

### System Components

**Controller**: ESP32 Development Board
- Master controller managing all peripherals
- Communicates with motor controller via UART/RS485
- Manages user interface and sensor inputs
- Provides real-time feedback and safety monitoring

**Motor Controller**: BTT Rodent V1.1 with FluidNC Firmware
- Controls 4 stepper motor drivers (TMC2209)
- G-code interpreter for motion commands
- Independent axis control (X, Y, Z, A)
- Real-time status reporting

**Pumps**: 4 Peristaltic Pumps
- Pump 1 (X-axis): DMDEE - Cyan LED indicator
- Pump 2 (Y-axis): T-12 - Magenta LED indicator
- Pump 3 (Z-axis): T-9 - Yellow LED indicator
- Pump 4 (A-axis): L25B - White LED indicator

**User Interface**:
- 3 Push buttons (START, MODE, STOP)
- Rotary encoder with push button
- 16x2 I2C LCD display
- 32 WS2812B addressable RGB LEDs (4 strips × 8 LEDs)

**Sensors**:
- Digital scale with RS232 output (for weight-based dispensing)

---

## 2. Hardware Architecture

### Pin Assignments

#### Control Inputs
| Component | GPIO Pin | Type | Pull-up | Active Level | Notes |
|-----------|----------|------|---------|--------------|-------|
| START Button | 13 | Input | Internal | LOW | Standard GPIO |
| MODE Button | 14 | Input | Internal | LOW | Standard GPIO |
| STOP Button (E-Stop) | 33 | Input | Internal | LOW | Emergency stop |
| Encoder CLK | 26 | Input | Internal | LOW | Quadrature A |
| Encoder DT | 27 | Input | Internal | LOW | Quadrature B |
| Encoder SW | 34 | Input | **EXTERNAL 10kΩ** | LOW | **Input-only GPIO** |

#### Communication Interfaces
| Interface | TX Pin | RX Pin | Additional | Protocol | Baud Rate |
|-----------|--------|--------|------------|----------|-----------|
| USB Serial (UART0) | GPIO 1 | GPIO 3 | - | UART | 115200 |
| Scale (UART1) | GPIO 32 | GPIO 35 | MAX3232 | RS232 | 9600 |
| Rodent (UART2) | GPIO 17 | GPIO 16 | GPIO 4 (RTS) | UART/RS485 | 115200 |
| LCD Display | GPIO 21 (SDA) | GPIO 22 (SCL) | - | I2C | 100 kHz |

#### Output Devices
| Device | GPIO Pin | Protocol | Notes |
|--------|----------|----------|-------|
| Built-in LED | 2 | Digital Out | Test only (strapping pin) |
| WS2812B LEDs | 25 | RMT | 32 LEDs total (4×8) |

### Motor/Pump Mapping
| Pump ID | Description | Axis | Motor Driver | LED Strip | LED Color |
|---------|-------------|------|--------------|-----------|-----------|
| 1 | DMDEE | X | Rodent X | Strip 0 (LEDs 0-7) | Cyan |
| 2 | T-12 | Y | Rodent Y | Strip 1 (LEDs 8-15) | Magenta |
| 3 | T-9 | Z | Rodent Z | Strip 2 (LEDs 16-23) | Yellow |
| 4 | L25B | A | Rodent A | Strip 3 (LEDs 24-31) | White |

### Power Requirements
- **ESP32**: 3.3V (USB powered or external regulator)
- **WS2812B LEDs**: 5V @ 1.5A max (external power supply required)
- **I2C LCD**: 5V (from ESP32 VIN or external)
- **BTT Rodent**: 24V (separate power supply)
- **MAX3232 (Scale)**: 3.3V (from ESP32)
- **MAX485 (RS485)**: 3.3V or 5V (check datasheet)

**CRITICAL**: Common ground connection required between ALL devices!

---

## 3. Communication Protocols

### 3.1 UART Communication

#### UART0 - USB Serial Monitor
- **Purpose**: Debug output and user commands
- **Pins**: GPIO 1 (TX), GPIO 3 (RX)
- **Baud**: 115200
- **Config**: 8N1 (8 data bits, no parity, 1 stop bit)
- **Usage**: All tests use Serial.begin(115200)

#### UART1 - Digital Scale (RS232)
- **Purpose**: Read weight from digital scale
- **Pins**: GPIO 32 (TX), GPIO 35 (RX - input-only)
- **Baud**: 9600 (configurable)
- **Config**: 8N1
- **Level Converter**: MAX3232 REQUIRED (RS232 is ±12V)
- **Protocol**: Burst command mode
  ```
  Command: @P<CR><LF> (literal text, 10 ASCII bytes)
  Timing: 13 bursts with 7ms char delay, 9ms line delay
  Read window: 160ms after burst
  ```
- **Used in**: Test 06, Test 15

#### UART2 - BTT Rodent Motor Controller
- **Purpose**: G-code commands to FluidNC
- **Pins**: GPIO 17 (TX), GPIO 16 (RX)
- **Baud**: 115200
- **Config**: 8N1
- **Modes**:
  - Direct UART (cable < 1m, common ground)
  - RS485 (longer distances, twisted pair, 120Ω termination)
- **Commands**: G-code (G0, G1, G92), FluidNC ($I, $$, $H, !, ~, Ctrl-X)
- **Used in**: Tests 07-20

### 3.2 RS485 Communication (Optional for UART2)

**When to Use**: Cable runs > 1 meter, noisy environments

**Hardware**: MAX485 or similar transceiver
- **TX Pin**: GPIO 17 → MAX485 DI
- **RX Pin**: GPIO 16 ← MAX485 RO
- **RTS Pin**: GPIO 4 → MAX485 DE/RE (direction control)
  - HIGH = Transmit mode
  - LOW = Receive mode
  - Switch delay: 100µs

**Wiring**:
- Twisted pair cable for A/B lines (Belden 9723 recommended)
- 120Ω termination resistors at both ends
- Common ground between ESP32 and Rodent
- Cable shield grounded at one end only

**Used in**: Test 07, Test 20

### 3.3 I2C Communication

**Purpose**: LCD display communication

**Configuration**:
- **SDA**: GPIO 21
- **SCL**: GPIO 22
- **Frequency**: 100 kHz (standard mode)
- **LCD Address**: 0x27 (or 0x3F - check with I2C scanner)

**Devices on Bus**:
- 16x2 LCD with I2C backpack (PCF8574 or similar)
- Potentially: RTC (0x68), BME280 (0x76), ADS1115 (0x48)

**Used in**: Tests 03-04, 10-11, 16, 19

### 3.4 WS2812B LED Protocol (RMT)

**Hardware**: 32 addressable RGB LEDs (WS2812B/NeoPixel)

**Configuration**:
- **Data Pin**: GPIO 25
- **Color Order**: GRB (not RGB!)
- **Brightness**: 25% default (64/255) for safety
- **LEDs per Strip**: 8
- **Total Strips**: 4 (one per pump)

**Timing Requirements**:
- WS2812B requires ±150ns timing precision
- ESP32 RMT peripheral handles timing automatically
- **CRITICAL**: Must disable WiFi and Bluetooth before initialization

**Initialization Sequence**:
```cpp
WiFi.mode(WIFI_OFF);
btStop();
esp_bt_controller_disable();
FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
FastLED.clear(true);
delay(50);  // RMT stabilization
```

**Power Considerations**:
- WS2812B expects 5V data signal (marginal with 3.3V ESP32)
- Solutions: 1N4001 diode trick or 74HCT245 level shifter
- Current draw: ~60mA per LED at full white

**Used in**: Tests 05, 11, 17, 19, 20

---

## 4. Test Program Breakdown

### Progressive Test Structure

The codebase contains 20 progressive tests, each building on previous functionality:

#### Phase 1: Basic Hardware Verification (Tests 00-05)

**Test 00: Blink Test** (`test_00_blink.cpp`)
- **Purpose**: Verify ESP32 basic operation
- **Hardware**: Built-in LED (GPIO 2)
- **Features**: LED blink, serial output, system info
- **Use Case**: Initial board verification

**Test 01: Push Buttons** (`test_01_buttons.cpp`)
- **Purpose**: Test physical control buttons
- **Hardware**: 3 buttons (GPIO 13, 14, 33)
- **Features**: Debouncing (50ms), press/release detection, statistics
- **Use Case**: Verify button wiring and functionality

**Test 02: Rotary Encoder** (`test_02_encoder.cpp`)
- **Purpose**: Test encoder for menu navigation
- **Hardware**: Encoder CLK/DT (GPIO 26/27), SW (GPIO 34 - needs external pull-up!)
- **Features**: Direction detection, position tracking, button press
- **Use Case**: User input for value adjustment

**Test 03: I2C Scanner** (`test_03_i2c_scanner.cpp`)
- **Purpose**: Discover I2C devices
- **Hardware**: I2C bus (GPIO 21/22)
- **Features**: Scans 0x00-0x7F, auto-rescan every 5s
- **Use Case**: Find LCD address, verify I2C wiring

**Test 04: LCD Display** (`test_04_lcd.cpp`)
- **Purpose**: Test LCD functionality
- **Hardware**: 16x2 I2C LCD (0x27 or 0x3F)
- **Features**: 4 display modes (welcome, system info, memory, uptime)
- **Use Case**: Verify LCD communication and display

**Test 05: WS2812B LED Strips** (`test_05_leds.cpp`)
- **Purpose**: Test addressable RGB LEDs
- **Hardware**: 32 WS2812B LEDs on GPIO 25
- **Features**: Color patterns, rainbow, chase, per-strip control
- **Patterns**: Off, solid colors, rainbow cycle, chase animation
- **Use Case**: Visual feedback system for pump status

#### Phase 2: External Communication (Tests 06-08)

**Test 06: Digital Scale via RS232** (`test_06_scale.cpp`)
- **Purpose**: Read weight from digital scale
- **Hardware**: UART1 (GPIO 32/35) + MAX3232 level converter
- **Protocol**: Burst command "@P<CR><LF>" with precise timing
- **Features**:
  - Continuous burst mode (matches Python implementation)
  - Automatic timing optimization (14 configurations)
  - HEX and ASCII display
  - Weight parsing from common formats
- **Timing**: 7ms char delay, 9ms line delay, 160ms read window
- **Use Case**: Weight-based dispensing control

**Test 07: RS485 to BTT Rodent** (`test_07_rs485.cpp`)
- **Purpose**: Test RS485 communication with motor controller
- **Hardware**: UART2 (GPIO 17/16) + MAX485 + RTS (GPIO 4)
- **Protocol**: FluidNC G-code at 115200 baud
- **Features**:
  - 3 automated test phases
  - Interactive command mode
  - Direction control (RTS pin management)
- **Commands**: $I, ?, $$, $H, G0/G1, !, ~, Ctrl-X
- **Use Case**: Long-distance motor control communication

**Test 08: Direct UART (No RS485)** (`test_08_uart.cpp`)
- **Purpose**: Test UART without RS485 transceiver
- **Hardware**: Direct UART2 connection (GPIO 17/16)
- **Wiring**: ESP32 TX→Rodent RX, ESP32 RX←Rodent TX, common ground
- **Distance**: < 1 meter recommended
- **Configuration**: Requires `btt_rodent_uart.yaml` on Rodent
- **Use Case**: Simplified wiring for short distances

#### Phase 3: Integrated User Interface (Tests 09-11)

**Test 09: UART + Button/Encoder Control** (`test_09_uart_buttons.cpp`)
- **Purpose**: Physical control of pumps
- **Hardware**: All buttons + encoder + UART
- **Controls**:
  - START button: Move selected pump (G0 X/Y/Z/A 10 F150)
  - MODE button: Cycle pump selection
  - ENCODER rotate: Select pump (CW/CCW)
  - ENCODER button: Start selected pump
  - STOP button: Emergency stop (!)
- **Use Case**: Manual pump operation without computer

**Test 10: UART + LCD + Encoder** (`test_10_uart_lcd.cpp`)
- **Purpose**: Full UI with LCD status
- **Hardware**: LCD + Encoder + UART
- **Display**:
  - Line 1: Pump selection / state
  - Line 2: Instructions
- **Features**: Status query every 2s, idle detection
- **Use Case**: User-friendly pump control with feedback

**Test 11: UART + LED + LCD + Encoder** (`test_11_uart_leds.cpp`)
- **Purpose**: Complete visual feedback system with automated testing
- **Hardware**: LEDs + LCD + Encoder + UART
- **Automated Test Sequence**:
  1. **Phase 1**: All pumps forward + LED scrolling right (→)
  2. **Phase 2**: All pumps reverse + LED scrolling left (←)
  3. **Phase 3**: Emergency stop test with red LED flash
- **LED Status Colors**:
  - Green = IDLE
  - Blue = RUNNING
  - Red (solid) = ERROR
  - Red (flash) = EMERGENCY STOP
- **Control**: Encoder for brightness, button to start/stop test
- **Use Case**: System integration testing and demo

#### Phase 4: Precision Pump Control (Test 12)

**Test 12: Single Pump Flow Rate Control** (`test_12_single_pump.cpp`)
- **Purpose**: Precise single pump dispensing
- **Calibration**:
  ```cpp
  ML_PER_MM = 0.05      // Pump-dependent calibration
  STEPS_PER_MM = 80.0    // Motor steps per mm
  SAFE_TEST_FEEDRATE = 300  // Max mm/min
  ```
- **Commands**:
  - Serial: `d <volume> <flowrate>` (e.g., "d 5.0 10.0" = 5ml @ 10ml/min)
  - Encoder: Adjust flow rate (1-15 ml/min)
  - Button: Start dispensing
- **Conversion**:
  - `distance_mm = volume_ml / ML_PER_MM`
  - `feedrate_mm_min = flow_rate_ml_min / ML_PER_MM`
- **Use Case**: Accurate single chemical dispensing

#### Phase 5: Multi-Pump Coordination (Tests 13-14)

**Test 13: Multi-Pump Sequential** (`test_13_multi_sequential.cpp`)
- **Purpose**: Execute recipes step-by-step
- **Example Recipe** (4-step mixing):
  ```
  Step 1: Pump X: 10ml @ 7.5ml/min (150 mm/min)
  Step 2: Pump Y: 5ml @ 6.0ml/min (120 mm/min)
  Step 3: Pump Z: 7.5ml @ 4.5ml/min (90 mm/min)
  Step 4: Pump A: 2.5ml @ 3.0ml/min (60 mm/min)
  ```
- **Features**:
  - Waits for "Idle" state before each step
  - 500ms pause between steps
  - Encoder navigation through steps
- **Use Case**: Complex formulation mixing

**Test 14: Multi-Pump Simultaneous** (`test_14_multi_simultaneous.cpp`)
- **Purpose**: Dispense multiple pumps at same time
- **Preset Patterns**:
  1. Equal mix: 5ml each @ 20ml/min
  2. Ratio 2:1:1:0.5 @ 15ml/min
  3. Custom: 3:2:1.5:0.5 @ 10ml/min
- **Command Format**: `G1 X<dist> Y<dist> Z<dist> A<dist> F<feedrate>`
- **Use Case**: Synchronized multi-component dispensing

#### Phase 6: Closed-Loop Control (Test 15)

**Test 15: Scale Integration** (`test_15_scale_integration.cpp`)
- **Purpose**: Weight-based dispensing (closed-loop)
- **Hardware**: UART1 (Scale) + UART2 (Rodent)
- **Features**:
  - Set target weight (0.5-100g via encoder)
  - Continuous dispensing until target reached
  - Real-time weight monitoring
  - Auto-stop when target met
  - Auto-reset after completion
- **Polling Strategy**:
  - Fast (200ms) when dispensing
  - Slow (2s) when idle (keeps encoder responsive)
- **Use Case**: Precise weight-based formulations

#### Phase 7: Advanced Features (Tests 16-20)

**Test 16: Recipe System** (`test_16_recipe_system.cpp`)
- **Purpose**: Comprehensive recipe management
- **Hardware**: LCD + Encoder + Buttons + UART
- **Preset Recipes**:
  1. **Cleaning Flush**: 5ml each pump @ 30ml/min
  2. **Color Mix**: 10ml X, 5ml Y, 2.5ml Z
  3. **Nutrient Mix**: 20ml X, 2ml Y, 1.5ml Z, 0.5ml A
- **Modes**:
  - BROWSE: Select recipe with encoder
  - EXECUTING: Follow recipe steps, LCD shows progress
- **Controls**:
  - Encoder: Browse/select recipes
  - START/Encoder button: Start selected recipe
  - STOP: Emergency stop
  - Serial: Type 1-3 for recipe selection
- **Use Case**: Production-ready recipe execution

**Test 17: Emergency Stop & Safety** (`test_17_safety_features.cpp`)
- **Purpose**: Safety interlocks and emergency shutdown
- **Hardware**: LEDs + Buttons + UART
- **Safety Features**:
  - Hardware E-Stop button (GPIO 33)
  - Heartbeat timeout (5 seconds)
  - Command timeout (30 seconds max run)
  - Alarm state detection
  - Visual LED feedback
- **LED Safety Codes**:
  - Green = Normal operation
  - Yellow = Warning
  - Red = Emergency stop
  - Flash = Alarm state
- **Responses**: Soft reset (Ctrl-X), unlock ($X)
- **Use Case**: Critical safety testing

**Test 18: Data Logging & Monitoring** (`test_18_data_logging.cpp`)
- **Purpose**: Audit trail and debugging
- **Buffer**: Circular buffer of last 50 commands
- **Logged Data**:
  - Timestamp (milliseconds)
  - Command sent
  - Response received
  - Duration (ms)
  - Success/failure status
- **Modes**:
  - Normal: Log actual commands only (no status spam)
  - Verbose: Log status queries too
- **Commands**:
  - `l`: Show log
  - `s`: Show statistics
  - `c`: Clear log
  - `v`: Toggle verbose mode
  - `?`: Manual status query
- **Statistics**:
  - Total commands sent
  - Success rate (%)
  - Failed commands (%)
  - System uptime
  - Free heap memory
- **Rate Limiting**: Status queries every 5s (prevents output flood)
- **Use Case**: Production monitoring and troubleshooting

**Test 19: Full System Integration** (`test_19_full_integration.cpp`)
- **Purpose**: Complete end-to-end system test
- **Hardware**: ALL peripherals (LCD, LEDs, Buttons, Encoder, UART, Scale)
- **State Machine**:
  1. **IDLE**: Display system status
  2. **SELECT**: Encoder browse 4 recipes
  3. **RUNNING**: Execute recipe steps
  4. **COMPLETE**: Show success message
  5. **ERROR**: Show error state
- **LED Feedback**: Progress bar (8 LEDs per step)
- **Interrupt-Driven**: Encoder uses falling edge interrupt for responsiveness
- **4 Recipes**: Water flush, 2 color mixes, nutrient mix
- **Use Case**: Final integration verification

**Test 20: LED Motor Status Display** (`test_20_led_motor_status.cpp`)
- **Purpose**: Real-time visual motor activity indication
- **Hardware**: LEDs + RS485/UART to Rodent
- **Monitoring**: Parse FluidNC status messages every 100ms
- **Status Parse**: Extract MPos from `<Jog|MPos:X,Y,Z,A|FS:4,0>`
- **Movement Detection**:
  - Threshold: 0.001mm position change
  - Active timeout: 500ms after movement stops
- **LED Display**:
  - Active motor: Bright assigned color
  - Idle motor: Dim (10% brightness)
- **Statistics**:
  - Parsing success/failure rate
  - Current positions
  - Activity status per motor
- **Use Case**: Visual debugging of motor activity

---

## 5. Features Developed

### Core Dispensing Features

1. **Single Pump Control** (Test 12)
   - Volume-based dispensing (ml)
   - Flow rate control (ml/min)
   - Pump-specific calibration (ML_PER_MM)
   - Safe feedrate limiting

2. **Multi-Pump Sequential** (Test 13)
   - Step-by-step recipe execution
   - Automatic idle detection between steps
   - Configurable volumes and flow rates per step

3. **Multi-Pump Simultaneous** (Test 14)
   - Synchronized multi-axis movement
   - Preset ratio patterns
   - Single G-code command for all axes

4. **Weight-Based Dispensing** (Test 15)
   - Closed-loop control with digital scale
   - Target weight setting (0.5-100g)
   - Auto-stop at target weight
   - Real-time weight monitoring

5. **Recipe Management** (Test 16, 19)
   - Pre-programmed recipes
   - Step-by-step execution with progress display
   - Encoder-based recipe selection
   - LCD feedback during execution

### User Interface Features

1. **Physical Controls**
   - 3-button control (START, MODE, STOP)
   - Rotary encoder for value adjustment
   - Encoder button for selection/confirmation

2. **LCD Display** (Tests 04, 10, 11, 16, 19)
   - 16x2 character display
   - Real-time status updates
   - Menu navigation
   - Progress indication

3. **LED Visual Feedback** (Tests 05, 11, 17, 19, 20)
   - 32 addressable RGB LEDs (4 strips × 8 LEDs)
   - Pump-specific color coding
   - Status indication (idle/running/error)
   - Scrolling effects for direction indication
   - Progress bar visualization
   - Real-time motor activity display

### Communication Features

1. **FluidNC G-code Interface** (Tests 07-20)
   - G0/G1 motion commands
   - Position reset (G92)
   - Status queries (?)
   - System info ($I, $$)
   - Emergency stop (!)
   - Resume (~)
   - Soft reset (Ctrl-X)
   - Unlock ($X)

2. **Digital Scale Integration** (Tests 06, 15)
   - RS232 communication via MAX3232
   - Burst command protocol
   - Automatic timing optimization
   - Continuous polling mode
   - Weight parsing from multiple formats

3. **RS485 Long-Distance** (Tests 07, 20)
   - MAX485 transceiver support
   - Automatic direction control
   - Twisted-pair wiring
   - Termination resistor support

### Safety Features

1. **Emergency Stop System** (Tests 11, 17, all pump tests)
   - Hardware E-Stop button
   - Software emergency stop command (!)
   - Visual indication (red LEDs)
   - Graceful recovery (resume or reset)

2. **Timeout Protection** (Test 17)
   - Heartbeat timeout (5s)
   - Command timeout (30s max)
   - Automatic alarm detection

3. **Error Handling**
   - Response validation
   - State detection (Idle, Run, Jog, Hold, Alarm)
   - Automatic retry on communication errors
   - Clear error messages

4. **Auto-Reset Features** (Test 15)
   - Automatic position reset after dispensing
   - Auto-return to idle state
   - Ready for next operation

### Monitoring & Logging Features

1. **Data Logging** (Test 18)
   - Circular buffer (50 entries)
   - Timestamp tracking
   - Command/response logging
   - Success/failure tracking
   - Duration measurement

2. **Statistics Tracking** (Test 18)
   - Total commands sent
   - Success rate calculation
   - Failure rate tracking
   - System uptime
   - Memory monitoring

3. **Verbose Mode** (Test 18)
   - Toggle detailed logging
   - Filter status query spam
   - Debug-friendly output

4. **Real-time Monitoring** (Test 20)
   - Motor position tracking
   - Movement detection
   - Activity visualization
   - Parsing statistics

### Calibration & Configuration

1. **Pump Calibration**
   - ML_PER_MM parameter (pump-dependent)
   - STEPS_PER_MM setting
   - Safe feedrate limits
   - Per-pump color assignment

2. **Timing Optimization**
   - Scale burst timing auto-test
   - Polling rate optimization
   - Encoder responsiveness tuning

3. **LED Configuration**
   - Brightness adjustment
   - Color coding per pump
   - Pattern customization
   - WiFi/BT disable for timing stability

---

## 6. Critical Bug Fixes

### 6.1 LED Data Corruption (Commit 2d6c124)

**Problem**: WS2812B LEDs showed random colors and corrupted data on GPIO 25

**Root Cause**: ESP32 WiFi/Bluetooth timing interference with WS2812B's strict ±150ns timing requirements

**Solution Implemented**:
1. Disable WiFi and Bluetooth before LED initialization
   ```cpp
   WiFi.mode(WIFI_OFF);
   btStop();
   esp_bt_controller_disable();
   ```
2. Clear LED buffer to remove garbage data
   ```cpp
   FastLED.clear(true);
   ```
3. Add 50ms stabilization delay for RMT peripheral
4. Add required includes (WiFi.h, esp_bt.h)

**Files Fixed**:
- test_05_leds.cpp
- test_11_uart_leds.cpp
- test_17_safety_features.cpp
- test_19_full_integration.cpp
- test_20_led_motor_status.cpp

**Impact**: High - Affects all LED-based visual feedback

---

### 6.2 Data Logging Performance Issues (Commit badaec3)

**Problem**:
- System logged every FluidNC response including continuous status updates
- Serial output overwhelmed with status messages
- System became unusable due to output flood

**Root Cause**: No filtering between command responses and status query responses

**Solution Implemented**:

1. **Intelligent Logging**:
   - Only log actual commands by default
   - Filter status responses unless verbose mode enabled
   - Distinguish between command and status updates

2. **Rate Limiting**:
   - Status queries limited to every 5 seconds (was continuous)
   - Prevents query storms
   - Only queries when not waiting for response

3. **Verbose Mode Toggle**:
   - New `v` command to enable/disable verbose logging
   - Default: OFF (no status spam)
   - Clearly indicated on startup

4. **Response Timeout**:
   - 2 second timeout for responses
   - Prevents hanging on lost messages
   - Timeout only shown in verbose mode

5. **Response Classification**:
   - Auto-detects status responses (<Idle, <Run, <Jog, etc.)
   - Treats separately from command responses
   - Only relevant responses in statistics

**New Commands**:
- `v`: Toggle verbose logging
- `?`: Manual status query

**Files Fixed**: test_18_data_logging.cpp

**Impact**: Critical - Restored system usability for data logging

---

### 6.3 Encoder Responsiveness in Scale Integration (Commit 36c3447)

**Problem**: Encoder very slow to respond during scale weight monitoring

**Root Cause**: Scale polled continuously (~1.2s per cycle) in tight loop, encoder only checked once per second

**Solution Implemented**:

Smart polling strategy:
- **IDLE mode**: Poll scale every 2 seconds (encoder very responsive)
- **DISPENSING mode**: Poll every 200ms (fast enough to stop at target)

```cpp
if (dispensing) {
  delay(200);  // Fast polling when active
} else {
  delay(2000);  // Slow polling when idle
}
// Encoder checked in tight loop between scale reads
```

**Files Fixed**: test_15_scale_integration.cpp

**Impact**: High - Dramatically improved user experience

---

### 6.4 Scale Command Format (Commit 000f755)

**Problem**: Scale not responding to commands from ESP32

**Root Cause**: Wrong command format sent to scale

**Incorrect**:
```cpp
"@P\r\n"  // 4 bytes: @ P 0x0D 0x0A
```

**Correct**:
```cpp
"@P<CR><LF>"  // 10 bytes: @ P < C R > < L F >
```

**Explanation**: The scale firmware expects the **literal ASCII text** "@P<CR><LF>", not the control characters. Python code comment confirmed: "literal text, as requested"

**Files Fixed**:
- test_06_scale.cpp
- test_15_scale_integration.cpp

**Impact**: Critical - Scale communication completely broken without this fix

---

### 6.5 Scale Continuous Mode (Commit 1f30776)

**Problem**: ESP32 scale reading didn't match working Python code behavior

**Root Cause**: ESP32 only sent bursts manually or every 2s, Python sends continuous bursts with no delay

**Python Behavior**:
```python
while True:
    send_burst()
    read_window()
    # No delay - immediately repeat
```

**ESP32 Previous**:
- Single burst on startup
- Manual with 'r' command
- Auto-read every 2s with 'a' command

**ESP32 Fixed**:
- Continuous mode ON by default
- Sends burst → reads → immediately repeats (no delay)
- Type 'c' to pause/resume
- Type 'r' for single burst when paused

**Files Fixed**: test_06_scale.cpp

**Impact**: Critical - Ensures ESP32 matches working Python implementation exactly

---

### 6.6 Compilation Errors

**Forward Declaration Issues**:
- Fixed missing forward declarations in tests 11, 12, 15
- Added function prototypes before usage
- Commits: ca03bb8, b024346

**Naming Conflicts**:
- Renamed MAX_FEEDRATE_MM_MIN to SAFE_TEST_FEEDRATE
- Resolved constant naming collision
- Commits: 66b85e7, 4577258

**Impact**: Medium - Build system fixes, no runtime impact

---

### 6.7 UART Configuration Fixes

**FluidNC UART Pin Configuration** (Commits 5519b57, f39e491, 58bb902):
- Fixed YAML syntax errors in FluidNC configuration
- Corrected UART pins to match BTT Rodent silkscreen
- Updated btt_rodent_uart.yaml with proper pin assignments

**ESP32-C3 Compatibility** (Commit beb1e91):
- Fixed UART port for ESP32-C3 compatibility
- Added platform-specific settings

**Impact**: High - Essential for UART communication to work

---

### 6.8 G-code Feedrate Safety Limits (Commit 89c7e48)

**Problem**: Some tests used unsafe feedrates that could damage pumps

**Solution**: Set safe G-code feedrate limits across all pump control tests

**Safe Limits Established**:
```cpp
SAFE_TEST_FEEDRATE = 300  // mm/min max for testing
```

**Files Updated**: All pump control tests (12-19)

**Impact**: High - Prevents hardware damage

---

### 6.9 Ground Connection Warnings (Commit 6436d04)

**Problem**: Users experiencing communication failures

**Root Cause**: Missing common ground connection between ESP32 and peripherals

**Solution**: Emphasized **CRITICAL** ground connection requirements in all communication-related tests

**Documentation Added**:
- Bold/capitalized warnings in code comments
- Wiring diagrams showing ground connections
- Troubleshooting sections

**Impact**: High - Prevents most common wiring error

---

### 6.10 Power Supply Voltage Warning (Commit f8c3ab9)

**Problem**: WS2812B LEDs sometimes unreliable with 3.3V data signal

**Solution**: Added power supply voltage level warning to test 05

**Warning Content**:
- WS2812B expects 5V data signal (spec: ≥0.7×VDD)
- ESP32 outputs 3.3V (marginal on 5V supply)
- Solutions: 1N4001 diode trick or 74HCT245 level shifter

**Impact**: Medium - Improves LED reliability

---

## 7. Wiring & Configuration

### 7.1 Critical Wiring Requirements

#### Ground Connections (MOST IMPORTANT!)

**Common ground required between**:
- ESP32 GND ↔ BTT Rodent GND
- ESP32 GND ↔ Scale MAX3232 GND
- ESP32 GND ↔ RS485 MAX485 GND
- ESP32 GND ↔ 5V LED power supply GND

**Failure to connect grounds will cause**:
- Erratic communication
- Random data corruption
- Commands ignored
- System instability

#### Button Wiring

All buttons use active-LOW with internal pull-ups:

```
Button Pin → ESP32 GPIO
Other Pin → GND
```

**Pins**:
- START: GPIO 13 → GND
- MODE: GPIO 14 → GND
- STOP: GPIO 33 → GND

#### Encoder Wiring

```
CLK → GPIO 26 (internal pull-up)
DT  → GPIO 27 (internal pull-up)
SW  → GPIO 34 (NEEDS EXTERNAL 10kΩ pull-up to 3.3V!)
GND → GND
+   → 3.3V
```

**CRITICAL**: GPIO 34 is input-only and has no internal pull-up. You MUST add external 10kΩ resistor from SW to 3.3V!

#### UART to BTT Rodent (Direct Connection)

**For distances < 1 meter**:

```
ESP32 GPIO 17 (TX) → BTT Rodent GPIO 14 (RX)
ESP32 GPIO 16 (RX) → BTT Rodent GPIO 15 (TX)
ESP32 GND         → BTT Rodent GND (CRITICAL!)
```

**Configuration Required**: Load `btt_rodent_uart.yaml` to Rodent

#### RS485 to BTT Rodent (Long Distance)

**For distances > 1 meter or noisy environments**:

```
ESP32 Side:
  GPIO 17 (TX)  → MAX485 DI
  GPIO 16 (RX)  → MAX485 RO
  GPIO 4 (RTS)  → MAX485 DE/RE (tied together)
  GND          → MAX485 GND
  3.3V         → MAX485 VCC

MAX485 A → Twisted Pair → Rodent RS485 A
MAX485 B → Twisted Pair → Rodent RS485 B

Termination: 120Ω resistor across A-B at BOTH ends
```

**Cable**: Belden 9723 or similar twisted pair recommended

#### Scale Connection (RS232)

**Level Converter Required** (RS232 is ±12V):

```
ESP32 Side:
  GPIO 32 (TX) → MAX3232 T1IN
  GPIO 35 (RX) → MAX3232 R1OUT (GPIO 35 is input-only)
  GND         → MAX3232 GND
  3.3V        → MAX3232 VCC

MAX3232 T1OUT → Scale RX (via DB9)
MAX3232 R1IN  → Scale TX (via DB9)
GND          → Scale GND (pin 5 on DB9)
```

**DB9 Pinout**:
- Pin 2: RX (← T1OUT)
- Pin 3: TX (→ R1IN)
- Pin 5: GND

#### I2C LCD Display

```
LCD SDA → GPIO 21
LCD SCL → GPIO 22
LCD VCC → 5V (or 3.3V depending on module)
LCD GND → GND
```

**Address**: Usually 0x27, sometimes 0x3F (use test_03 to scan)

**Long Runs**: Add 4.7kΩ pull-up resistors on SDA and SCL if > 30cm

#### WS2812B LED Strips

```
LED Data → GPIO 25
LED 5V   → 5V power supply (1.5A+ recommended)
LED GND  → Common ground (to ESP32 GND and 5V supply GND)
```

**Level Shifting** (recommended):
- **Option 1**: 1N4001 diode from LED 5V to LED Data line (pulls data high)
- **Option 2**: 74HCT245 buffer (proper 3.3V → 5V level shift)

**Power**: Do NOT power LEDs from ESP32 VIN! Use separate 5V supply.

### 7.2 FluidNC Configuration Files

**For UART Direct Connection**: `btt_rodent_uart.yaml`
```yaml
uart:
  rxd_pin: gpio.14
  txd_pin: gpio.15
  baud: 115200
```

**For RS485 Connection**: `btt_rodent_rs485.yaml`
```yaml
uart:
  rxd_pin: gpio.14
  txd_pin: gpio.15
  rts_pin: gpio.4
  baud: 115200
  mode: rs485
```

**Motor Configuration** (example for X-axis):
```yaml
axes:
  x:
    steps_per_mm: 80.0
    max_rate_mm_per_min: 5000
    acceleration_mm_per_sec2: 200
    max_travel_mm: 100
    homing:
      cycle: 0
      mpos_mm: 0
```

### 7.3 PlatformIO Configuration

**platformio.ini** (example for test_19):
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
build_src_filter =
    +<test_19_full_integration.cpp>
    -<*.cpp>
lib_deps =
    fastled/FastLED@^3.5.0
    marcoschwartz/LiquidCrystal_I2C@^1.1.4
```

**Key Settings**:
- `monitor_speed = 115200`: Match Serial.begin() baud rate
- `build_src_filter`: Select which test to compile
- `lib_deps`: Required libraries (FastLED, LiquidCrystal_I2C)

---

## 8. Production-Ready Configuration

### 8.1 Recommended Final System

For production deployment, use **Test 19** as the foundation with these enhancements:

#### Hardware Configuration
- ESP32 Dev Module
- BTT Rodent V1.1 with FluidNC (UART direct connection)
- 4 Peristaltic pumps with calibrated ML_PER_MM values
- Digital scale with RS232 output (optional but recommended)
- 16x2 I2C LCD display (0x27)
- 32 WS2812B RGB LEDs (4 strips × 8 LEDs)
- 3 push buttons (START, MODE, STOP)
- Rotary encoder with button
- Proper power supplies (3.3V for ESP32, 5V for LEDs, 24V for Rodent)

#### Software Features
- Recipe management system (Test 16 code)
- Data logging with statistics (Test 18 code)
- Safety features with emergency stop (Test 17 code)
- LED visual feedback (Test 20 motor status code)
- Weight-based dispensing option (Test 15 code)
- Full LCD user interface (Test 19 base)

#### Calibration Steps

1. **Determine ML_PER_MM for each pump**:
   ```
   Method:
   1. Dispense 100mm of movement
   2. Measure actual volume dispensed
   3. ML_PER_MM = measured_ml / 100
   ```

2. **Set safe feedrate limits**:
   ```cpp
   // Conservative starting points:
   MAX_FEEDRATE_X = 300;  // mm/min
   MAX_FEEDRATE_Y = 300;
   MAX_FEEDRATE_Z = 300;
   MAX_FEEDRATE_A = 300;
   ```

3. **Verify encoder direction**:
   - Clockwise should increment
   - Counter-clockwise should decrement
   - Swap CLK/DT if reversed

4. **Test scale communication**:
   - Use Test 06 to verify scale responses
   - Run automatic timing optimization
   - Note optimal timing values
   - Apply to Test 15/19

#### Production Checklist

**Before Deployment**:
- [ ] All grounds properly connected
- [ ] Power supplies adequate and stable
- [ ] LED level shifting implemented (if needed)
- [ ] Encoder external pull-up on GPIO 34
- [ ] Scale MAX3232 wired correctly
- [ ] FluidNC configuration uploaded to Rodent
- [ ] All pumps calibrated (ML_PER_MM)
- [ ] Emergency stop tested and verified
- [ ] Recipes programmed and tested
- [ ] Data logging verified
- [ ] LCD displaying correctly
- [ ] LED colors assigned per pump
- [ ] Safety timeout values appropriate
- [ ] Serial output clean (no spam)

**Testing Protocol**:
1. Run Test 00 (verify ESP32 basics)
2. Run Test 01 (verify all buttons)
3. Run Test 02 (verify encoder + external pull-up)
4. Run Test 03 (scan I2C, find LCD)
5. Run Test 04 (verify LCD display)
6. Run Test 05 (verify LED strips, check WiFi/BT disabled)
7. Run Test 08 (verify UART to Rodent)
8. Run Test 06 (verify scale if used)
9. Run Test 12 (calibrate single pump)
10. Run Test 17 (verify all safety features)
11. Run Test 19 (full integration test)
12. Run production firmware with all features

### 8.2 Key Configuration Constants

```cpp
// Pin Definitions
#define START_BUTTON_PIN 13
#define MODE_BUTTON_PIN 14
#define STOP_BUTTON_PIN 33
#define ENCODER_CLK_PIN 26
#define ENCODER_DT_PIN 27
#define ENCODER_SW_PIN 34  // Needs external pull-up!

#define LED_PIN 25
#define NUM_LEDS 32
#define LEDS_PER_STRIP 8

#define SDA_PIN 21
#define SCL_PIN 22
#define LCD_ADDRESS 0x27  // Or 0x3F

// UART Pins
#define RODENT_TX_PIN 17
#define RODENT_RX_PIN 16
#define SCALE_TX_PIN 32
#define SCALE_RX_PIN 35

// Pump Calibration (MUST BE CALIBRATED PER PUMP!)
#define ML_PER_MM_X 0.05  // Pump 1 (DMDEE)
#define ML_PER_MM_Y 0.05  // Pump 2 (T-12)
#define ML_PER_MM_Z 0.05  // Pump 3 (T-9)
#define ML_PER_MM_A 0.05  // Pump 4 (L25B)

// Safety Limits
#define SAFE_MAX_FEEDRATE 300  // mm/min
#define COMMAND_TIMEOUT 30000  // 30 seconds
#define HEARTBEAT_TIMEOUT 5000  // 5 seconds
#define RESPONSE_TIMEOUT 2000   // 2 seconds

// LED Colors (GRB format)
#define COLOR_PUMP_X 0x00FFFF  // Cyan
#define COLOR_PUMP_Y 0xFF00FF  // Magenta
#define COLOR_PUMP_Z 0xFFFF00  // Yellow
#define COLOR_PUMP_A 0xFFFFFF  // White

#define COLOR_IDLE 0x00FF00    // Green
#define COLOR_RUNNING 0x0000FF // Blue
#define COLOR_ERROR 0xFF0000   // Red
#define COLOR_WARNING 0xFFFF00 // Yellow
```

### 8.3 Final Feature Integration

**Combining Multiple Test Features**:

To create the ultimate production system, integrate:

1. **Base from Test 19**: Full integration framework
2. **Add from Test 16**: Enhanced recipe management
3. **Add from Test 18**: Data logging and statistics
4. **Add from Test 20**: Real-time motor status LEDs
5. **Add from Test 15**: Weight-based dispensing option

**Example Combined Structure**:
```cpp
void setup() {
  // Initialize all peripherals (from Test 19)
  initSerial();
  initButtons();
  initEncoder();
  initI2C();
  initLCD();
  initLEDs();  // With WiFi/BT disable (from bug fix)
  initUART();
  initScale();  // Optional

  // Initialize subsystems
  initRecipeSystem();  // From Test 16
  initDataLogging();   // From Test 18
  initSafety();        // From Test 17
}

void loop() {
  // Main state machine (from Test 19)
  updateEncoder();
  updateButtons();
  updateSafety();      // Heartbeat, timeouts
  updateDataLog();     // Background logging
  updateMotorStatus(); // LED motor visualization

  switch (systemState) {
    case IDLE:
      handleIdleMode();
      break;
    case SELECT:
      handleRecipeSelection();
      break;
    case RUNNING:
      handleRecipeExecution();
      updateMotorStatusLEDs();  // Real-time feedback
      break;
    case WEIGHT_MODE:
      handleWeightDispensing();  // Optional scale mode
      break;
    case COMPLETE:
      handleCompletion();
      break;
    case ERROR:
      handleError();
      break;
  }
}
```

### 8.4 Maintenance & Troubleshooting

**Common Issues**:

| Symptom | Likely Cause | Solution |
|---------|--------------|----------|
| LEDs show random colors | WiFi/BT interference | Verify WiFi.mode(WIFI_OFF) before FastLED.addLeds() |
| Scale not responding | Wrong command format | Use "@P<CR><LF>" literal text, not "\r\n" |
| Encoder not responding | No external pull-up | Add 10kΩ resistor from GPIO 34 to 3.3V |
| UART commands ignored | No common ground | Connect ESP32 GND to Rodent GND |
| LCD not found | Wrong I2C address | Run Test 03 to scan and find actual address |
| Motors stutter | Feedrate too high | Reduce to ≤300 mm/min |
| Data logging floods output | Verbose mode on | Type 'v' to disable verbose logging |
| Emergency stop doesn't work | Wrong response sent | Send "!" character, not word "stop" |

**Debugging Commands**:

```
Serial Commands (Test 18 enhanced):
  l - Show command log
  s - Show statistics
  c - Clear log
  v - Toggle verbose mode
  ? - Query status

FluidNC Commands:
  $I - System info
  $$ - List all settings
  ? - Status query
  $H - Home all axes
  ! - Emergency stop (Feed Hold)
  ~ - Resume from hold
  Ctrl-X - Soft reset
  $X - Unlock after alarm
```

---

## Conclusion

This comprehensive documentation covers all aspects of the peristaltic pump control system developed across 20 progressive test programs. The system demonstrates a complete progression from basic hardware verification through to a fully integrated production-ready control system with:

- Precise volume and flow rate control
- Multi-pump coordination (sequential and simultaneous)
- Closed-loop weight-based dispensing
- Comprehensive user interface (buttons, encoder, LCD, LEDs)
- Recipe management and execution
- Safety features and emergency stop
- Data logging and monitoring
- Real-time visual feedback

All critical bugs have been identified and fixed, with particular attention to:
- LED timing interference from WiFi/Bluetooth
- Scale communication protocol
- Data logging performance
- Encoder responsiveness
- UART configuration
- Safety limits

The final production configuration is based on Test 19 (Full Integration) enhanced with features from Tests 15-18 and 20, providing a robust and feature-complete system suitable for production deployment.

---

**Document Version**: 1.0
**Created**: 2025-11-18
**Repository**: https://github.com/maverick8969/Test_Programming
**Branch**: claude/document-skills-requirements-013oHCd7oMfnW2ZWzxXYNC1c
