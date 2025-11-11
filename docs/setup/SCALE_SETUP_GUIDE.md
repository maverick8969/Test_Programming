# Scale Setup Guide

This guide explains how to configure your scale for use with the peristaltic pump controller.

## Scale Protocol Specifications

The scale communicates using the following protocol:

- **Format**: `25.34 g<CR><LF>` (weight value, space, unit, carriage return, line feed)
- **Baud Rate**: 9600
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Mode**: Manual print (triggered by button press) or Continuous mode

## Hardware Connections

The scale connects to UART2 on the ESP32:

- **RX Pin**: GPIO 16
- **TX Pin**: GPIO 17

Refer to `include/config.h` for pin configurations.

## Configuring Scale to Continuous Output Mode

Since the scale does not have a dedicated menu button, you'll need to use a button combination or long press to access the configuration menu.

### Method 1: Long Press (Most Common)

1. **Power on the scale** (if not already on)
2. **Long press the TARE button** (hold for 3-5 seconds)
   - Watch for the display to change or show a menu indicator
   - Some scales may show "CAL", "FUNC", or "SETUP"
3. **Navigate the menu**:
   - Use the TARE button to cycle through menu options
   - Look for options like "MODE", "OUTPUT", "PRINT", or "CONT"
4. **Select Continuous Mode**:
   - When you find the output mode setting, press the appropriate button to enter
   - Select "CONTINUOUS" or "AUTO PRINT" mode
   - The scale may display "CONT", "AUTO", or similar
5. **Save and Exit**:
   - Long press the TARE button again to save and exit
   - Or wait for the menu to timeout (usually 10-30 seconds)

### Method 2: Multi-Button Press

If long press doesn't work, try these combinations:

1. **TARE + ON/OFF simultaneously**:
   - Power off the scale
   - Hold TARE and press ON/OFF
   - Release after menu appears

2. **TARE + ZERO simultaneously**:
   - Hold both buttons for 3-5 seconds
   - Look for menu access

3. **UNIT + TARE simultaneously**:
   - Some scales use this combination
   - Hold for 3-5 seconds

### Menu Navigation Tips

Once in the menu:

- **Cycle options**: Usually TARE or UNIT button
- **Select/Enter**: Usually ZERO or short press of the button held to enter menu
- **Exit**: Long press the entry button, or wait for timeout

### What to Look For

Common menu settings you may encounter:

- **Output Mode**: Set to "CONTINUOUS" or "AUTO"
- **Print Interval**: If available, set to fastest (e.g., 500ms or 1 second)
- **Baud Rate**: Verify it's set to **9600** (this is critical)
- **Data Format**: Should show weight + unit (e.g., "25.34 g")

## Verification

After configuration:

1. **Connect the scale** to the ESP32 UART2 pins
2. **Power on both devices**
3. **Monitor the serial output** from the ESP32
4. You should see log messages like:
   ```
   [SCALE] Received data (10 bytes): 25.34 g
   [SCALE] Parsed weight: 25.34 g
   ```

## Troubleshooting

### No Data Received

- **Check wiring**: Ensure RX/TX are not swapped
- **Verify baud rate**: Must be 9600 on both scale and ESP32
- **Check scale mode**: Must be in continuous output mode
- **Test with PRINT button**: Press the print button to verify manual output works

### Garbled Data

- **Baud rate mismatch**: Verify scale is set to 9600
- **Wiring issue**: Check for loose connections or interference
- **Ground connection**: Ensure GND is connected between scale and ESP32

### Incorrect Weight Values

- **Unit conversion**: Check if scale is outputting kg/mg instead of g
- **Tare the scale**: Press TARE to zero the scale
- **Calibration**: Scale may need recalibration (refer to scale manual)

## Manual Print Mode (Current Default)

If you cannot configure continuous mode, the scale will work in manual print mode:

- **Press the PRINT button** on the scale to send a weight reading
- The ESP32 will read and parse the data when it arrives
- This mode works but requires manual button presses for each reading

## Scale Models

This configuration has been tested with scales using the standard serial output format. Many digital scales from manufacturers like:

- OHAUS
- A&D Weighing
- Mettler Toledo
- Adam Equipment
- Generic bench scales with RS232 output

Use similar protocols. Consult your scale's manual for specific menu access instructions.

## Additional Resources

- **Scale Manual**: Check the manufacturer's manual for menu access details
- **Config File**: `include/config.h` - Contains pin and baud rate settings
- **Scale Driver**: `src/scale.cpp` - Implementation of protocol parsing
- **Protocol Format**: See comments in `scale_read_weight()` function
