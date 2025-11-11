# Boot-Safe Pin Assignment Changes

**Version:** 1.1
**Date:** 2025-11-11
**Issue:** Original pin assignments used ESP32 strapping pins that interfere with boot sequence

---

## Problem Summary

The original pin assignments (v1.0) used three **strapping pins** that control ESP32 boot behavior:

| GPIO | Original Use | Boot Issue |
|------|-------------|------------|
| **GPIO 2** | RS485 TX (Rodent) | Must be LOW/floating during boot. If RS485 transceiver pulls HIGH → boot failure |
| **GPIO 12** | Encoder Button (SELECT) | Controls flash voltage (3.3V vs 1.8V). If HIGH during boot → flash voltage issues |
| **GPIO 15** | RS485 RTS | Controls boot messages. Should be HIGH. Usually okay but can be sensitive |

### Why This Matters

If any peripheral (RS485 transceiver, encoder module, etc.) pulls these pins to the wrong state during power-on:
- ❌ ESP32 may fail to boot
- ❌ ESP32 may enter download mode unintentionally
- ❌ Flash memory may be damaged (GPIO 12 issue)
- ❌ Unreliable startup behavior

**User feedback: "is gpio 2 safe to use, doesn't it interact with the boot sequence?"** - Absolutely correct! ✅

---

## Solution: Boot-Safe Pin Reassignment (v1.1)

### Complete Pin Changes

| Function | Old Pin (v1.0) | New Pin (v1.1) | Change Reason |
|----------|----------------|----------------|---------------|
| **RS485 (Rodent Board)** | | | |
| RS485 TX | GPIO 2 ⚠️ | GPIO 17 ✅ | Eliminate boot strapping pin |
| RS485 RX | GPIO 4 | GPIO 16 ✅ | Keep safe (moved to match UART2) |
| RS485 RTS | GPIO 15 ⚠️ | GPIO 4 ✅ | Eliminate boot strapping pin |
| UART Port | UART1 | UART2 | Better organization |
| **RS232 (Scale)** | | | |
| Scale RX | GPIO 16 | GPIO 35 ✅ | Input-only pin (perfect for RX) |
| Scale TX | GPIO 17 | GPIO 32 ✅ | Safe GPIO |
| UART Port | UART2 | UART1 | Better organization |
| **Encoder** | | | |
| Encoder CLK | GPIO 26 ✅ | GPIO 26 ✅ | No change (safe) |
| Encoder DT | GPIO 27 ✅ | GPIO 27 ✅ | No change (safe) |
| Encoder SW (SELECT) | GPIO 12 ⚠️ | GPIO 34 ✅ | Input-only pin (boot-safe) |
| **Other** | | | |
| START Button | GPIO 13 ✅ | GPIO 13 ✅ | No change (safe) |
| MODE Button | GPIO 14 ✅ | GPIO 14 ✅ | No change (safe) |
| STOP Button | GPIO 33 ✅ | GPIO 33 ✅ | No change (safe) |
| LCD SDA | GPIO 21 ✅ | GPIO 21 ✅ | No change (safe) |
| LCD SCL | GPIO 22 ✅ | GPIO 22 ✅ | No change (safe) |
| LED Data | GPIO 25 ✅ | GPIO 25 ✅ | No change (safe) |

---

## ESP32 Strapping Pins Reference

### All Strapping Pins (AVOID in general)

| GPIO | Boot Function | Required State | What Happens If Wrong |
|------|---------------|----------------|----------------------|
| **GPIO 0** | Boot mode select | HIGH = normal boot<br>LOW = download mode | Wrong mode entered |
| **GPIO 2** | Boot mode select | LOW/floating = normal boot<br>HIGH = special mode | Boot failure or wrong mode |
| **GPIO 5** | SDIO timing | (context dependent) | Usually okay, but can cause issues |
| **GPIO 12** | Flash voltage select | LOW = 3.3V flash (most boards)<br>HIGH = 1.8V flash | Flash damage or read errors |
| **GPIO 15** | Boot message output | HIGH = normal messages<br>LOW = silent boot | Messages suppressed (usually okay) |

