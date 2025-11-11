# Test Programs - Peristaltic Pump System

This directory contains incremental test programs for building and testing the 4-pump peristaltic system using **PlatformIO with ESP-IDF framework**.

## Quick Start

### Prerequisites

1. **Install PlatformIO:**
   ```bash
   # Via VSCode extension (recommended)
   # Or via CLI:
   pip install platformio
   ```

2. **Clone repository:**
   ```bash
   git clone <repository-url>
   cd Test_Programming
   ```

3. **Connect ESP32** via USB

### Running Tests

Each test is a separate PlatformIO environment. Use these commands:

```bash
# Build and upload a specific test
pio run -e test_00_blink -t upload

# Build, upload, and monitor serial output
pio run -e test_00_blink -t upload -t monitor

# Just monitor (after uploading)
pio device monitor

# List all available test environments
pio run --list-targets
```

---

## Phase 1: Basic Hardware Testing

### Test 00: Blink and Serial Output

**Purpose:** Verify ESP32 is working and programmable.

**Command:**
```bash
pio run -e test_00_blink -t upload -t monitor
```

**Hardware Required:**
- ESP32 development board
- USB cable

**Success Criteria:**
- ✅ Built-in LED (GPIO 2) blinks every 1 second
- ✅ Serial monitor shows "Blink Test - Running..." messages
- ✅ Free heap memory displayed

**Expected Output:**
```
========================================
Peristaltic Pump System - Test 00
Blink Test
========================================
[0] LED: ON  | Free Heap: 295000 bytes
[1] LED: OFF | Free Heap: 295000 bytes
[2] LED: ON  | Free Heap: 295000 bytes
```

**Troubleshooting:**
- If upload fails: Check USB cable, try different port
- If LED doesn't blink: Verify GPIO 2 is available (some boards use it for other purposes)
- If no serial output: Check baud rate is 115200

---

### Test 01: Push Buttons

**Purpose:** Verify all 3 control buttons with internal pull-ups.

**Command:**
```bash
pio run -e test_01_buttons -t upload -t monitor
```

**Hardware Required:**
- ESP32 development board
- 3× Push buttons (Normally Open, NO)
- Jumper wires

**Wiring:**
```
START Button:  GPIO 13 ──[Button]── GND
MODE Button:   GPIO 14 ──[Button]── GND
STOP Button:   GPIO 33 ──[Button]── GND
```

**No external resistors needed** - uses internal pull-ups!

**Success Criteria:**
- ✅ START button (GPIO 13) detected on press/release
- ✅ MODE button (GPIO 14) detected on press/release
- ✅ STOP button (GPIO 33) detected on press/release
- ✅ Button press duration calculated correctly
- ✅ No false triggers (good debouncing)

**Expected Output:**
```
[12345] ✓ START button PRESSED (count: 1)
[12789] ✗ START button RELEASED (duration: 444ms)
[15632] ✓ MODE button PRESSED (count: 1)
[15901] ✗ MODE button RELEASED (duration: 269ms)
```

**Troubleshooting:**
- If button not detected: Check wiring, verify common GND
- If false triggers: Increase debounce time in pin_definitions.h
- If button always reads pressed: Check for short to GND

---

### Test 02: Rotary Encoder

**Purpose:** Verify encoder rotation and SELECT button.

**Command:**
```bash
pio run -e test_02_encoder -t upload -t monitor
```

**Hardware Required:**
- ESP32 development board
- Rotary encoder with button (KY-040 or similar)
- Jumper wires

**Wiring:**
```
Encoder CLK:  GPIO 26
Encoder DT:   GPIO 27
Encoder SW:   GPIO 12 (SELECT button)
Encoder GND:  GND
Encoder VCC:  3.3V (if required by module)
```

**Success Criteria:**
- ✅ Clockwise rotation increases position counter
- ✅ Counter-clockwise rotation decreases counter
- ✅ Encoder button (SELECT) press detected
- ✅ Smooth rotation without skipping
- ✅ No false triggers

