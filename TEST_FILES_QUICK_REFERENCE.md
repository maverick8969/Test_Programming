# Test Files Quick Reference

## All Test Programs (21 files total)

| Test | File | Purpose | Hardware Required | Complexity |
|------|------|---------|-------------------|------------|
| 00 | test_00_blink.cpp | Basic ESP32 verification | LED only | ⭐ |
| 01 | test_01_buttons.cpp | Button input testing | 3 buttons | ⭐ |
| 02 | test_02_encoder.cpp | Rotary encoder | Encoder (needs ext pull-up on SW!) | ⭐ |
| 03 | test_03_i2c_scanner.cpp | I2C device discovery | I2C bus | ⭐ |
| 04 | test_04_lcd.cpp | LCD display | I2C LCD | ⭐⭐ |
| 05 | test_05_leds.cpp | WS2812B LED control | 32 RGB LEDs | ⭐⭐ |
| 06 | test_06_scale.cpp | Digital scale reading | Scale + MAX3232 | ⭐⭐⭐ |
| 07 | test_07_rs485.cpp | RS485 to Rodent | Rodent + MAX485 | ⭐⭐⭐ |
| 08 | test_08_uart.cpp | Direct UART to Rodent | Rodent (no RS485) | ⭐⭐ |
| 09 | test_09_uart_buttons.cpp | Buttons + UART control | Buttons + Encoder + Rodent | ⭐⭐⭐ |
| 10 | test_10_uart_lcd.cpp | LCD + Encoder + UART | LCD + Encoder + Rodent | ⭐⭐⭐⭐ |
| 11 | test_11_uart_leds.cpp | Full UI with automation | LEDs + LCD + Encoder + Rodent | ⭐⭐⭐⭐ |
| 12 | test_12_single_pump.cpp | Single pump flow control | All above + calibration | ⭐⭐⭐ |
| 13 | test_13_multi_sequential.cpp | Sequential recipes | All above | ⭐⭐⭐⭐ |
| 14 | test_14_multi_simultaneous.cpp | Simultaneous pumps | All above | ⭐⭐⭐⭐ |
| 15 | test_15_scale_integration.cpp | Weight-based dispensing | All + Scale | ⭐⭐⭐⭐⭐ |
| 16 | test_16_recipe_system.cpp | Recipe management | All UI components | ⭐⭐⭐⭐⭐ |
| 17 | test_17_safety_features.cpp | Emergency stop & safety | LEDs + Buttons + Rodent | ⭐⭐⭐ |
| 18 | test_18_data_logging.cpp | Command logging & stats | Rodent | ⭐⭐⭐ |
| 19 | test_19_full_integration.cpp | **COMPLETE SYSTEM** | **ALL HARDWARE** | ⭐⭐⭐⭐⭐⭐ |
| 20 | test_20_led_motor_status.cpp | Motor status visualization | LEDs + Rodent (RS485/UART) | ⭐⭐⭐⭐ |

## Compilation Instructions

### To compile a specific test:

1. Edit `platformio.ini`
2. Change `build_src_filter` to select the test:
   ```ini
   build_src_filter =
       +<test_XX_name.cpp>
       -<*.cpp>
   ```
3. Run: `pio run`
4. Upload: `pio run --target upload`
5. Monitor: `pio device monitor`

### Example for Test 19 (Full Integration):
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

## Required Libraries

Install via PlatformIO:
```bash
pio lib install "fastled/FastLED@^3.5.0"
pio lib install "marcoschwartz/LiquidCrystal_I2C@^1.1.4"
```

Or add to `platformio.ini`:
```ini
lib_deps =
    fastled/FastLED@^3.5.0
    marcoschwartz/LiquidCrystal_I2C@^1.1.4
```

## Critical Wiring Notes

### Must-Know Before Testing:

1. **GROUND CONNECTIONS ARE CRITICAL!**
   - Connect GND between ESP32 and ALL peripherals
   - No common ground = erratic behavior

2. **GPIO 34 (Encoder SW) needs EXTERNAL 10kΩ pull-up**
   - Input-only GPIO, no internal pull-up available
   - Connect 10kΩ resistor from GPIO 34 to 3.3V

3. **WS2812B LEDs require WiFi/BT disabled**
   - Timing interference causes data corruption
   - All LED tests include WiFi.mode(WIFI_OFF)

4. **Scale uses literal text command**
   - Command is "@P<CR><LF>" (10 ASCII bytes)
   - NOT "@P\r\n" (4 bytes with control chars)

5. **LED power supply**
   - Use separate 5V supply for LEDs (not ESP32 VIN)
   - Common ground to ESP32

## Testing Progression

### Recommended Order:

**Phase 1: Verify Hardware**
1. Test 00 → Verify ESP32
2. Test 01 → Verify buttons
3. Test 02 → Verify encoder (**check external pull-up!**)
4. Test 03 → Scan I2C bus
5. Test 04 → Verify LCD
6. Test 05 → Verify LEDs (**WiFi/BT disabled?**)

**Phase 2: Communication**
7. Test 08 → Verify UART to Rodent (**common ground?**)
8. Test 06 → Verify scale (if using)

**Phase 3: Integration**
9. Test 09 → Basic control
10. Test 10 → LCD feedback
11. Test 11 → Full automation test

**Phase 4: Pump Control**
12. Test 12 → Calibrate single pump
13. Test 13 → Sequential recipes
14. Test 14 → Simultaneous pumps

**Phase 5: Advanced**
15. Test 15 → Weight-based (if using scale)
16. Test 16 → Recipe system
17. Test 17 → Safety features
18. Test 18 → Data logging

**Phase 6: Final**
19. Test 19 → **FULL INTEGRATION TEST**
20. Test 20 → Motor status visualization

## Production Deployment

**Recommended Base**: Test 19

**Optional Enhancements**:
- Add Test 18 data logging features
- Add Test 20 motor status LEDs
- Add Test 15 weight mode (if scale available)
- Add Test 16 enhanced recipe management

## Common Issues & Solutions

| Issue | Cause | Fix |
|-------|-------|-----|
| Random LED colors | WiFi/BT interference | Verify WiFi.mode(WIFI_OFF) called |
| Scale no response | Wrong command | Use "@P<CR><LF>" not "\r\n" |
| Encoder SW not working | No pull-up | Add 10kΩ GPIO34→3.3V |
| UART no response | No ground | Connect ESP32 GND to Rodent GND |
| LCD not found | Wrong address | Run Test 03 to find address |
| Compile error | Wrong build filter | Check platformio.ini filter |
| Data log spam | Verbose mode | Type 'v' to toggle off |

## Serial Monitor Commands

**Test 18 (Data Logging)**:
- `l` - Show log
- `s` - Show statistics
- `c` - Clear log
- `v` - Toggle verbose
- `?` - Query status

**FluidNC Commands** (Tests 07-20):
- `$I` - System info
- `$$` - Settings
- `?` - Status
- `!` - Emergency stop
- `~` - Resume
- `Ctrl-X` - Reset
- `$X` - Unlock

## File Status

✅ All 21 test files present and complete
✅ All critical bugs fixed
✅ All features documented
✅ Production configuration defined

**Last Updated**: 2025-11-18