### Safe GPIO Pins for General Use

**Fully safe bidirectional pins:**
- GPIO 4, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33

**Input-only pins (perfect for buttons/RX):**
- GPIO 34, 35, 36, 39 *(no internal pull-up/pull-down available)*

---

## Important Notes About Input-Only Pins

### GPIO 34, 35, 36, 39 Characteristics:

✅ **Perfect for:**
- Button inputs (with external pull-up)
- UART RX (receive only)
- Analog input (ADC)
- Any read-only signal

❌ **Cannot be used for:**
- Output signals
- I2C (needs bidirectional)
- UART TX (transmit)

⚠️ **No internal pull-up/pull-down available!**
- Must use external resistor if needed
- Example: Button with 10kΩ pull-up to 3.3V

---

## Hardware Changes Required

### For RS485 (Rodent Board) - MAX485 Module

**Old Wiring (v1.0):**
```
ESP32 GPIO 2  → MAX485 DI  (BOOT ISSUE!)
ESP32 GPIO 4  → MAX485 RO
ESP32 GPIO 15 → MAX485 DE/RE  (BOOT ISSUE!)
```

**New Wiring (v1.1):**
```
ESP32 GPIO 17 → MAX485 DI  ✅ Boot-safe
ESP32 GPIO 16 → MAX485 RO  ✅ Boot-safe
ESP32 GPIO 4  → MAX485 DE/RE  ✅ Boot-safe
```

### For RS232 (Scale) - MAX3232 Module

**Old Wiring (v1.0):**
```
ESP32 GPIO 16 (RX) ← MAX3232 R1OUT
ESP32 GPIO 17 (TX) → MAX3232 T1IN
```

**New Wiring (v1.1):**
```
ESP32 GPIO 35 (RX) ← MAX3232 R1OUT  ✅ Input-only, perfect for RX
ESP32 GPIO 32 (TX) → MAX3232 T1IN  ✅ Boot-safe
```

### For Rotary Encoder

**Old Wiring (v1.0):**
```
Encoder CLK → GPIO 26  ✅
Encoder DT  → GPIO 27  ✅
Encoder SW  → GPIO 12  ⚠️ BOOT ISSUE!
```

**New Wiring (v1.1):**
```
Encoder CLK → GPIO 26  ✅
Encoder DT  → GPIO 27  ✅
Encoder SW  → GPIO 34  ✅ Boot-safe
               ↑
               [10kΩ pull-up to 3.3V]  ⚠️ Required! GPIO 34 has no internal pull-up
```

**Important:** Add external 10kΩ resistor from GPIO 34 to 3.3V, or use encoder module with built-in pull-up.

---

## Software Changes

### Files Modified:

1. **`test/common/pin_definitions.h`** (v1.1)
   - Updated all pin assignments
   - Added boot-safe pin documentation
   - Added `configure_input_only_gpio()` helper function
   - Added revision summary

2. **`test/test_02_encoder/test_02_encoder.c`**
   - Updated to use GPIO 34 for encoder button
   - Added warnings about external pull-up requirement
   - Uses `configure_input_only_gpio()` for GPIO 34

3. **Future test programs** (Phase 2+)
   - Will use new UART assignments (UART1 ↔ UART2 swapped)
   - RS485 code will use GPIO 17/16/4 instead of 2/4/15
   - Scale code will use GPIO 35/32 instead of 16/17

### Code Example - Old vs New:

**Old (v1.0) - Boot issues:**
```c
#define RODENT_TX_PIN   GPIO_NUM_2   // ⚠️ Boot strapping pin!
#define ENCODER_SW_PIN  GPIO_NUM_12  // ⚠️ Flash voltage pin!
```