**Expected Output:**
```
[12345] Position: 1 (CW →)
[12456] Position: 2 (CW →)
[12789] Position: 1 (CCW ←)
[15632] ✓ SELECT button PRESSED (count: 1) [Position: 1]
[15901] ✗ SELECT button RELEASED (duration: 269ms)
```

**Note:** The encoder button serves dual purpose:
1. **Rotation:** Navigate through menus
2. **Press:** Confirm selection (SELECT function)

**Troubleshooting:**
- If position jumps erratically: Add hardware debouncing (0.1µF capacitors)
- If direction reversed: Swap CLK and DT pins
- If button not detected: Check SW pin connection
- If double-counting: Adjust debounce delay

---

## Phase 2: Communication Peripherals (Coming Soon)

### Test 03: I2C Scanner
**Status:** 🚧 Not yet implemented

**Purpose:** Detect I2C devices, verify LCD address

### Test 04: LCD Display
**Status:** 🚧 Not yet implemented

**Purpose:** Display text on 1602 LCD via I2C

### Test 05: WS2812B LED Strips
**Status:** 🚧 Not yet implemented

**Purpose:** Control 32 LEDs (4 strips × 8 LEDs)

### Test 06: Digital Scale (RS232)
**Status:** 🚧 Not yet implemented

**Purpose:** Read weight data via RS232 (requires MAX3232 converter)

---

## Phase 3: RS485 Motor Control (Coming Soon)

### Test 07: RS485 Basic Communication
**Status:** 🚧 Not yet implemented

**Purpose:** Test RS485 communication with BTT Rodent board

### Test 08: G-code Commands
**Status:** 🚧 Not yet implemented

**Purpose:** Send G-code commands, parse responses

### Test 09: Single Motor
**Status:** 🚧 Not yet implemented

**Purpose:** Test one pump motor

### Test 10: All Motors
**Status:** 🚧 Not yet implemented

**Purpose:** Test all 4 pump motors

---

## Project Structure

```
Test_Programming/
├── platformio.ini          # PlatformIO configuration (all test environments)
├── test/                   # Test programs directory
│   ├── common/            # Shared headers
│   │   └── pin_definitions.h  # Pin mappings for entire system
│   ├── test_00_blink/     # Test 00: Blink
│   │   └── test_00_blink.c
│   ├── test_01_buttons/   # Test 01: Buttons
│   │   └── test_01_buttons.c
│   ├── test_02_encoder/   # Test 02: Encoder
│   │   └── test_02_encoder.c
│   └── ...                # More tests (coming soon)
├── docs/                  # Documentation
│   ├── hardware/          # Hardware guides
│   ├── setup/            # Setup guides
│   └── reference/        # Reference docs
└── DEVELOPMENT_PLAN.md   # Complete development plan

```

---

## Common Pin Definitions

All tests use the same pin definitions from `test/common/pin_definitions.h`:

| Component | GPIO | Function |
|-----------|------|----------|
| **Buttons** | | |
| START Button | 13 | Start operation |
| MODE Button | 14 | Mode selection |
| STOP Button | 33 | Emergency stop |
| **Encoder** | | |
| Encoder CLK | 26 | Rotation clock |
| Encoder DT | 27 | Rotation direction |
| Encoder SW (SELECT) | 12 | Button press / SELECT |
| **RS485 (Rodent)** | | |
| Rodent TX | 2 | UART1 TX → RS485 DI |
| Rodent RX | 4 | UART1 RX ← RS485 RO |
| Rodent RTS | 15 | RS485 direction control |
| **RS232 (Scale)** | | |
| Scale RX | 16 | UART2 RX ← MAX3232 |
| Scale TX | 17 | UART2 TX → MAX3232 |
| **I2C (LCD)** | | |
| LCD SDA | 21 | I2C data |
| LCD SCL | 22 | I2C clock |
| **LED Control** | | |
| LED Data | 25 | WS2812B data (RMT) |

