# Peristaltic Pump System - Incremental Development Plan

## Overview

This document provides a step-by-step plan for developing the 4-pump peristaltic pumping system. The approach is **incremental and modular**, allowing you to test each component individually before integration.

## Development Philosophy

✅ **Build slowly and test frequently**
✅ **Each phase is independent and testable**
✅ **Verify hardware before moving to next phase**
✅ **Keep code modular and well-documented**

---

## Hardware Summary

| Component | Interface | GPIO Pins | Purpose |
|-----------|-----------|-----------|---------|
| BTT Rodent Board | RS485 | GPIO 2, 4, 15 | Motor control (4 pumps) |
| Digital Scale | RS232 | GPIO 16, 17 | Weight monitoring |
| LCD Display | I2C | GPIO 21, 22 | User interface |
| LED Strips | RMT | GPIO 25 | Visual feedback (32 LEDs) |
| START Button | Digital In | GPIO 13 | Start operation |
| STOP Button | Digital In | GPIO 33 | Stop/Emergency stop |
| MODE Button | Digital In | GPIO 14 | Mode selection |
| Encoder CLK | Digital In | GPIO 26 | Rotation sensing |
| Encoder DT | Digital In | GPIO 27 | Direction sensing |
| Encoder SW (SELECT) | Digital In | GPIO 12 | Button press/SELECT |

---

## Phase 1: Basic Hardware Testing (Foundation)

**Goal:** Verify all hardware connections and basic I/O functionality.

### Step 1.1: Test Built-in LED and Serial Monitor
**Objective:** Confirm ESP32 is working and can be programmed.

**Test File:** `test_sketches/00_test_blink.ino`

**What to create:**
```cpp
- Simple blink sketch
- Serial output at 115200 baud
- Verify USB connection works
```

**Success Criteria:**
- ✅ ESP32 uploads code successfully
- ✅ Built-in LED blinks
- ✅ Serial monitor shows messages

**Estimated Time:** 10 minutes

---

### Step 1.2: Test Push Buttons
**Objective:** Verify all 3 buttons are wired correctly with internal pull-ups.

**Test File:** `test_sketches/01_test_buttons.ino`

**What to create:**
```cpp
- Configure GPIO 13, 14, 33 as INPUT_PULLUP
- Read button states
- Print to serial when pressed/released
- Test debouncing
```

**Success Criteria:**
- ✅ START button (GPIO 13) detected
- ✅ MODE button (GPIO 14) detected
- ✅ STOP button (GPIO 33) detected
- ✅ No false triggers (good debouncing)

**Hardware Check:**
- Buttons connected between GPIO and GND
- No external resistors needed (using internal pull-ups)

**Estimated Time:** 20 minutes

---

### Step 1.3: Test Rotary Encoder
**Objective:** Verify encoder rotation and button (SELECT function).

**Test File:** `test_sketches/02_test_encoder.ino`

**What to create:**
```cpp
- Configure GPIO 26, 27, 12 as INPUT_PULLUP
- Detect rotation direction (CW/CCW)
- Detect button press (SELECT function)
- Track position counter
```

**Success Criteria:**
- ✅ Clockwise rotation increases counter
- ✅ Counter-clockwise rotation decreases counter
- ✅ Encoder button press detected (dual function as SELECT)
- ✅ Smooth operation without skipping

**Hardware Check:**
- CLK connected to GPIO 26
- DT connected to GPIO 27
- SW connected to GPIO 12 (SELECT button)

**Estimated Time:** 30 minutes

---

## Phase 2: Communication Peripherals

**Goal:** Test all communication interfaces before integration.

### Step 2.1: I2C Scanner
**Objective:** Detect I2C devices and verify LCD address.

**Test File:** `test_sketches/03_test_i2c_scanner.ino`

**What to create:**
```cpp
- Scan I2C bus (addresses 0x00-0x7F)
- Report found devices
- Identify LCD address (typically 0x27 or 0x3F)
```

**Success Criteria:**
- ✅ I2C bus operational
- ✅ LCD detected at correct address
- ✅ No bus errors

**Hardware Check:**
- SDA connected to GPIO 21
- SCL connected to GPIO 22
- LCD powered (5V and GND)

**Estimated Time:** 15 minutes

**Required Library:** `Wire.h` (built-in)

---

### Step 2.2: LCD Display Test
**Objective:** Display text on LCD screen.

**Test File:** `test_sketches/04_test_lcd.ino`

**What to create:**
```cpp
- Initialize LCD with detected address
- Display static text on both lines
- Test LCD special characters
- Adjust contrast if needed
```

