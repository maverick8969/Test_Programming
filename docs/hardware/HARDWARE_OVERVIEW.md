# Hardware Overview

This document provides comprehensive hardware specifications, pinouts, and wiring information for the Peristaltic Pump Control System.

## Table of Contents

- [System Architecture](#system-architecture)
- [Component List](#component-list)
- [ESP32 Pin Assignments](#esp32-pin-assignments)
- [Wiring Diagrams](#wiring-diagrams)
- [Power Requirements](#power-requirements)
- [Component Details](#component-details)

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        ESP32 Main Controller                 │
│                                                               │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐    │
│  │  UART1   │  │  UART2   │  │   I2C    │  │   RMT    │    │
│  │  RS485   │  │  Scale   │  │   LCD    │  │   LED    │    │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘    │
│       │             │             │             │            │
│  ┌────┴─────┐  ┌────┴─────┐  ┌────┴─────┐  ┌────┴─────┐    │
│  │  GPIO    │  │  GPIO    │  │  GPIO    │  │  GPIO    │    │
│  │ 2,4,15   │  │  16,17   │  │  21,22   │  │    25    │    │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘    │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │       Input Buttons & Encoder (GPIO 12-14,26-27,33) │   │
│  └──────────────────────────────────────────────────────┘   │
└───────────────────────┬───────────────────────────────────┬─┘
                        │                                   │
            ┌───────────▼──────────┐          ┌────────────▼────────┐
            │  BTT Rodent V1.1     │          │   WS2812B LEDs      │
            │  Motor Controller    │          │   4 strips x 8 LEDs │
            │  (FluidNC)           │          │   Total: 32 LEDs    │
            └───────────┬──────────┘          └─────────────────────┘
                        │
        ┌───────┬───────┼───────┬───────┐
        │       │       │       │       │
    ┌───▼───┐ ┌─▼─────┐┌──▼────┐ ┌─────▼┐
    │Pump 1 │ │Pump 2 ││Pump 3 │ │Pump 4│
    │(DMDEE)│ │(T-12) ││(T-9)  │ │(L25B)│
    │ X-axis│ │Y-axis ││Z-axis │ │A-axis│
    └───────┘ └───────┘└───────┘ └──────┘
```

---

## Component List

### Main Components

| Component | Model/Type | Quantity | Purpose |
|-----------|------------|----------|---------|
| Microcontroller | ESP32-WROOM-32 Dev Board | 1 | Main controller |
| Motor Controller | BTT Rodent V1.1 (FluidNC) | 1 | Stepper motor control |
| Stepper Motors | NEMA 17 (or similar) | 4 | Pump actuation |
| Digital Scale | uxilaii exc20250700830 | 1 | Weight monitoring |
| LED Strips | WS2812B RGB | 4 | Visual feedback (8 LEDs each) |
| LCD Display | 1602 I2C (16x2) | 1 | Status display |
| Rotary Encoder | Standard rotary encoder | 1 | Menu navigation + SELECT button |
| Pushbuttons | Momentary switches | 3 | START, STOP, MODE |
| **RS485 Transceiver** | **MAX485 Module** | **1** | **TTL to RS485 conversion (Rodent)** |
| **RS232 Transceiver** | **MAX3232 Module** | **1** | **TTL to RS232 conversion (Scale)** |

### Additional Components

| Component | Specification | Quantity |
|-----------|--------------|----------|
| Power Supply | 12V or 24V DC | 1 |
| Buck Converter | 24V to 5V, 3A | 1 |
| Pull-up Resistors | Built into ESP32 | N/A |
| Wiring | 22-24 AWG | As needed |

---

## ESP32 Pin Assignments

### Complete Pin Map

| GPIO | Alt Label | Function | Direction | Type | Component |
|------|-----------|----------|-----------|------|-----------|
| 2 | - | MAX485 DI (Data In) | Output | Serial | RS485 Transceiver |
| 4 | - | MAX485 RO (Receive Out) | Input | Serial | RS485 Transceiver |
| 12 | - | SELECT Button (Encoder SW) | Input | Digital | Rotary Encoder / Control Input |
| 13 | - | START Button | Input | Digital | Control Input |
| 14 | - | MODE Button | Input | Digital | Control Input |
| 15 | - | RS485 DE/RE (Direction) | Output | Digital | RS485 Transceiver |
| 16 | - | MAX3232 R1OUT (TTL RX) | Input | Serial | RS232 Transceiver |
| 17 | - | MAX3232 T1IN (TTL TX) | Output | Serial | RS232 Transceiver |
| 21 | - | LCD I2C SDA | Bidirectional | I2C Data | LCD Display |
| 22 | - | LCD I2C SCL | Output | I2C Clock | LCD Display |
| 25 | - | LED Data Line | Output | RMT/PWM | WS2812B LEDs |
| 26 | - | Encoder CLK | Input | Digital | Rotary Encoder |
| 27 | - | Encoder DT | Input | Digital | Rotary Encoder |
| 33 | - | STOP Button | Input | Digital | Control Input |

**Total GPIO Used**: 13 pins
**Available GPIO**: 19+ pins still available for expansion

### Pin Notes

- **All button/encoder inputs**: Use GPIOs with internal pull-up resistors (no external resistors needed)
- **SELECT button consolidated**: The rotary encoder's built-in switch serves as SELECT button
- **GPIO 2, 4, 15**: Connected to MAX485 module for RS485 communication with Rodent board
- **GPIO 16, 17**: Connected to MAX3232 module for RS232 communication with digital scale
- **GPIO 25**: Uses RMT peripheral for WS2812B timing-critical protocol
- **GPIO 12, 13, 14, 26, 27, 33**: All have internal pull-up resistors enabled

---

## Wiring Diagrams

### 1. ESP32 to BTT Rodent V1.1 (RS485 Communication)

**Two module types are supported:**

#### Option A: Auto-Direction RS485 Module (RECOMMENDED - Simpler!)

```
┌─────────────────┐       ┌──────────────┐       ┌──────────────────┐
│     ESP32       │       │  RS485 Auto  │       │  BTT Rodent V1.1 │
│                 │       │  Module      │       │  Motor Control   │
│                 │       │  (6 pins)    │       │                  │
│ GPIO 2 (TX) ────┼──────→│ DI (or TXD)  │       │                  │
│ GPIO 4 (RX) ────┼←──────│ RO (or RXD)  │       │                  │
│                 │       │              │       │                  │
│ 3.3V or 5V ─────┼──────→│ VCC          │       │                  │
│ GND ────────────┼──────→│ GND ─────────┼──────→│ GND             │
│                 │       │              │       │                  │
│                 │       │ A ───────────┼──────→│ RS485 A         │
│                 │       │ B ───────────┼──────→│ RS485 B         │
│                 │       │              │       │                  │
│                 │       │              │       │ ┌──────────────┐ │
│                 │       │              │       │ │Motor Outputs │ │
│                 │       │              │       │ │ X: Pump 1    │ │
│                 │       │              │       │ │ Y: Pump 2    │ │
│                 │       │              │       │ │ Z: Pump 3    │ │
│                 │       │              │       │ │ A: Pump 4    │ │
│                 │       │              │       │ └──────────────┘ │
└─────────────────┘       └──────────────┘       └──────────────────┘

Auto-Direction Module Pin Connections:
┌──────────┬─────────────────┬──────────────────────────┐
│ Module   │ ESP32           │ Function                 │
├──────────┼─────────────────┼──────────────────────────┤
│ DI/TXD   │ GPIO 2 (TX)     │ Data Input (TTL to RS485)│
│ RO/RXD   │ GPIO 4 (RX)     │ Receive Output (RS485→TTL)│
│ VCC      │ 3.3V or 5V      │ Power (check module spec)│
│ GND      │ GND             │ Ground                   │
│ A        │ → Rodent RS485 A│ RS485 Differential A     │
│ B        │ → Rodent RS485 B│ RS485 Differential B     │
└──────────┴─────────────────┴──────────────────────────┘

✅ Advantages: Simpler wiring, no direction control needed, GPIO 15 free for other uses
```

#### Option B: Manual Direction RS485 Module (MAX485/Similar)

```
┌─────────────────┐       ┌──────────────┐       ┌──────────────────┐
│     ESP32       │       │  MAX485      │       │  BTT Rodent V1.1 │
│                 │       │  RS485       │       │  Motor Control   │
│                 │       │  (7-8 pins)  │       │                  │
│ GPIO 2 (TX) ────┼──────→│ DI           │       │                  │
│ GPIO 4 (RX) ────┼←──────│ RO           │       │                  │
│ GPIO 15 (RTS)───┼──────→│ DE/RE or DIR │       │                  │
│                 │       │              │       │                  │
│ 3.3V ───────────┼──────→│ VCC          │       │                  │
│ GND ────────────┼──────→│ GND ─────────┼──────→│ GND             │
│                 │       │              │       │                  │
│                 │       │ A ───────────┼──────→│ RS485 A         │
│                 │       │ B ───────────┼──────→│ RS485 B         │
└─────────────────┘       └──────────────┘       └──────────────────┘

Manual Direction Module Pin Connections:
┌──────────┬─────────────────┬──────────────────────────┐
│ Module   │ ESP32           │ Function                 │
├──────────┼─────────────────┼──────────────────────────┤
│ DI       │ GPIO 2 (TX)     │ Data Input (TTL to RS485)│
│ RO       │ GPIO 4 (RX)     │ Receive Output (RS485→TTL)│
│ DE       │ GPIO 15 (RTS)   │ Driver Enable (TX mode)  │
│ RE       │ GPIO 15 (RTS)   │ Receiver Enable (RX mode)│
│ VCC      │ 3.3V or 5V      │ Power (check module spec)│
│ GND      │ GND             │ Ground                   │
│ A        │ → Rodent RS485 A│ RS485 Differential A     │
│ B        │ → Rodent RS485 B│ RS485 Differential B     │
└──────────┴─────────────────┴──────────────────────────┘

Note: If module has separate DE and RE pins, tie them together to GPIO 15
```

**Common Configuration (both module types):**
- UART1: 115200 baud, 8N1
- RS485: Half-duplex differential signaling
- Use twisted pair cable for A/B lines (reduces noise)
- Common ground is essential

**Firmware Configuration:**
- Edit `include/config.h`
- For auto-direction: `#define RS485_AUTO_DIRECTION` (default)
- For manual direction: Comment out RS485_AUTO_DIRECTION and uncomment `RODENT_RTS_PIN`

**Important Notes:**
⚠️ RS485 transceiver module is REQUIRED - ESP32 cannot directly connect to RS485
⚠️ Ensure common ground between ESP32, RS485 module, and Rodent board
⚠️ Use twisted pair cable for RS485 A/B lines (reduces noise)
⚠️ Keep RS485 cable length under 1200m (typically <10m for this application)
```

### 2. ESP32 to Digital Scale (RS232 via MAX3232)

```
┌─────────────────┐       ┌──────────────┐       ┌──────────────────┐
│     ESP32       │       │  MAX3232     │       │  Digital Scale   │
│                 │       │  RS232       │       │  uxilaii         │
│                 │       │  Transceiver │       │                  │
│ GPIO 17 (TX) ───┼──────→│ T1IN         │       │                  │
│ GPIO 16 (RX) ───┼←──────│ R1OUT        │       │                  │
│                 │       │              │       │                  │
│ 3.3V or 5V ─────┼──────→│ VCC          │       │                  │
│ GND ────────────┼──────→│ GND ─────────┼──────→│ GND             │
│                 │       │              │       │                  │
│                 │       │ T1OUT ───────┼──────→│ RX (if needed)  │
│                 │       │ R1IN  ←──────┼───────│ TX (scale out)  │
│                 │       │              │       │                  │
└─────────────────┘       └──────────────┘       └──────────────────┘

MAX3232 Pin Connections:
┌──────────┬─────────────────┬──────────────────────────┐
│ MAX3232  │ ESP32           │ Function                 │
├──────────┼─────────────────┼──────────────────────────┤
│ T1IN     │ GPIO 17 (TX)    │ TTL input from ESP32     │
│ R1OUT    │ GPIO 16 (RX)    │ TTL output to ESP32      │
│ VCC      │ 3.3V or 5V      │ Power (check module)     │
│ GND      │ GND             │ Ground                   │
│ T1OUT    │ → Scale RX      │ RS232 TX (±12V) optional │
│ R1IN     │ ← Scale TX      │ RS232 RX (±12V)          │
└──────────┴─────────────────┴──────────────────────────┘

Configuration:
- UART2: 9600 baud, 8N1
- Format: "25.34 g\r\n"
- Protocol: RS232 (±12V levels converted to 3.3V TTL)

Important Notes:
⚠️ MAX3232 module is REQUIRED - RS232 uses ±12V which would damage ESP32
⚠️ Some scales are TX-only (no RX input needed)
⚠️ Check if your module needs 3.3V or 5V power
⚠️ MAX3232 modules often include required capacitors
⚠️ Ensure common ground between ESP32, MAX3232, and scale
```

### 3. ESP32 to 1602 LCD (I2C)

```
┌─────────────────────┐                    ┌──────────────────────┐
│      ESP32          │                    │   1602 LCD Display   │
│                     │                    │   (I2C Interface)    │
│                     │                    │                      │
│  GPIO 21 (SDA) ─────┼────────────────────┼─→ SDA               │
│  GPIO 22 (SCL) ─────┼────────────────────┼─→ SCL               │
│                     │                    │                      │
│  VCC (3.3V) ────────┼────────────────────┼─→ VCC               │
│  GND ───────────────┼────────────────────┼─→ GND               │
│                     │                    │                      │
└─────────────────────┘                    └──────────────────────┘

Configuration:
- I2C Address: 0x27
- Display: 16 columns x 2 rows
- Pull-up resistors: Typically included on I2C module
```

### 4. ESP32 to WS2812B LED Strips

```
┌─────────────────────┐                    ┌──────────────────────┐
│      ESP32          │                    │   WS2812B LED Strips │
│                     │                    │   (4 strips)         │
│                     │                    │                      │
│  GPIO 25 (Data) ────┼────────────────────┼─→ DIN (Data In)     │
│                     │                    │                      │
│  VCC (5V) ──────────┼────────────────────┼─→ VCC (5V)          │
│  GND ───────────────┼────────────────────┼─→ GND               │
│                     │                    │                      │
│                     │                    │  ┌─────────────────┐ │
│                     │                    │  │ Strip 1: 8 LEDs │ │
│                     │                    │  │ Strip 2: 8 LEDs │ │
│                     │                    │  │ Strip 3: 8 LEDs │ │
│                     │                    │  │ Strip 4: 8 LEDs │ │
│                     │                    │  │ Total: 32 LEDs  │ │
│                     │                    │  └─────────────────┘ │
└─────────────────────┘                    └──────────────────────┘

Configuration:
- Protocol: WS2812B (GRB format)
- Data Rate: 800 kHz
- LEDs per strip: 8
- Strips connected in series
- Driver: ESP32 RMT peripheral

LED Strip Assignment:
┌────────┬───────┬────────────┐
│ Strip  │ Pump  │ Color      │
├────────┼───────┼────────────┤
│ 1      │ DMDEE │ Cyan       │
│ 2      │ T-12  │ Magenta    │
│ 3      │ T-9   │ Yellow     │
│ 4      │ L25B  │ White      │
└────────┴───────┴────────────┘
```

### 5. ESP32 to Control Buttons

```
┌─────────────────────┐
│      ESP32          │                    Button Connections
│                     │                    (Active LOW with pull-up)
│                     │
│  GPIO 13 (START) ───┼───┐                ┌─── VCC (Internal)
│                     │   └─────[Button]───┤
│                     │                    └─── GND
│                     │
│  GPIO 33 (STOP) ────┼───┐                ┌─── VCC (Internal)
│                     │   └─────[Button]───┤
│                     │                    └─── GND
│                     │
│  GPIO 14 (MODE) ────┼───┐                ┌─── VCC (Internal)
│                     │   └─────[Button]───┤
│                     │                    └─── GND
└─────────────────────┘

Configuration:
- All buttons: Normally Open (NO)
- Pull-up: Internal ESP32 pull-up enabled (fully supported)
- Active: LOW (pressed = GND, released = HIGH)
- SELECT function: See rotary encoder section (encoder button)
```

### 6. ESP32 to Rotary Encoder

```
┌─────────────────────┐                    ┌──────────────────────┐
│      ESP32          │                    │   Rotary Encoder     │
│                     │                    │                      │
│  GPIO 26 (CLK) ─────┼────────────────────┼─→ CLK (A)           │
│  GPIO 27 (DT) ──────┼────────────────────┼─→ DT (B)            │
│  GPIO 12 (SW) ──────┼────────────────────┼─→ SW (Button/SELECT)│
│                     │                    │                      │
│  GND ───────────────┼────────────────────┼─→ GND               │
│  VCC (3.3V) ────────┼────────────────────┼─→ VCC (if needed)   │
│                     │                    │                      │
└─────────────────────┘                    └──────────────────────┘

Configuration:
- Type: Incremental rotary encoder
- Pull-up: Internal ESP32 pull-up enabled (fully supported)
- Detents: Standard (depends on encoder model)
- Button: Active LOW, serves dual purpose as SELECT button
- Note: Encoder button replaces separate SELECT pushbutton

### 7. Complete System Wiring Diagram

```
                                  ┌──────────────┐
                                  │  Power Supply│
                                  │  12V or 24V  │
                                  └──────┬───────┘
                                         │
                         ┌───────────────┼────────────────┐
                         │               │                │
                         ▼               ▼                ▼
                  ┌──────────┐    ┌──────────┐    ┌──────────┐
                  │  Rodent  │    │  ESP32   │    │ WS2812B  │
                  │  Board   │    │  (5V/3V) │    │  (5V)    │
                  │  (12/24V)│    │          │    │          │
                  └────┬─────┘    └────┬─────┘    └──────────┘
                       │               │
          ┌────────────┼───────────────┼────────────┐
          │            │               │            │
      ┌───▼───┐    ┌───▼───┐      ┌───▼───┐    ┌───▼───┐
      │Pump 1 │    │Pump 2 │      │Pump 3 │    │Pump 4 │
      │NEMA 17│    │NEMA 17│      │NEMA 17│    │NEMA 17│
      └───────┘    └───────┘      └───────┘    └───────┘


          ┌─────────────────────────────────┐
          │        Control Panel            │
          │                                 │
          │  [START]  [STOP]  [MODE]       │
          │  (ENCODER with SELECT button)  │
          │                                 │
          │  ┌──────────────────┐          │
          │  │  1602 LCD        │          │
          │  │  16x2 Display    │          │
          │  └──────────────────┘          │
          └─────────────┬───────────────────┘
                        │
                        │ (I2C, Buttons, Encoder)
                        │
                        ▼
                 ┌──────────┐
                 │  ESP32   │
                 └──────────┘
```

---

## Power Requirements

### Power Distribution

| Component | Voltage | Current (Est.) | Notes |
|-----------|---------|----------------|-------|
| ESP32 | 5V (USB) or 3.3V | 500mA peak | Can be powered via USB or external 5V |
| BTT Rodent Board | 12-24V DC | 5-10A | Depends on motor load |
| Stepper Motors (4x) | 12-24V DC | 1-2A each | Via Rodent board |
| WS2812B LEDs (32) | 5V DC | ~2A max | At full brightness (60mA/LED) |
| LCD Display | 5V DC | 50mA | Via I2C module |
| Digital Scale | 9V battery or USB | 100mA | Self-powered or USB |

### Power Supply Recommendations

**Option 1: Single Power Supply**
- 24V DC, 10A power supply
  - Main power for Rodent board and motors
  - Use DC-DC buck converter for 5V (5A) → ESP32 + LEDs
  - Use DC-DC buck converter for 3.3V (1A) → LCD (if needed)

**Option 2: Dual Power Supply**
- 24V DC, 10A → Rodent board + motors
- 5V DC, 3A → ESP32, LEDs, LCD

### Power Connections

```
┌──────────────────┐
│  24V Power Supply│
│      10A         │
└────────┬─────────┘
         │
    ┌────┴────┐
    │         │
    ▼         ▼
┌────────┐  ┌──────────────┐
│ Rodent │  │ Buck Converter│
│ Board  │  │  24V → 5V     │
│        │  │  (3A min)     │
└────────┘  └───────┬───────┘
                    │
            ┌───────┴────────┐
            │                │
            ▼                ▼
        ┌──────┐      ┌──────────┐
        │ESP32 │      │ WS2812B  │
        │ VIN  │      │   LEDs   │
        └──────┘      └──────────┘
```

---

## Component Details

### 1. MAX485 RS485 Transceiver Module

**Features:**
- TTL to RS485 differential signal conversion
- Half-duplex communication
- 3.3V or 5V operation
- Built-in direction control (DE/RE pins)

**Pinout:**
- **DI (Data In)**: TTL input from ESP32 TX (GPIO 2)
- **RO (Receive Out)**: TTL output to ESP32 RX (GPIO 4)
- **DE (Driver Enable)**: HIGH to transmit, tied to RE
- **RE (Receiver Enable)**: LOW to receive, tied to DE
- **VCC**: 3.3V or 5V power
- **GND**: Ground
- **A**: RS485 differential A (connect to Rodent RS485 A)
- **B**: RS485 differential B (connect to Rodent RS485 B)

**Critical Notes:**
- DE and RE pins must be tied together and connected to GPIO 15
- Firmware automatically controls direction (HIGH=TX, LOW=RX)
- Use twisted pair cable for A/B connections
- Common ground between ESP32, MAX485, and Rodent is essential

**Module Selection:**
- MAX485, MAX3485, or SP3485 all work
- Pre-built modules (~$2) are easiest
- Some modules have DE/RE already tied together internally

### 2. BTT Rodent V1.1 Motor Controller

**Features:**
- FluidNC firmware compatible
- 4 independent stepper motor drivers
- RS485 communication interface
- G-code command support
- Built-in USB for configuration

**Connections:**
- **RS485 A**: Connect to MAX485 A terminal
- **RS485 B**: Connect to MAX485 B terminal
- **GND**: Connect to common ground
- **Motor Outputs**: X, Y, Z, A (4 pumps)
- **Power**: 12-24V DC input

**Configuration:**
See [FluidNC Setup Guide](../setup/FLUIDNC_SETUP_GUIDE.md) for complete setup instructions.

### 3. MAX3232 RS232 Transceiver Module

**Features:**
- TTL (3.3V/5V) to RS232 (±12V) level conversion
- Bidirectional communication
- Built-in charge pump (no external power supply needed for RS232 levels)
- Includes required capacitors on most modules

**Pinout (typical module):**
- **T1IN**: TTL input from ESP32 TX (GPIO 17)
- **R1OUT**: TTL output to ESP32 RX (GPIO 16)
- **T1OUT**: RS232 output to Scale RX (±12V) - may not be needed
- **R1IN**: RS232 input from Scale TX (±12V)
- **VCC**: 3.3V or 5V power (check your module specification)
- **GND**: Ground

**Critical Notes:**
- **NEVER connect RS232 (±12V) directly to ESP32** - will damage GPIO pins
- Most scales are transmit-only (you may only need R1IN connection)
- Some modules require 5V, others work with 3.3V - check your module datasheet
- Modules typically include 4-5 capacitors (0.1µF) for charge pump
- DB9 connector on scale side may be male or female

**Common RS232 Connectors:**
- **DB9 Female** (on scale cable): Pin 2 = TX, Pin 3 = RX, Pin 5 = GND
- **DB9 Male** (on scale): Pin 3 = TX, Pin 2 = RX, Pin 5 = GND
- Some scales use 3.5mm jack or other connectors - check manual

**Module Selection:**
- MAX3232 - most common, works with 3.3V or 5V
- MAX232 - older, requires 5V and external capacitors
- SP3232 - pin-compatible with MAX3232
- Pre-built "RS232 to TTL" modules (~$2-5) are easiest

### 4. Digital Scale (uxilaii exc20250700830)

**Specifications:**
- **Interface**: RS232 Serial (±12V levels)
- **Baud Rate**: 9600
- **Data Format**: 8N1
- **Output Format**: `"25.34 g\r\n"` (weight, space, unit, CR, LF)
- **Modes**: Manual print or Continuous
- **Connector**: Typically DB9 or proprietary cable

**Connections (via MAX3232 module):**
- **Scale TX (RS232)**: Connect to MAX3232 R1IN
- **Scale RX (RS232)**: Connect to MAX3232 T1OUT (if scale supports input)
- **Scale GND**: Connect to MAX3232 GND
- **MAX3232 TTL side**: Connects to ESP32 GPIO 16/17 (see MAX3232 section)

**Important:**
⚠️ **Do NOT connect scale RS232 directly to ESP32!** Use MAX3232 converter module.

**Configuration:**
See [Scale Setup Guide](../setup/SCALE_SETUP_GUIDE.md) for complete setup instructions.

### 4. WS2812B LED Strips

**Specifications:**
- **Type**: Addressable RGB LEDs
- **Voltage**: 5V DC
- **Protocol**: WS2812B (800kHz, GRB format)
- **Configuration**: 4 strips × 8 LEDs = 32 total LEDs
- **Current**: ~60mA per LED at full white brightness

**Connections:**
- **DIN**: Connect to ESP32 GPIO 25
- **VCC**: Connect to 5V power supply
- **GND**: Connect to common ground

**Wiring Order** (strips in series):
```
ESP32 GPIO25 → Strip 1 (Pump 1) → Strip 2 (Pump 2) → Strip 3 (Pump 3) → Strip 4 (Pump 4)
               DOUT ──────────→ DIN    DOUT ──────→ DIN    DOUT ──────→ DIN
```

**LED Animations:**
- Boot sequence, Idle (pulsing), Priming (flashing)
- Active dosing (solid color), Paused (pulsing yellow)
- Complete (solid green), Error (flashing red)

### 5. 1602 LCD Display

**Specifications:**
- **Type**: Character LCD, 16 columns × 2 rows
- **Interface**: I2C (via PCF8574 or similar)
- **I2C Address**: 0x27 (default, may vary)
- **Voltage**: 5V DC

**Connections:**
- **SDA**: Connect to ESP32 GPIO 21
- **SCL**: Connect to ESP32 GPIO 22
- **VCC**: Connect to 5V power
- **GND**: Connect to ground

**Note**: LCD implementation is currently marked as TODO in firmware.

### 6. Control Inputs

#### Pushbuttons (3x)
- **START** (GPIO 13): Initiates dosing operation
- **STOP** (GPIO 33): Emergency stop
- **MODE** (GPIO 14): Operating mode toggle

**Wiring**: Active LOW with internal pull-ups enabled (all GPIOs fully support pull-ups)

#### Rotary Encoder
- **CLK** (GPIO 26): Clock/rotation signal
- **DT** (GPIO 27): Data/direction signal
- **SW** (GPIO 12): Switch/button press (also serves as SELECT button)

**Wiring**: Internal pull-ups enabled (all GPIOs fully support pull-ups)

**Note**: The encoder's built-in pushbutton serves dual purpose: navigation and SELECT function. This eliminates the need for a separate SELECT button, reducing component count from 4 buttons to 3.

---

## Safety Considerations

### Electrical Safety
1. **Proper Grounding**: Ensure all components share common ground
2. **Voltage Isolation**: Keep high voltage (24V) separate from logic level (3.3V)
3. **Fusing**: Use appropriate fuses on power supplies
4. **Wire Gauge**: Use appropriate wire gauge for current draw

### Wiring Best Practices
1. **Shielded Cable**: Use shielded cable for RS485 communication
2. **Twisted Pair**: Use twisted pair for encoder signals
3. **Short LED Runs**: Keep LED data line as short as possible
4. **Power Distribution**: Use star grounding for power distribution
5. **Strain Relief**: Provide strain relief for all connections

### Interference Prevention
1. **Separation**: Keep motor power wires away from signal wires
2. **Filtering**: Add capacitors near power inputs for noise filtering
3. **Shielding**: Ground shields at one end only to prevent ground loops

---

## Troubleshooting

### Common Issues

| Issue | Possible Cause | Solution |
|-------|----------------|----------|
| Motors not responding | RS485 wiring | Check RTS direction control, verify TX/RX not swapped |
| Scale not reading | Baud rate mismatch | Verify scale set to 9600 baud, check TX/RX connections |
| LEDs not lighting | Power or data | Check 5V power, verify GPIO 25 connection, check LED strip order |
| LCD not detected | I2C address | Scan I2C bus, verify address (0x27 or 0x3F common) |
| Buttons not working | Pull-up config | Verify internal pull-ups enabled in firmware |
| Encoder erratic | Noise/debouncing | Add hardware debouncing capacitors (0.1µF) |

### Testing Procedures

1. **Power Test**: Verify all voltage rails before connecting ESP32
2. **Continuity Test**: Check all connections with multimeter
3. **I2C Scan**: Use I2C scanner sketch to find LCD address
4. **Serial Test**: Test scale output with serial monitor
5. **LED Test**: Use simple LED test pattern before full integration

---

## Expansion Possibilities

### Available GPIO Pins

The ESP32 has additional GPIO pins available for future expansion:

**Available Input/Output Pins:**
- GPIO 0, 5, 12, 13, 18, 19, 23, 26, 27

**Input-Only Pins:**
- GPIO 34, 35, 36, 39 (currently in use, listed for reference)

### Potential Additions

1. **Temperature Sensors**: DS18B20 or DHT22 (1-wire or digital)
2. **Additional Displays**: OLED display (I2C)
3. **SD Card**: Logging data to SD card (SPI)
4. **Ethernet**: W5500 module for wired networking (SPI)
5. **Additional Sensors**: Flow sensors, pressure sensors
6. **Relay Outputs**: Control external devices

---

## References

- [FluidNC Setup Guide](../setup/FLUIDNC_SETUP_GUIDE.md)
- [Scale Setup Guide](../setup/SCALE_SETUP_GUIDE.md)
- [LED Integration Guide](../integration/LED_INTEGRATION_GUIDE.md)
- [G-code Command Reference](../reference/GCODE_COMMAND_REFERENCE.md)
- ESP32 Pinout: https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
- WS2812B Datasheet: https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf

---

*Last Updated: 2025-11-03*