---

## Tips and Best Practices

### 1. Always Test in Order
Don't skip test phases! Each test builds on previous ones:
- Test 00 → Test 01 → Test 02 → ...

### 2. Use Serial Monitor
The serial monitor (115200 baud) is essential for debugging:
```bash
pio device monitor
# Or combined with upload:
pio run -e test_XX -t upload -t monitor
```

Exit monitor: `Ctrl + ]`

### 3. Hardware Before Software
Always verify hardware connections before debugging code:
1. Check continuity with multimeter
2. Verify power (3.3V or 5V)
3. Confirm common ground
4. Look for shorts

### 4. ESP-IDF vs Arduino
These tests use **ESP-IDF** (not Arduino framework):
- More control and efficiency
- Better for production systems
- Steeper learning curve
- Full RTOS (FreeRTOS) features

### 5. Clean Build
If you encounter strange errors:
```bash
pio run -t clean
pio run -e test_XX -t upload
```

### 6. Update PlatformIO
Keep PlatformIO and platforms updated:
```bash
pio upgrade
pio platform update
```

---

## Safety Warnings

### ⚠️ Critical: Level Shifters Required

1. **RS232 (Digital Scale):**
   - Scale outputs ±12V (RS232 levels)
   - **MUST use MAX3232 converter** to 3.3V TTL
   - **NEVER connect RS232 directly to ESP32** - will destroy GPIO!

2. **RS485 (BTT Rodent Board):**
   - Differential signaling (A+/B-)
   - **MUST use MAX485 (or auto-direction) converter module**
   - **NEVER connect RS485 directly to ESP32**

3. **Common Ground:**
   - All devices must share common ground
   - Check with multimeter before powering on

4. **Voltage Levels:**
   - ESP32 GPIO: 3.3V max (5V NOT tolerant!)
   - LEDs (WS2812B): 5V power, but 3.3V data usually OK
   - LCD: Usually 5V power, but I2C is 3.3V tolerant

---

## Troubleshooting

### Upload Fails
```
Error: Failed to connect to ESP32
```
**Solution:**
- Hold BOOT button while uploading
- Check USB cable (needs data lines, not just power)
- Try different USB port
- Install/update USB drivers (CP2102, CH340)

### Serial Monitor Shows Garbage
**Solution:**
- Verify baud rate is 115200
- Check USB cable quality
- Try: `pio device monitor --baud 115200`

### ESP32 Keeps Resetting
**Solution:**
- Insufficient power (use good USB cable/power supply)
- Brown-out detector triggered
- Check for shorts on GPIO pins

### GPIO Not Working
**Solution:**
- Some GPIOs are input-only: 34, 35, 36, 39
- Some GPIOs have special functions (strapping pins)
- Check pin is not used by built-in peripherals

---

## Next Steps

After completing Phase 1 tests:

1. **Review results** - ensure all 3 tests pass
2. **Document issues** - note any hardware problems
3. **Proceed to Phase 2** - communication peripherals
4. **Reference documentation:**
   - `docs/hardware/HARDWARE_OVERVIEW.md`
   - `docs/hardware/WIRING_GUIDE.md`
   - `DEVELOPMENT_PLAN.md`

---

## Getting Help

If you're stuck:

1. **Check documentation** in `docs/` folder
2. **Review wiring** against HARDWARE_OVERVIEW.md
3. **Check GitHub issues** (if available)
4. **ESP-IDF documentation:** https://docs.espressif.com/projects/esp-idf/

---

## Contributing

When creating new test programs:

1. **Follow naming convention:** `test_XX_name/test_XX_name.c`
2. **Add to platformio.ini** as new environment
3. **Document in this README**
4. **Include clear success criteria**
5. **Add troubleshooting section**

---

**Document Version:** 1.0
**Last Updated:** 2025-11-11
**Framework:** ESP-IDF via PlatformIO
**Board:** ESP32 Dev Module