**Success Criteria:**
- ✅ LCD initializes successfully
- ✅ Text displays on Line 1
- ✅ Text displays on Line 2
- ✅ Characters are readable

**Hardware Check:**
- Adjust LCD contrast potentiometer for visibility

**Estimated Time:** 20 minutes

**Required Library:** `LiquidCrystal_I2C` (install from Library Manager)

---

### Step 2.3: WS2812B LED Strip Test
**Objective:** Control all 32 LEDs (4 strips × 8 LEDs).

**Test File:** `test_sketches/05_test_leds.ino`

**What to create:**
```cpp
- Initialize FastLED library
- Test all LEDs: Red, Green, Blue, White
- Test individual LED addressing (0-31)
- Test per-strip control (8 LEDs each)
- Create simple animations (rainbow, chase, pulse)
```

**Success Criteria:**
- ✅ All 32 LEDs light up
- ✅ Correct color display
- ✅ LEDs addressed correctly in sequence
- ✅ All 4 strips work independently

**Hardware Check:**
- Data line connected to GPIO 25
- All strips powered (5V and GND)
- Strips connected in series (DOUT → next DIN)
- Power distributed to all strips (star topology)

**Estimated Time:** 30 minutes

**Required Library:** `FastLED` (install from Library Manager)

---

### Step 2.4: Digital Scale Communication (RS232)
**Objective:** Read weight data from scale via RS232.

**Test File:** `test_sketches/06_test_scale.ino`

**What to create:**
```cpp
- Initialize UART2 on GPIO 16/17
- Read serial data from scale
- Parse weight format: "XX.XX g\r\n"
- Convert to float value
- Handle different units (g, kg, oz)
```

**Success Criteria:**
- ✅ Scale data received
- ✅ Weight values parsed correctly
- ✅ Updates in real-time
- ✅ Handles tare and negative values

**Hardware Check:**
- ⚠️ **CRITICAL:** MAX3232 converter module installed
- ⚠️ **NEVER connect RS232 (±12V) directly to ESP32!**
- Scale TX → MAX3232 R1IN → GPIO 16 (RX)
- Scale GND → MAX3232 GND → ESP32 GND
- MAX3232 powered correctly (check if 3.3V or 5V)

**Estimated Time:** 30 minutes

**Configuration:**
- Baud Rate: 9600
- Data Format: 8N1

---

## Phase 3: RS485 Communication with BTT Rodent

**Goal:** Establish reliable communication with the motor controller.

### Step 3.1: RS485 Basic Communication Test
**Objective:** Send and receive data via RS485.

**Test File:** `test_sketches/07_test_rs485_basic.ino`

**What to create:**
```cpp
- Initialize UART1 on GPIO 2/4 with RTS on GPIO 15
- Implement RTS direction control (HIGH=TX, LOW=RX)
- Send test string
- Receive echo or response
- Add timing delays for transceiver switching
```

**Success Criteria:**
- ✅ RS485 transceiver switches correctly
- ✅ Data transmitted without errors
- ✅ Responses received from Rodent board

**Hardware Check:**
- ⚠️ **CRITICAL:** MAX485 (or auto-direction) converter module installed
- ⚠️ **NEVER connect RS485 directly to ESP32!**
- ESP32 TX (GPIO 2) → RS485 DI
- ESP32 RX (GPIO 4) → RS485 RO
- ESP32 RTS (GPIO 15) → RS485 DE/RE (if manual direction module)
- RS485 A+ ↔ Rodent RS485 A+
- RS485 B- ↔ Rodent RS485 B-
- Common ground between all devices
- Use twisted pair cable for A+/B-

**Estimated Time:** 45 minutes

**Configuration:**
- Baud Rate: 115200
- Data Format: 8N1
- RTS switching delay: 10-100 microseconds

---

### Step 3.2: G-code Command Test
**Objective:** Send G-code commands and parse responses.

**Test File:** `test_sketches/08_test_gcode_commands.ino`

**What to create:**
```cpp
- Send basic G-code commands:
  - "?" (status query)
  - "$I" (system info)
  - "$$" (view settings)
  - "$X" (unlock)
- Parse responses:
  - "ok"
  - "<State|MPos:...>"
  - "error:X"
  - "ALARM:X"
- Implement timeout handling
```

**Success Criteria:**
- ✅ Commands sent successfully
- ✅ "ok" responses received
- ✅ Status query returns position data
- ✅ Error responses handled

**Hardware Check:**
- Rodent board powered (12V or 24V)
- USB connected to Rodent for monitoring (optional)

