# Wiring Guide

Complete step-by-step wiring instructions for the Peristaltic Pump Control System.

## Table of Contents

- [Safety First](#safety-first)
- [Tools Required](#tools-required)
- [Bill of Materials](#bill-of-materials)
- [Step-by-Step Assembly](#step-by-step-assembly)
- [Connection Verification](#connection-verification)
- [Testing Procedures](#testing-procedures)

---

## Safety First

### Before You Begin

⚠️ **IMPORTANT SAFETY WARNINGS:**

1. **Disconnect all power** before making connections
2. **Verify voltage levels** with multimeter before connecting ESP32
3. **Check polarity** of all power connections
4. **Use proper wire gauge** for current requirements
5. **Never hot-plug** motor connectors while powered
6. **Double-check all connections** before applying power

### Electrostatic Discharge (ESD) Protection

- Use ESD wrist strap when handling ESP32 and electronic components
- Work on ESD-safe mat if available
- Touch grounded metal object before handling components

---

## Tools Required

### Essential Tools
- Soldering iron and solder
- Wire strippers (22-24 AWG)
- Wire cutters
- Small screwdrivers (Phillips and flathead)
- Multimeter (essential for testing)
- Heat shrink tubing or electrical tape

### Recommended Tools
- Crimping tool for connectors
- Hot glue gun (strain relief)
- Label maker or labels
- Cable ties
- Third hand or PCB holder

---

## Bill of Materials

### Electronics Components

| Component | Specification | Quantity | Notes |
|-----------|---------------|----------|-------|
| ESP32 Dev Board | ESP32-WROOM-32 or similar | 1 | 30-pin version recommended |
| **MAX485 Module** | **RS485 Transceiver** | **1** | **Required for Rodent communication** |
| **MAX3232 Module** | **RS232 to TTL Converter** | **1** | **Required for scale communication** |
| BTT Rodent V1.1 | FluidNC-compatible | 1 | Or equivalent FluidNC board |
| NEMA 17 Stepper Motors | 1.8° step angle | 4 | With peristaltic pump heads |
| Digital Scale | RS232 serial output | 1 | 9600 baud, DB9 or similar connector |
| WS2812B LED Strip | 5V, 60 LEDs/meter | 4 strips | 8 LEDs per strip (32 total) |
| 1602 LCD Display | I2C interface, 0x27 address | 1 | 16x2 character display |
| Rotary Encoder | KY-040 or similar | 1 | With pushbutton (serves as SELECT) |
| Pushbuttons | 12mm momentary switches | 3 | Normally open (NO) - START, STOP, MODE |
| Power Supply | 24V DC, 10A | 1 | Or 12V depending on motors |
| Buck Converter | 24V to 5V, 3A+ | 1 | For ESP32 and LEDs |

### Connectors and Wiring

| Item | Specification | Quantity | Notes |
|------|---------------|----------|-------|
| Wire 22 AWG | Stranded, multiple colors | 10 meters | For signal connections |
| Wire 18 AWG | Stranded, red/black | 5 meters | For power connections |
| JST-XH Connectors | 2-pin, 3-pin, 4-pin sets | 10 sets | For removable connections |
| Dupont Connectors | 2.54mm pitch, F-F, M-F | 40 wires | For breadboard connections |
| Screw Terminals | 2-pin, 5mm pitch | 6 pieces | For power distribution |
| Heat Shrink Tubing | Assorted sizes | 1 set | For insulation |

### Optional Components

| Component | Purpose | Quantity |
|-----------|---------|----------|
| Capacitors (0.1µF) | Debouncing buttons/encoder | 7 |
| Capacitor (1000µF, 16V) | Power filtering | 1 |
| Resistors (10kΩ) | External pull-ups if needed | 7 |
| Perfboard or PCB | Permanent assembly | 1 |
| Project enclosure | Housing electronics | 1 |
| Panel mount connectors | Professional finish | As needed |

---

## Step-by-Step Assembly

### Step 1: Prepare the ESP32

**1.1 Test ESP32**
```
Action: Connect ESP32 to computer via USB
Verify: Blue LED lights up, computer recognizes device
Tool: USB cable, computer
```

**1.2 Label GPIO Pins**
```
Print and attach labels for:
- GPIO 2, 4, 15: MAX485 (DI, RO, DE/RE)
- GPIO 16, 17: Serial (Scale)
- GPIO 21, 22: I2C (LCD)
- GPIO 25: LED Data
- GPIO 13, 14, 33: Buttons (START, MODE, STOP)
- GPIO 12, 26, 27: Encoder (SW/SELECT, CLK, DT)

Note: All GPIOs selected have internal pull-up resistor support
```

---

### Step 2: Wire Control Buttons

**2.1 Button Connections (Active LOW)**

Each button connects between GPIO and GND:

```
Button Wiring (All 3 buttons follow same pattern):

Button Terminal 1 ──────→ ESP32 GPIO (with internal pull-up)
Button Terminal 2 ──────→ ESP32 GND

Button Assignments:
┌────────┬──────────────┬──────────────┐
│ Button │ GPIO         │ Wire Color   │
├────────┼──────────────┼──────────────┤
│ START  │ 13           │ Green        │
│ STOP   │ 33           │ Red          │
│ MODE   │ 14           │ Blue         │
└────────┴──────────────┴──────────────┘

⚠️ Note: SELECT function is provided by encoder button (see Step 3)
✅ All GPIOs have full internal pull-up resistor support (no external resistors needed)
```

**2.2 Assembly Steps**

1. Cut 3 pairs of wires (each pair ~20cm):
   - START: Green wire
   - STOP: Red wire
   - MODE: Blue wire
   - All need black wire for GND

2. Solder wires to button terminals:
   ```
   [Button]
     NO ──── Colored wire → GPIO
     C  ──── Black wire   → GND
   ```

3. Add heat shrink to solder joints

4. Add labels to each button

**2.3 Optional: Hardware Debouncing**

For each button, solder a 0.1µF capacitor across the switch terminals:
```
      GPIO ───┬──────[Button]────┬─── GND
              │                   │
              └──────[0.1µF]──────┘
```

---

### Step 3: Wire Rotary Encoder

**3.1 Encoder Connections**

```
┌──────────────────────────────────────────────────┐
│        Rotary Encoder Pinout                     │
│                                                  │
│  CLK ──────────────→ ESP32 GPIO 26              │
│  DT  ──────────────→ ESP32 GPIO 27              │
│  SW  ──────────────→ ESP32 GPIO 12 (SELECT)     │
│  +   ──────────────→ ESP32 3.3V                 │
│  GND ──────────────→ ESP32 GND                  │
└──────────────────────────────────────────────────┘

Wire Colors (recommended):
- CLK:  White
- DT:   Gray
- SW:   Yellow (SELECT function)
- VCC:  Red
- GND:  Black

✅ All GPIOs have full internal pull-up resistor support
⚠️ Note: Encoder SW button serves dual purpose as SELECT button
```

**3.2 Assembly Steps**

1. Cut 5 wires (~15cm each)
2. Solder to encoder pins (or use connector if provided)
3. Add heat shrink to solder joints
4. Label wires

---

### Step 4: Wire RS485 Module (RS485 Communication)

**Choose your module type:**
- **Option A:** Auto-Direction Module (6 pins) - RECOMMENDED, simpler!
- **Option B:** Manual Direction Module (7-8 pins with DE/RE or DIR)

---

**4.1A Auto-Direction RS485 Module (6 pins) - RECOMMENDED**

```
┌──────────────┐      ┌──────────────┐      ┌──────────────┐
│    ESP32     │      │  RS485 Auto  │      │  Rodent V1.1 │
│              │      │  Module      │      │    Board     │
├──────────────┤      │  (6 pins)    │      ├──────────────┤
│ GPIO 2 (TX)  ├─────→│ DI or TXD    │      │              │
│ GPIO 4 (RX)  │←─────┤ RO or RXD    │      │              │
│              │      │              │      │              │
│ 3.3V or 5V   ├─────→│ VCC          │      │              │
│ GND          ├─────→│ GND  ────────┼─────→│ GND         │
│              │      │ A  ──────────┼─────→│ RS485 A     │
│              │      │ B  ──────────┼─────→│ RS485 B     │
└──────────────┘      └──────────────┘      └──────────────┘

Auto-Direction RS485 Pin Assignments (6 pins total):
┌───────────┬──────────────┬────────────────────────┐
│ Module    │ ESP32        │ Wire Color (Suggested) │
├───────────┼──────────────┼────────────────────────┤
│ DI/TXD    │ GPIO 2       │ Orange                 │
│ RO/RXD    │ GPIO 4       │ Yellow                 │
│ VCC       │ 3.3V or 5V   │ Red (check module!)    │
│ GND       │ GND          │ Black                  │
│ A         │ Rodent RS485A│ Blue (twisted pair)    │
│ B         │ Rodent RS485B│ White/Blue (twisted)   │
└───────────┴──────────────┴────────────────────────┘

✅ ADVANTAGES:
- Only 4 wires to ESP32 (no direction control wire needed)
- GPIO 15 is free for other uses
- Module automatically handles TX/RX direction
- Simpler firmware (default configuration)

⚠️ NOTES:
- Module has built-in automatic direction detection
- No DE/RE or DIR pin - only 6 pins total
- Check VCC voltage (some need 3.3V, others need 5V)
```

**Assembly Steps for Auto-Direction Module:**

1. **Identify your auto-direction module:**
   - Has only 6 pins total: VCC, GND, DI (or TXD), RO (or RXD), A, B
   - No DE, RE, or DIR pin
   - Common models: Various generic "TTL to RS485" auto modules

2. **ESP32 to RS485 module connections:**
   ```
   Using Dupont wires (recommended for testing):
   - GPIO 2  → Module DI or TXD  (orange wire)
   - GPIO 4  → Module RO or RXD  (yellow wire)
   - 3.3V/5V → Module VCC        (red wire - check module spec!)
   - GND     → Module GND        (black wire)
   ```

3. **RS485 module to Rodent board:**
   ```
   Using twisted pair cable:
   - Module A → Rodent RS485 A (blue wire)
   - Module B → Rodent RS485 B (white/blue wire)

   Using separate ground wire:
   - Module GND → Rodent GND (black wire)

   Connect at Rodent screw terminals:
   - Tighten securely
   - Test with gentle tug
   ```

4. **Firmware configuration:**
   ```
   ✅ Default firmware is already configured for auto-direction!

   In include/config.h, verify this line is active:
   #define RS485_AUTO_DIRECTION

   This is the default setting - no changes needed!
   ```

5. **Label connections:**
   ```
   ESP32 side: "RS485 Auto - DI/RO"
   Rodent side: "RS485 to Rodent - A/B"
   ```

**Testing Auto-Direction RS485 Connection:**

Before proceeding:
```
1. Power off everything
2. Check continuity:
   - ESP32 GND to RS485 GND: BEEP
   - RS485 GND to Rodent GND: BEEP
3. Check no shorts:
   - A to B: NO BEEP
   - A to GND: NO BEEP
   - B to GND: NO BEEP
4. Power on and test with test sketch (see test_sketches/07_test_rs485_rodent.ino)
```

---

**4.1B Manual Direction RS485 Module (7-8 pins with DE/RE or DIR)**

If you have a module with DE/RE or DIR pin for direction control:

```
┌──────────────┐      ┌──────────────┐      ┌──────────────┐
│    ESP32     │      │   MAX485     │      │  Rodent V1.1 │
│              │      │   Module     │      │    Board     │
├──────────────┤      │  (7-8 pins)  │      ├──────────────┤
│ GPIO 2 (TX)  ├─────→│ DI           │      │              │
│ GPIO 4 (RX)  │←─────┤ RO           │      │              │
│ GPIO 15      ├─────→│ DE/RE or DIR │      │              │
│ 3.3V         ├─────→│ VCC          │      │              │
│ GND          ├─────→│ GND  ────────┼─────→│ GND         │
│              │      │ A  ──────────┼─────→│ RS485 A     │
│              │      │ B  ──────────┼─────→│ RS485 B     │
└──────────────┘      └──────────────┘      └──────────────┘

Manual Direction RS485 Pin Assignments:
┌───────────┬──────────────┬────────────────────────┐
│ Module    │ ESP32        │ Wire Color (Suggested) │
├───────────┼──────────────┼────────────────────────┤
│ DI        │ GPIO 2       │ Orange                 │
│ RO        │ GPIO 4       │ Yellow                 │
│ DE        │ GPIO 15      │ Green   (tie to RE)    │
│ RE        │ GPIO 15      │ Green   (tie to DE)    │
│ VCC       │ 3.3V         │ Red                    │
│ GND       │ GND          │ Black                  │
│ A         │ Rodent RS485A│ Blue (twisted pair)    │
│ B         │ Rodent RS485B│ White/Blue (twisted)   │
└───────────┴──────────────┴────────────────────────┘

⚠️ CRITICAL NOTES:
- DE and RE pins MUST be connected together to GPIO 15
- Use twisted pair cable for A/B lines (reduces noise)
- Common ground is essential for all three devices
- Some modules have DE/RE already tied together internally
```

**Assembly Steps for Manual Direction Module:**

1. **Identify your manual direction module:**
   - Has 7-8 pins: VCC, GND, DI, RO, DE, RE (or DIR), A, B
   - Or has combined DE/RE or DIR pin (7 pins total)

2. **Firmware configuration:**
   ```
   In include/config.h, make these changes:

   Comment out this line:
   // #define RS485_AUTO_DIRECTION

   Uncomment this line:
   #define RODENT_RTS_PIN  15
   ```

3. **ESP32 to RS485 module connections:**
   ```
   Using Dupont wires:
   - GPIO 2  → Module DI          (orange wire)
   - GPIO 4  → Module RO          (yellow wire)
   - GPIO 15 → Module DE & RE tied together (green wire)
   - 3.3V    → Module VCC         (red wire)
   - GND     → Module GND         (black wire)
   ```

4. **Verify DE/RE connection:**
   ```
   ⚠️ CRITICAL: DE and RE must be tied together!

   If separate pins:
   - Solder jumper wire between DE and RE pins
   - Connect GPIO 15 to both

   If single DIR pin:
   - Connect GPIO 15 to DIR pin
   ```

5. **RS485 module to Rodent board:**
   ```
   Using twisted pair cable:
   - Module A → Rodent RS485 A (blue wire)
   - Module B → Rodent RS485 B (white/blue wire)
   - Module GND → Rodent GND (black wire)
   ```

**Testing Manual Direction RS485 Connection:**

Before proceeding:
```
1. Power off everything
2. Check continuity:
   - ESP32 GND to RS485 GND: BEEP
   - RS485 GND to Rodent GND: BEEP
   - GPIO 15 to DE: BEEP
   - GPIO 15 to RE: BEEP (should be tied together)
3. Check no shorts:
   - A to B: NO BEEP
   - A to GND: NO BEEP
   - B to GND: NO BEEP
4. Power on and test with test sketch
```

---

### Step 5: Wire MAX3232 Module (RS232 for Scale)

**5.1 RS232 Connection via MAX3232 Module**

```
┌──────────────┐      ┌──────────────┐      ┌──────────────┐
│    ESP32     │      │   MAX3232    │      │ Digital Scale│
│              │      │   RS232/TTL  │      │  (uxilaii)   │
├──────────────┤      ├──────────────┤      ├──────────────┤
│ GPIO 17 (TX) ├─────→│ T1IN         │      │              │
│ GPIO 16 (RX) │←─────┤ R1OUT        │      │              │
│ 3.3V or 5V   ├─────→│ VCC          │      │              │
│ GND          ├─────→│ GND  ────────┼─────→│ GND (Pin 5)  │
│              │      │ T1OUT ───────┼─────→│ RX (Pin 3)*  │
│              │      │ R1IN  ←──────┼───────│ TX (Pin 2)   │
└──────────────┘      └──────────────┘      └──────────────┘
                                             *If scale has RX

MAX3232 Pin Assignments:
┌───────────┬──────────────┬────────────────────────┐
│ MAX3232   │ ESP32        │ Wire Color (Suggested) │
├───────────┼──────────────┼────────────────────────┤
│ T1IN      │ GPIO 17      │ Orange                 │
│ R1OUT     │ GPIO 16      │ Brown                  │
│ VCC       │ 3.3V or 5V   │ Red (check module!)    │
│ GND       │ GND          │ Black                  │
│ T1OUT     │ Scale RX     │ (DB9 Pin 3)            │
│ R1IN      │ Scale TX     │ (DB9 Pin 2)            │
│ GND       │ Scale GND    │ (DB9 Pin 5)            │
└───────────┴──────────────┴────────────────────────┘

DB9 Connector Pinout (typical):
- Pin 2: TX (from scale) → MAX3232 R1IN
- Pin 3: RX (to scale)   → MAX3232 T1OUT (if needed)
- Pin 5: GND             → MAX3232 GND

⚠️ CRITICAL NOTES:
- RS232 uses ±12V - will DESTROY ESP32 if connected directly
- Check if your MAX3232 module needs 3.3V or 5V power
- Most scales are TX-only (you may not need T1OUT connection)
- DB9 Male vs Female: Pin numbers same, but check carefully
```

**5.2 Assembly Steps**

1. **Identify MAX3232 module pinout:**
   - Check your module's silkscreen labels
   - Common layouts: "RS232 to TTL" module or bare MAX3232 chip
   - Verify VCC voltage rating (3.3V or 5V)
   - Some modules include DB9 connector

2. **ESP32 to MAX3232 connections (TTL side):**
   ```
   Using Dupont wires (recommended for testing):
   - GPIO 17 → MAX3232 T1IN  (orange wire)
   - GPIO 16 → MAX3232 R1OUT (brown wire)
   - Check module: 3.3V or 5V → MAX3232 VCC (red wire)
   - GND → MAX3232 GND (black wire)
   ```

3. **Identify scale connector:**
   - Check scale manual for pinout
   - Common: DB9 connector (male or female)
   - Some scales: 3.5mm jack, USB, or proprietary
   - You may need a DB9 breakout cable

4. **MAX3232 to Scale connections (RS232 side):**
   ```
   If scale has DB9 Female connector on cable:
   - MAX3232 R1IN → DB9 Pin 2 (Scale TX)
   - MAX3232 T1OUT → DB9 Pin 3 (Scale RX) - optional
   - MAX3232 GND → DB9 Pin 5 (GND)

   If scale has DB9 Male connector:
   - MAX3232 R1IN → DB9 Pin 3 (Scale TX)
   - MAX3232 T1OUT → DB9 Pin 2 (Scale RX) - optional
   - MAX3232 GND → DB9 Pin 5 (GND)

   Note: Pin 2 and 3 are swapped between male and female!
   ```

5. **Test scale output first:**
   ```
   Before connecting to MAX3232:
   1. Power on scale
   2. Use multimeter on DC voltage setting
   3. Measure between Pin 2 (or 3) and Pin 5 (GND)
   4. Press scale print button
   5. Should see voltage swinging between +12V and -12V
   6. This confirms RS232 output
   ```

6. **Label connections:**
   ```
   ESP32 side: "MAX3232 - TTL side (GPIO 16/17)"
   Scale side: "RS232 to Scale - 9600 baud"
   ```

**5.3 Testing MAX3232 Connection**

Before proceeding:
```
1. Power off everything
2. Check continuity:
   - ESP32 GND to MAX3232 GND: BEEP
   - MAX3232 GND to Scale GND: BEEP
3. Check no shorts on RS232 side:
   - T1OUT to GND: NO BEEP
   - R1IN to GND: NO BEEP
4. Power on scale only, verify it powers up
5. Power on ESP32, upload firmware to test
```

---

### Step 6: Wire I2C LCD Display

**6.1 LCD Connection (4-wire)**

```
┌────────────────────────────────────────────┐
│         1602 LCD I2C Wiring                │
│                                            │
│  ESP32 GPIO 21 (SDA) ──→ LCD SDA          │
│  ESP32 GPIO 22 (SCL) ──→ LCD SCL          │
│  ESP32 5V ────────────→ LCD VCC           │
│  ESP32 GND ───────────→ LCD GND           │
└────────────────────────────────────────────┘

Wire Colors (recommended):
- SDA:  Green
- SCL:  Blue
- VCC:  Red
- GND:  Black

I2C Module Check:
- Most 1602 I2C modules have built-in pull-ups
- Default address usually 0x27 or 0x3F
- Has potentiometer for contrast adjustment
```

**6.2 Assembly Steps**

1. **Verify I2C module:**
   - Check for I2C backpack on LCD
   - Note the address (usually marked)
   - Test potentiometer adjusts contrast

2. **Prepare 4-wire cable:**
   - Cut 4 wires (~20cm each)
   - Strip and tin ends
   - Consider using 4-pin JST connector

3. **Connect to LCD:**
   - Solder or plug into I2C module
   - Double-check pinout (varies by module)
   - Common order: GND-VCC-SDA-SCL

4. **Connect to ESP32:**
   - SDA → GPIO 21
   - SCL → GPIO 22
   - VCC → 5V (or 3.3V if LCD supports)
   - GND → GND

5. **Test before final assembly:**
   - Use I2C scanner sketch
   - Verify address detected

---

### Step 7: Wire WS2812B LED Strips

**7.1 LED Strip Connection (3-wire)**

```
┌────────────────────────────────────────────────────────┐
│              WS2812B LED Wiring                        │
│                                                        │
│  ESP32 GPIO 25 ────→ Strip 1 DIN                      │
│                      Strip 1 DOUT → Strip 2 DIN       │
│                      Strip 2 DOUT → Strip 3 DIN       │
│                      Strip 3 DOUT → Strip 4 DIN       │
│                                                        │
│  5V Supply ────────→ VCC (all strips, common bus)     │
│  GND ──────────────→ GND (all strips, common bus)     │
└────────────────────────────────────────────────────────┘

Important Notes:
- Power: Connect VCC/GND at MULTIPLE points (every strip)
- Data: Connect in series (DOUT to next DIN)
- Direction: Data flows one direction only
- Signal: Keep data wire short (<2m from ESP32)
```

**7.2 LED Strip Layout**

```
Strip Configuration (32 LEDs total):

ESP32 GPIO 25 ───→ [Strip 1: 8 LEDs] ───→ [Strip 2: 8 LEDs]
                    Pump 1 (Cyan)          Pump 2 (Magenta)
                           │                       │
                           └─→ [Strip 3: 8 LEDs]  │
                               Pump 3 (Yellow)     │
                                      │            │
                                      └─→ [Strip 4: 8 LEDs]
                                          Pump 4 (White)

Power Distribution:
    5V ─┬─→ Strip 1 VCC
        ├─→ Strip 2 VCC
        ├─→ Strip 3 VCC
        └─→ Strip 4 VCC

   GND ─┬─→ Strip 1 GND
        ├─→ Strip 2 GND
        ├─→ Strip 3 GND
        └─→ Strip 4 GND
```

**7.3 Assembly Steps**

1. **Cut LED strips:**
   - Cut 4 strips of 8 LEDs each
   - Cut ONLY at designated cut points (marked on strip)
   - Note direction arrow on strip

2. **Prepare power bus:**
   - Create 5V power bus using screw terminal
   - Create GND power bus using screw terminal
   - Use 18 AWG wire for power distribution

3. **Solder power connections:**
   ```
   For each strip:
   - Solder red wire (18 AWG) to VCC pad
   - Solder black wire (18 AWG) to GND pad
   - Connect all VCC wires to 5V bus
   - Connect all GND wires to GND bus
   ```

4. **Solder data connections:**
   ```
   Strip 1 DIN  ← GPIO 25 (22 AWG wire)
   Strip 1 DOUT → Strip 2 DIN (22 AWG wire)
   Strip 2 DOUT → Strip 3 DIN (22 AWG wire)
   Strip 3 DOUT → Strip 4 DIN (22 AWG wire)
   ```

5. **Add capacitor (recommended):**
   - 1000µF capacitor across 5V and GND near ESP32
   - Prevents voltage spikes

6. **Test LEDs:**
   - Upload test sketch before final assembly
   - Verify all 32 LEDs light up
   - Check correct order and colors

---

### Step 8: Wire Motors to Rodent Board

**8.1 Stepper Motor Connections**

```
┌──────────────────────────────────────────────────────┐
│         Rodent Board Motor Outputs                   │
│                                                      │
│  X-axis (Motor 1) ────→ Pump 1 (DMDEE)              │
│  Y-axis (Motor 2) ────→ Pump 2 (T-12)               │
│  Z-axis (Motor 3) ────→ Pump 3 (T-9)                │
│  A-axis (Motor 4) ────→ Pump 4 (L25B)               │
└──────────────────────────────────────────────────────┘

NEMA 17 Typical Pinout (verify with motor datasheet):
- Wire 1: Coil A+  (often Black or Red)
- Wire 2: Coil A-  (often Green or Blue)
- Wire 3: Coil B+  (often Red or Yellow)
- Wire 4: Coil B-  (often Blue or White)

⚠️ IMPORTANT: Wrong wiring won't damage motor but it won't work
Test motor direction and adjust if needed in FluidNC config
```

**8.2 Assembly Steps**

1. **Identify motor wires:**
   - Check motor datasheet
   - Use multimeter to find coil pairs:
     * Measure resistance between wires
     * Coil A: ~4-8 ohms between pair
     * Coil B: ~4-8 ohms between pair
     * No continuity between coils

2. **Connect to Rodent board:**
   - Plug motor connector into X/Y/Z/A ports
   - Or use screw terminals if no connector
   - Label each motor: "Pump 1 (X)", "Pump 2 (Y)", etc.

3. **Secure motors:**
   - Mount motors to pump mechanisms
   - Ensure alignment
   - Use strain relief on cables

---

### Step 9: Power Distribution

**9.1 Power Architecture**

```
┌──────────────┐
│ 24V / 10A    │
│ Power Supply │
└──────┬───────┘
       │
       ├─────────────────────────────┐
       │                             │
       │                             │
   ┌───▼──────┐              ┌──────▼─────────┐
   │  Rodent  │              │ Buck Converter │
   │  Board   │              │  24V → 5V/3A   │
   │          │              └──────┬─────────┘
   │  Motor   │                     │
   │  Power   │         ┌───────────┼──────────┐
   └──────────┘         │           │          │
                        │           │          │
                   ┌────▼────┐  ┌───▼───┐  ┌──▼──┐
                   │  ESP32  │  │  LEDs │  │ LCD │
                   │   5V    │  │  5V   │  │ 5V  │
                   └─────────┘  └───────┘  └─────┘
```

**9.2 Power Wiring Steps**

1. **Main power input:**
   ```
   Power Supply:
   - Connect AC input (follow local electrical codes)
   - V+ terminal
   - V- terminal (GND)
   ```

2. **Rodent board power:**
   ```
   From 24V supply:
   - Red wire (18 AWG)  → Rodent V+
   - Black wire (18 AWG) → Rodent GND
   - Add inline fuse (10A)
   ```

3. **Buck converter input:**
   ```
   From 24V supply:
   - Red wire (18 AWG)  → Buck IN+
   - Black wire (18 AWG) → Buck IN-
   ```

4. **Buck converter output (5V):**
   ```
   Adjust output to 5V using potentiometer:
   - Use multimeter to verify 5.0V ± 0.1V
   - DO NOT connect loads until verified!
   ```

5. **5V distribution:**
   ```
   From Buck OUT+:
   - ESP32 VIN pin (or 5V pin)
   - LED strips VCC (all 4)
   - LCD VCC

   From Buck OUT-:
   - ESP32 GND
   - LED strips GND (all 4)
   - LCD GND
   ```

6. **Create common ground:**
   ```
   ⚠️ CRITICAL: All grounds must connect together:
   - 24V supply GND
   - Buck converter GND
   - Rodent board GND
   - ESP32 GND
   - All peripheral GNDs

   Use star grounding topology (all to one point)
   ```

---

## Connection Verification

### Pre-Power Checks

Use these checks BEFORE applying power:

#### 1. Visual Inspection
```
□ All solder joints shiny and solid (no cold joints)
□ No solder bridges between pins
□ All connections secure (gentle tug test)
□ Polarity correct on all components
□ No exposed wire conductors (all insulated)
□ No wires rubbing against sharp edges
```

#### 2. Continuity Tests
```
With multimeter in continuity mode (power OFF):

□ ESP32 GND to Buck converter GND: BEEP
□ Buck GND to Rodent GND: BEEP
□ ESP32 GPIO 2 to Rodent RS485 A: BEEP
□ ESP32 GPIO 4 to Rodent RS485 B: BEEP
□ ESP32 GPIO 25 to LED Strip 1 DIN: BEEP
□ Each LED strip VCC to 5V bus: BEEP
□ Each LED strip GND to GND bus: BEEP
□ LCD SDA to ESP32 GPIO 21: BEEP
□ LCD SCL to ESP32 GPIO 22: BEEP
```

#### 3. Isolation Tests
```
With multimeter in continuity mode (power OFF):

□ 24V+ to GND: NO BEEP (no short circuit)
□ 5V+ to GND: NO BEEP (no short circuit)
□ ESP32 3.3V to GND: NO BEEP (no short circuit)
```

#### 4. Resistance Tests
```
With multimeter in resistance mode (power OFF):

□ 24V+ to GND: >10kΩ (high impedance)
□ 5V+ to GND: >1kΩ (expect some load from LEDs/components)
□ ESP32 3.3V to GND: >1kΩ
```

---

## Testing Procedures

### Phase 1: Power Test (ESP32 Only)

**Step 1: Connect ESP32 via USB**
```
1. Connect ESP32 to computer via USB only
2. Do NOT connect external power yet
3. Verify:
   - Blue LED lights up
   - Computer recognizes device
   - No smoke, no smell
```

**Step 2: Test Buck Converter**
```
1. Disconnect ESP32 from USB
2. Connect buck converter INPUT to 24V supply
3. Adjust potentiometer to 5.0V output
4. Verify with multimeter: 5.0V ± 0.1V
5. Disconnect 24V supply
```

**Step 3: Connect ESP32 to Buck Converter**
```
1. Connect buck converter output to ESP32 VIN/GND
2. Apply 24V power to buck converter
3. Verify:
   - ESP32 blue LED lights up
   - Voltage at ESP32 VIN: 5.0V
   - No unusual heating
4. Power OFF
```

---

### Phase 2: Peripheral Test (One at a Time)

**Test 1: Buttons**
```
1. Power on ESP32
2. Upload test sketch: test_sketches/01_test_buttons.ino
3. Open Serial Monitor (115200 baud)
4. Press each button, verify output:
   - START (GPIO 13): "✓ START button PRESSED"
   - STOP (GPIO 33): "✓ STOP button PRESSED"
   - MODE (GPIO 14): "✓ MODE button PRESSED"
5. Power OFF

Note: SELECT button is tested with encoder (see Test 2)
```

**Test 2: Rotary Encoder**
```
1. Upload test sketch: test_sketches/02_test_encoder.ino
2. Open Serial Monitor (115200 baud)
3. Rotate encoder clockwise: "CW → Position: X"
4. Rotate encoder counter-clockwise: "CCW ← Position: X"
5. Press encoder button: "✓ SELECT button PRESSED" (dual function)
6. Power OFF

Note: Encoder button also serves as SELECT button
```

**Test 3: I2C LCD**
```
1. Upload test sketch: test_sketches/03_test_i2c_scanner.ino
2. Verify LCD address detected (0x27 or 0x3F)
3. Upload test sketch: test_sketches/04_test_lcd.ino
4. Adjust contrast potentiometer until text visible
5. Verify display shows:
   - Line 1: "Pump Controller"
   - Line 2: "LCD Test OK!"
6. Power OFF

Note: Requires LiquidCrystal_I2C library
```

**Test 4: LED Strips**
```
1. Ensure 5V power connected to all LED strips
2. Upload test sketch: test_sketches/05_test_leds.ino
3. Observe automatic test sequence:
   - All LEDs: Red → Green → Blue → White
   - Individual LED test (0-31)
   - Strip-by-strip test (different colors)
   - Rainbow animation
4. Verify all 32 LEDs work correctly
5. If issues:
   - Check which LED is last working (data connection)
   - Check if strip has power (VCC/GND)
6. Power OFF

Note: Requires FastLED library
```

**Test 5: Scale Serial (via MAX3232)**
```
1. Connect scale to power (battery or USB)
2. Upload test sketch: test_sketches/06_test_scale.ino
3. Open Serial Monitor (115200 baud)
4. Press scale PRINT button or enable continuous mode
5. Verify Serial Monitor shows:
   "✓ Parsed weight: XX.XX g"
6. Test with known weight
7. Power OFF

⚠️ CRITICAL: Requires MAX3232 converter!
   NEVER connect RS232 directly to ESP32!
```

**Test 6: RS485 to Rodent (via MAX485)**
```
1. Power on Rodent board (24V)
2. Power on ESP32 (5V)
3. Upload test sketch: test_sketches/07_test_rs485_rodent.ino
4. Open Serial Monitor (115200 baud)
5. Observe automatic command sequence:
   - "?" (status query)
   - "$X" (unlock)
   - "$$" (view settings)
   - "$G" (parser state)
   - "$I" (build info)
6. Verify responses: "✓ Status response received", "✓ OK response"
7. Power OFF both boards

⚠️ CRITICAL: Requires MAX485 converter!
   DE and RE pins must be tied together!
```

---

### Phase 3: Motor Test

**Safety First:**
```
⚠️ Ensure pumps are NOT connected to chemicals
⚠️ Use water or run dry for initial testing
⚠️ Have STOP button easily accessible
```

**Motor Test Procedure:**
```
1. Power on all systems
2. Verify Rodent board initialized
3. Send unlock command: "$X"
4. Test each motor individually:

   Motor 1 (X-axis / Pump 1):
   - Send: "G1 X10 F100"
   - Verify motor rotates
   - Verify correct direction
   - Send: "G1 X0 F100" (return)

   Motor 2 (Y-axis / Pump 2):
   - Send: "G1 Y10 F100"
   - Verify motor rotates
   - Send: "G1 Y0 F100"

   Motor 3 (Z-axis / Pump 3):
   - Send: "G1 Z10 F100"
   - Verify motor rotates
   - Send: "G1 Z0 F100"

   Motor 4 (A-axis / Pump 4):
   - Send: "G1 A10 F100"
   - Verify motor rotates
   - Send: "G1 A0 F100"

5. Test simultaneous motion:
   - Send: "G1 X10 Y10 Z10 A10 F100"
   - Verify all motors move

6. Test emergency stop:
   - Start motion
   - Press STOP button
   - Verify immediate halt
```

---

### Phase 4: Integration Test

**Full System Test:**
```
1. Upload final firmware
2. Power on entire system
3. Verify boot sequence:
   - LEDs show boot animation
   - LCD displays startup message
   - Serial Monitor shows initialization
   - WiFi connects (if configured)

4. Test UI:
   - Buttons respond correctly
   - Encoder navigates menus
   - LCD displays updates
   - LEDs show status

5. Test dosing operation (with water):
   - Place container on scale
   - Tare scale
   - Select recipe
   - Press START
   - Verify:
     * Motors dispense correctly
     * Scale reads increasing weight
     * LEDs show animation
     * Process completes
     * Final weight matches target

6. Test error conditions:
   - Press STOP during operation
   - Simulate scale error (disconnect)
   - Simulate motor error (disconnect)
   - Verify error indicators (red LEDs)
```

---

## Troubleshooting Common Issues

### LED Strip Issues

| Problem | Cause | Solution |
|---------|-------|----------|
| First LED works, rest dark | Data connection broken | Check DOUT to next DIN connection |
| All LEDs dark | No power or data | Check 5V power, verify GPIO 25 connection |
| Random colors/flickering | Insufficient power | Add capacitor, use thicker power wires |
| Some strips don't work | Data signal degraded | Keep data wire short, add level shifter |

### RS485 Communication Issues (MAX485 Module)

| Problem | Cause | Solution |
|---------|-------|----------|
| No response from Rodent | Missing MAX485 module | Verify MAX485 is installed and powered |
| No response from Rodent | Wrong A/B polarity | Swap RS485 A and B connections |
| Garbled data | DE/RE not tied together | Verify DE and RE both connect to GPIO 15 |
| Intermittent communication | Wrong direction control | Check GPIO 15 connection to DE/RE |
| Random characters | No common ground | Connect GND between ESP32, MAX485, and Rodent |
| Weak signal/errors | Power issue | Verify MAX485 VCC is 3.3V or 5V (check module spec) |
| Noise/interference | Poor wiring | Use shielded twisted pair cable for A/B |
| No communication at all | Wrong baud rate | Verify 115200 baud in firmware |

### Scale / RS232 Communication Issues

| Problem | Cause | Solution |
|---------|-------|----------|
| No data from scale | Missing MAX3232 module | Verify MAX3232 is installed and powered |
| No data from scale | Wrong baud rate | Verify 9600 baud setting on scale |
| ESP32 damaged/not working | Direct RS232 connection | **NEVER connect RS232 (±12V) directly to ESP32!** |
| Continuous gibberish | Wrong polarity | Check DB9 male vs female (Pin 2/3 swap) |
| Wrong values | Wrong format | Verify scale outputs "XX.XX g\r\n" format |
| Intermittent data | Poor ground connection | Ensure GND connected between all three devices |
| No power to MAX3232 | Wrong voltage | Check if module needs 3.3V or 5V |
| Scale reads but ESP32 doesn't | TX/RX confusion | Verify: Scale TX → MAX3232 R1IN → ESP32 GPIO 16 (RX) |

### Motor Issues

| Problem | Cause | Solution |
|---------|-------|----------|
| Motor hums but doesn't turn | Wrong coil pairing | Re-identify coil pairs with multimeter |
| Wrong direction | Phase sequence | Swap one coil pair wires |
| Motor skips steps | Insufficient current | Increase motor current in FluidNC |
| All motors dead | FluidNC not responding | Check RS485, verify FluidNC firmware |

---

## Final Assembly Tips

### 1. Cable Management
- Use cable ties for bundling
- Separate power and signal cables
- Label all connections
- Allow slack for movement

### 2. Strain Relief
- Hot glue around solder joints
- Use cable glands in enclosure
- Avoid sharp bends in wires

### 3. Mounting
- Use standoffs for ESP32
- Mount Rodent board securely
- Ensure adequate cooling
- Keep components accessible

### 4. Documentation
- Take photos of final assembly
- Create wiring diagram specific to your build
- Label everything
- Keep spare parts

---

## Next Steps

After completing wiring:

1. **Test Each Component**: Upload and run test sketches in `test_sketches/` directory
   - Follow the test sequence in order (buttons → encoder → I2C → LCD → LEDs → scale → RS485)
   - See [Test Sketches README](../../test_sketches/README.md) for details
2. **Firmware Upload**: After all tests pass, see [Quick Start Deployment Guide](../setup/QUICK_START_DEPLOYMENT_GUIDE.md)
3. **FluidNC Configuration**: See [FluidNC Setup Guide](../setup/FLUIDNC_SETUP_GUIDE.md)
4. **Scale Configuration**: See [Scale Setup Guide](../setup/SCALE_SETUP_GUIDE.md)
5. **System Calibration**: Calibrate pumps for accurate dispensing

---

## References

- [Hardware Overview](HARDWARE_OVERVIEW.md) - Complete component specifications
- [FluidNC Setup Guide](../setup/FLUIDNC_SETUP_GUIDE.md)
- [Scale Setup Guide](../setup/SCALE_SETUP_GUIDE.md)
- ESP32 Pinout Reference: https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
- WS2812B Wiring Guide: https://learn.adafruit.com/adafruit-neopixel-uberguide

---

*Last Updated: 2025-11-03*