**New (v1.1) - Boot safe:**
```c
#define RODENT_TX_PIN   GPIO_NUM_17  // ✅ Safe
#define ENCODER_SW_PIN  GPIO_NUM_34  // ✅ Safe (input-only)
```

---

## Testing Checklist

After implementing these changes:

### ✅ Test 00: Blink
- No changes needed (uses GPIO 2 for built-in LED only, acceptable for basic test)
- ✅ Should still work

### ✅ Test 01: Buttons
- No changes needed (GPIO 13, 14, 33 are all safe)
- ✅ Should still work

### ⚠️ Test 02: Encoder
- **Changed:** Encoder button moved from GPIO 12 → GPIO 34
- **Action required:** Add external 10kΩ pull-up resistor from GPIO 34 to 3.3V
- **Wiring update:** Update encoder button connection to GPIO 34
- Test encoder rotation (should work)
- Test encoder button press (needs external pull-up)

### 🔧 Phase 2 Tests (Future)
- **Test 06: Scale** - Update to use GPIO 35 (RX) and GPIO 32 (TX)
- **Test 07-10: RS485/Motors** - Update to use GPIO 17 (TX), GPIO 16 (RX), GPIO 4 (RTS)

---

## Migration Guide for Users

If you've already wired your hardware using v1.0 pin assignments:

### Option 1: Re-wire (Recommended)
- Follow new wiring guide in `docs/hardware/WIRING_GUIDE.md` (to be updated)
- Use new boot-safe pins
- Add 10kΩ pull-up resistor for GPIO 34 (encoder button)

### Option 2: Keep Old Wiring (Not Recommended)
- Possible but risky
- May experience boot issues
- Add pull-down resistor on GPIO 2 to ensure LOW during boot
- Add pull-down resistor on GPIO 12 to ensure LOW during boot
- Not guaranteed to work reliably

**We strongly recommend Option 1 (re-wiring) for reliable operation.**

---

## Benefits of New Pin Assignment

✅ **Reliable Booting**
- ESP32 boots consistently regardless of peripheral states
- No need to disconnect hardware during programming
- No boot mode confusion

✅ **Better Organization**
- Input-only pins (34, 35) used for input signals (buttons, RX)
- Bidirectional pins used appropriately
- Clear separation of concerns

✅ **Future-Proof**
- No conflicts with ESP32 boot mechanism
- Compatible with all ESP32 modules and boards
- Easier debugging and development

✅ **Production-Ready**
- No special considerations during manufacturing
- Reliable field deployment
- No customer boot issues

---

## Updated Pin Summary (Quick Reference)

### Communication
```
RS485 (Rodent):  TX=17, RX=16, RTS=4  (UART2, 115200 baud)
RS232 (Scale):   TX=32, RX=35         (UART1, 9600 baud)
I2C (LCD):       SDA=21, SCL=22
LED (WS2812B):   Data=25
```

### User Interface
```
Buttons:         START=13, MODE=14, STOP=33
Encoder:         CLK=26, DT=27, SW=34  (⚠️ GPIO 34 needs 10kΩ pull-up!)
```

### Notes
- All bidirectional pins support internal pull-ups except GPIO 34, 35
- GPIO 34, 35 are input-only - add external 10kΩ pull-up if needed
- All pins are boot-safe - no strapping pin conflicts

---

## References

- **ESP32 Datasheet:** Section on "Strapping Pins"
- **ESP32 Technical Reference:** GPIO and IO MUX chapter
- **pin_definitions.h:** Complete annotated pin mapping
- **Espressif Documentation:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-11-11 | Initial pin assignment (had boot issues) |
| 1.1 | 2025-11-11 | Boot-safe redesign - moved away from strapping pins |

---

**Status:** ✅ Implemented and ready for testing
**Impact:** Low (only Test 02 hardware change needed)
**Benefit:** High (reliable booting)