**Estimated Time:** 45 minutes

---

### Step 3.3: Single Motor Test
**Objective:** Command one motor to move.

**Test File:** `test_sketches/09_test_single_motor.ino`

**What to create:**
```cpp
- Unlock motors with "$X"
- Set incremental mode "G91"
- Command single axis movement:
  - "G1 X10 F100" (move X 10mm at 100mm/min)
- Verify motor rotates
- Test stop command "!"
- Test resume "~"
```

**Success Criteria:**
- ✅ Motor unlocks successfully
- ✅ Motor moves when commanded
- ✅ Correct direction
- ✅ Stops on command
- ✅ Resumes correctly

**Hardware Check:**
- Motor connected to X-axis port on Rodent
- Motor power supply adequate
- No mechanical binding

**Estimated Time:** 30 minutes

---

### Step 3.4: All Motors Test
**Objective:** Test all 4 motors independently and together.

**Test File:** `test_sketches/10_test_all_motors.ino`

**What to create:**
```cpp
- Test each motor independently:
  - X-axis (Pump 1 - DMDEE)
  - Y-axis (Pump 2 - T-12)
  - Z-axis (Pump 3 - T-9)
  - A-axis (Pump 4 - L25B)
- Test simultaneous motion:
  - "G1 X10 Y10 Z10 A10 F100"
- Implement emergency stop
```

**Success Criteria:**
- ✅ All 4 motors move correctly
- ✅ Each motor moves independently
- ✅ Simultaneous motion works
- ✅ Emergency stop works instantly

**Hardware Check:**
- All 4 motors connected and powered
- No mechanical interference between pumps

**Estimated Time:** 45 minutes

---

## Phase 4: Pump Control Logic

**Goal:** Implement pump control with flow rate calculations.

### Step 4.1: Flow Rate Calculation
**Objective:** Convert flow rate (ml/min) to feedrate (mm/min).

**Test File:** `test_sketches/11_test_flow_rate.ino`

**What to create:**
```cpp
- Define calibration constants (ml/mm for each pump)
- Function: flow_rate_to_feedrate()
  - Input: desired flow rate (ml/min)
  - Output: G-code feedrate (mm/min)
- Test with different values
- Validate range limits
```

**Formula:**
```
Feedrate (mm/min) = Flow_Rate (ml/min) / Calibration (ml/mm)
```

**Success Criteria:**
- ✅ Accurate conversion calculations
- ✅ Range limiting works (min/max speeds)
- ✅ All 4 pumps have calibration values

**Estimated Time:** 30 minutes

---

### Step 4.2: Single Pump Start/Stop
**Objective:** Start and stop a pump at specific flow rate.

**Test File:** `test_sketches/12_test_pump_control.ino`

**What to create:**
```cpp
- Function: start_pump(pump_id, flow_rate)
  - Calculate feedrate
  - Build G-code command
  - Send via RS485
- Function: stop_pump(pump_id)
  - Send feed hold "!"
- Function: stop_all_pumps()
  - Emergency stop all
```

**Success Criteria:**
- ✅ Pump starts at commanded flow rate
- ✅ Pump stops cleanly
- ✅ Flow rate is accurate (verify with scale)

**Estimated Time:** 45 minutes

---

### Step 4.3: Multi-Pump Coordination
**Objective:** Run multiple pumps at different flow rates simultaneously.

**Test File:** `test_sketches/13_test_multi_pump.ino`

**What to create:**
```cpp
- Start multiple pumps with different flow rates
- Monitor all pump states
- Stop specific pumps without affecting others
- Handle state tracking for 4 pumps
```

**Success Criteria:**
- ✅ All pumps run simultaneously
- ✅ Independent flow rate control
- ✅ Accurate state tracking
- ✅ Clean stop of individual pumps

**Estimated Time:** 1 hour

---

### Step 4.4: Weight-Based Dosing (Single Pump)
**Objective:** Dispense specific amount by weight using scale feedback.

**Test File:** `test_sketches/14_test_weight_dosing.ino`

**What to create:**
```cpp
- Read scale continuously
- Start pump
- Monitor weight increase
- Stop pump when target reached
- Handle tolerance and overshoot
```

**Success Criteria:**
- ✅ Accurate dosing to target weight
- ✅ Minimal overshoot (<2%)
- ✅ Handles scale noise/fluctuation
- ✅ Stops cleanly at target

**Estimated Time:** 1 hour

---

## Phase 5: User Interface Integration

**Goal:** Create complete user interface with buttons, encoder, LCD, and LEDs.

### Step 5.1: Menu System
**Objective:** Navigate menus using encoder and display on LCD.

**Test File:** `test_sketches/15_test_menu_system.ino`

**What to create:**
```cpp
- Menu structure:
  - Main Menu
    - Start Dosing
    - Select Recipe
    - Settings
    - Calibration
- Encoder rotation navigates menu
- SELECT button (encoder button) confirms selection
- LCD displays current menu item
```

**Success Criteria:**
- ✅ Smooth menu navigation
- ✅ SELECT button confirms choices
- ✅ LCD updates correctly
- ✅ Intuitive user experience

**Estimated Time:** 1.5 hours

---

### Step 5.2: LED Status Indicators
**Objective:** Use LED strips to show pump status.

**Test File:** `test_sketches/16_test_led_status.ino`

**What to create:**
```cpp
- LED patterns for each state:
  - Idle: Slow pulse
  - Running: Solid color
  - Paused: Pulsing yellow
  - Complete: Solid green
  - Error: Flashing red
- Different color for each pump strip
- Progress bar visualization (8 LEDs per pump)
```

**Success Criteria:**
- ✅ Clear visual feedback for each state
- ✅ Each pump has independent LED control
- ✅ Animations smooth and clear
- ✅ Easy to understand at a glance

**Estimated Time:** 1 hour

---

### Step 5.3: Button Integration
**Objective:** Wire buttons to control system state.

**Test File:** `test_sketches/17_test_button_control.ino`

**What to create:**
```cpp
- START button: Begin dosing operation
- STOP button: Emergency stop
- MODE button: Cycle through operating modes
- State machine implementation
- Button debouncing
```

**Success Criteria:**
- ✅ START begins operation
- ✅ STOP immediately halts pumps
- ✅ MODE switches between modes
- ✅ No false triggers

**Estimated Time:** 45 minutes

---

## Phase 6: Recipe Management

**Goal:** Store and execute dosing recipes.

### Step 6.1: Simple Recipe Execution
**Objective:** Execute a pre-defined recipe.

**Test File:** `test_sketches/18_test_simple_recipe.ino`

**What to create:**
```cpp
- Recipe structure:
  struct Recipe {
    char name[20];
    float pump1_ml;
    float pump2_ml;
    float pump3_ml;
    float pump4_ml;
    float flow_rate;
  };
- Execute recipe:
  - Start pumps in sequence
  - Monitor weights
  - Stop when targets reached
- Display progress on LCD and LEDs
```

**Success Criteria:**
- ✅ Recipe executes correctly
- ✅ All pumps dispense correct amounts
- ✅ Progress displayed clearly
- ✅ Completes successfully

**Estimated Time:** 1.5 hours

---

### Step 6.2: Recipe Selection
**Objective:** Select from multiple stored recipes.

**Test File:** `test_sketches/19_test_recipe_selection.ino`

**What to create:**
```cpp
- Store multiple recipes (5-10 recipes)
- Menu to browse recipes
- Display recipe details:
  - Name
  - Ingredients (pump amounts)
  - Total volume
  - Estimated time
- Confirm and execute selected recipe
```

**Success Criteria:**
- ✅ Browse through recipe list
- ✅ View recipe details
- ✅ Execute any selected recipe
- ✅ Easy to use interface

**Estimated Time:** 2 hours

---

## Phase 7: Main Application Integration

**Goal:** Combine all features into complete system.

### Step 7.1: State Machine
**Objective:** Implement complete system state machine.

**Test File:** `main/main.ino`

**What to create:**
```cpp
- States:
  - IDLE: Waiting for user input
  - MENU: Navigating menus
  - DOSING: Active dispensing
  - PAUSED: Operation paused
  - COMPLETE: Recipe finished
  - ERROR: Error state
- State transitions
- Timeout handling
- Error recovery
```

**Success Criteria:**
- ✅ Clean state transitions
- ✅ All states work correctly
- ✅ Proper error handling
- ✅ Graceful recovery from errors

**Estimated Time:** 2 hours

---

### Step 7.2: Full System Test
**Objective:** Test complete system with real-world scenario.

**Test Procedure:**
```
1. Power on system
   - Verify boot sequence
   - Check all peripherals initialize

2. Navigate menu
   - Select recipe
   - Review recipe details

3. Prepare for dosing
   - Place container on scale
   - Tare scale
   - Verify ready state

4. Execute dosing
   - Press START
   - Monitor progress (LCD + LEDs)
   - Observe all pumps
   - Watch scale weight increase

5. Complete operation
   - Verify final weights
   - Check completion indicator
   - Remove container

6. Test error scenarios
   - Press STOP during operation
   - Remove scale power
   - Test emergency stop
```

**Success Criteria:**
- ✅ Complete recipe executes successfully
- ✅ All displays update correctly
- ✅ Accurate dosing (±2% tolerance)
- ✅ Error handling works
- ✅ User experience is smooth

**Estimated Time:** 2-3 hours of testing

---

## Phase 8: Advanced Features (Optional)

### Step 8.1: WiFi and MQTT Integration
- Connect to WiFi network
- Publish pump status to MQTT broker
- Remote monitoring capability
- Integration with Home Assistant or similar

**Reference:** `docs/integration/MQTT_SETUP_README.md`

**Estimated Time:** 3-4 hours

---

### Step 8.2: Data Logging
- Log all operations to SD card or cloud
- Track pump usage statistics
- Recipe execution history
- Error logs

**Estimated Time:** 2-3 hours

---

### Step 8.3: Web Interface
- Create web dashboard
- Remote recipe management
- Live monitoring
- Configuration interface

**Reference:** `docs/reference/WEB_UI_README.md`

**Estimated Time:** 4-6 hours

---

## Development Timeline Estimate

| Phase | Description | Time Estimate | Cumulative |
|-------|-------------|---------------|------------|
| Phase 1 | Basic Hardware Testing | 1-2 hours | 1-2 hours |
| Phase 2 | Communication Peripherals | 2-3 hours | 3-5 hours |
| Phase 3 | RS485 & Motor Control | 3-4 hours | 6-9 hours |
| Phase 4 | Pump Control Logic | 3-4 hours | 9-13 hours |
| Phase 5 | User Interface | 3-4 hours | 12-17 hours |
| Phase 6 | Recipe Management | 3-4 hours | 15-21 hours |
| Phase 7 | Main Application | 4-6 hours | 19-27 hours |
| Phase 8 | Advanced Features | 9-13 hours | 28-40 hours |

**Minimum Viable Product:** Phases 1-7 (19-27 hours)
**Complete System:** All Phases (28-40 hours)

---

## Critical Safety Reminders

### ⚠️ Before Powering On:
1. **Double-check all voltage levels**
   - RS232: Must use MAX3232 converter (±12V → 3.3V TTL)
   - RS485: Must use MAX485 converter (differential → 3.3V TTL)
   - Never connect RS232 or RS485 directly to ESP32!

2. **Verify common ground**
   - All devices must share common ground
   - Check continuity with multimeter

3. **Check power supply voltages**
   - ESP32: 5V via USB or external
   - Rodent Board: 12V or 24V (per motor specification)
   - LEDs: 5V DC
   - Scale: Battery or USB (self-powered)

4. **Inspect all connections**
   - No shorts between power and ground
   - All wires properly insulated
   - Solid solder joints (no cold joints)

---

## Development Best Practices

### 1. Version Control
- Commit after each working phase
- Tag major milestones
- Keep test sketches separate from main code

### 2. Documentation
- Comment all magic numbers
- Document calibration values
- Note hardware issues encountered

### 3. Testing
- Never skip a test phase
- Document test results
- Keep a lab notebook

### 4. Debugging
- Use Serial.print extensively
- Monitor all serial channels when troubleshooting
- Use oscilloscope/logic analyzer for timing issues

---

## Troubleshooting Guide

If you encounter issues, refer to:
- `docs/hardware/WIRING_GUIDE.md` - Complete wiring instructions
- `docs/setup/FLUIDNC_SETUP_GUIDE.md` - Rodent board setup
- `docs/setup/SCALE_SETUP_GUIDE.md` - Scale configuration
- `docs/reference/GCODE_COMMAND_REFERENCE.md` - G-code details

Common issues and solutions are documented in each test sketch.

---

## Next Steps

1. **Review hardware connections** using `docs/hardware/HARDWARE_OVERVIEW.md`
2. **Start with Phase 1, Step 1.1** (Blink test)
3. **Test each component individually** before moving forward
4. **Document any issues** you encounter
5. **Ask for help** if stuck on any phase

---

## Conclusion

This incremental approach ensures:
- ✅ Each component is verified before integration
- ✅ Problems are isolated and easier to debug
- ✅ You build confidence as you progress
- ✅ System is reliable and maintainable

**Remember:** Take your time, test thoroughly, and don't skip phases!

---

**Document Version:** 1.0
**Last Updated:** 2025-11-11
**Status:** Ready to begin Phase 1
