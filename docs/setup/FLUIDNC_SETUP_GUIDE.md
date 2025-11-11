# FluidNC BTT Rodent Configuration Guide

This guide explains how to use the FluidNC configuration file with the BTT Rodent board and ESP32 integration via UART.

## Overview

This configuration allows you to control 4 independent peristaltic pumps using FluidNC firmware on an ESP32, which communicates with a BTT Rodent CNC controller board via UART. The system uses all 4 stepper driver channels (X, Y, Z, A axes) on the BTT Rodent board.

## Hardware Requirements

1. **ESP32 Development Board** - Running FluidNC firmware
2. **BTT Rodent CNC Controller Board** - Main controller with 4 stepper drivers
3. **4x Stepper Motors** - For pump control (NEMA 17 or similar)
4. **Power Supply** - 12V or 24V depending on your motors (sufficient current for 4 motors)
5. **Wiring** - Dupont cables for UART connection between ESP32 and BTT Rodent

## Pin Connections

### ESP32 to BTT Rodent RS485 Connection

| ESP32 Pin | BTT Rodent Pin | Function |
|-----------|----------------|----------|
| GPIO 2    | A+ (RS485)     | TX Data  |
| GPIO 4    | B- (RS485)     | RX Data  |
| GPIO 15   | -              | RTS (Direction Control) |
| GND       | GND            | Ground   |

**Important:**
- This connection uses RS485, not direct UART
- An RS485 transceiver module (MAX485 or similar) is required on the ESP32 side
- GPIO 15 controls the RS485 transceiver direction (TX/RX mode)
- Ensure both boards share a common ground!

### ESP32 GPIO Mapping (Current System)

| GPIO Pin | Function           | Description                    |
|----------|--------------------|--------------------------------|
| GPIO 2   | Rodent UART TX     | RS485 transmit to Rodent board |
| GPIO 4   | Rodent UART RX     | RS485 receive from Rodent board|
| GPIO 15  | RS485 RTS          | RS485 direction control        |
| GPIO 16  | Scale UART RX      | Receive from scale             |
| GPIO 17  | Scale UART TX      | Transmit to scale              |
| GPIO 21  | LCD I2C SDA        | LCD display data               |
| GPIO 22  | LCD I2C SCL        | LCD display clock              |
| GPIO 25  | LED Data           | WS2812B LED strip (all pumps)  |
| GPIO 13  | START Button       | Start operation button         |
| GPIO 14  | MODE Button        | Mode selection button          |
| GPIO 33  | STOP Button        | Stop/Cancel button             |
| GPIO 12  | Encoder SW (SELECT)| Rotary encoder switch / SELECT |
| GPIO 26  | Encoder CLK        | Rotary encoder clock           |
| GPIO 27  | Encoder DT         | Rotary encoder data            |

**Note:** The BTT Rodent board running FluidNC controls the stepper motors directly. The ESP32 sends G-code commands via RS485 to control pump operation.

## Installation Steps

### 1. Configure FluidNC on BTT Rodent Board

The BTT Rodent board should come pre-installed with FluidNC firmware. If you need to update or configure it:

```bash
# Connect to Rodent board via USB
# Access FluidNC web interface or use a serial terminal

# Alternatively, flash FluidNC to Rodent board
# Visit: https://fluidnc.com/install/
# Follow BTT Rodent specific instructions
```

### 2. Upload Configuration File to Rodent Board

Once FluidNC is running on the BTT Rodent board:

#### Option A: Via Web Interface
1. Connect to ESP32 WiFi access point (default: FluidNC)
2. Navigate to http://192.168.0.1
3. Go to the "Config" tab
4. Upload `btt_rodent_fluidnc.yaml`
5. Reboot the ESP32

#### Option B: Via Serial Terminal
1. Connect ESP32 to computer via USB
2. Open serial terminal (115200 baud)
3. Upload the YAML file through the FluidNC file system
4. Type `$Config/Filename=btt_rodent_fluidnc.yaml`
5. Reboot with `$Reboot`

#### Option C: Via SD Card
1. Format SD card as FAT32
2. Copy `btt_rodent_fluidnc.yaml` to SD card root
3. Rename to `config.yaml`
4. Insert SD card into ESP32 SD card slot
5. Reboot ESP32

### 3. Configure RS485 Communication Settings

Ensure the ESP32 firmware and BTT Rodent are configured for:
- **Baud Rate:** 115200
- **Data Bits:** 8
- **Parity:** None
- **Stop Bits:** 1
- **Protocol:** RS485 half-duplex
- **Flow Control:** RTS (GPIO 15 on ESP32 side)

## Configuration Customization

### Adjusting Steps Per MM

For peristaltic pumps, you may need to calibrate steps per mm:

```yaml
steps_per_mm: 80.000  # Adjust this value
```

To calculate:
1. Measure the actual distance moved
2. Calculate: `new_steps = (target_distance / actual_distance) * current_steps`
3. Update the value in the YAML file

### Adjusting Speed and Acceleration

```yaml
max_rate_mm_per_min: 5000.000        # Maximum speed
acceleration_mm_per_sec2: 200.000    # Acceleration rate
```

Start with conservative values and increase gradually for your application.

### PWM Speed Control

If using PWM for pump speed control:

```yaml
pwm:
  pwm_hz: 5000                       # PWM frequency
  output_pin: gpio.2                 # PWM output pin
  speed_map: 0=0.000% 1000=100.000% # Speed mapping
```

## Testing the Configuration

### 1. Connect via Serial Terminal

```bash
# Linux/Mac
screen /dev/ttyUSB0 115200

# Windows - use PuTTY or Tera Term
```

### 2. Basic GCode Commands

```gcode
$I                    # Show system info
$$                    # Show all settings
$X                    # Unlock (if needed)
G91                   # Relative positioning mode
G1 X10 F1000         # Move X axis (Pump 1) 10mm at 1000mm/min
G1 Y10 F1000         # Move Y axis (Pump 2) 10mm
G1 Z10 F1000         # Move Z axis (Pump 3) 10mm
G1 A10 F1000         # Move A axis (Pump 4) 10mm
M3 S500              # Start PWM at 50% (if configured)
M5                   # Stop PWM
```

### 3. Verify UART Communication

Monitor the UART communication between ESP32 and BTT Rodent:
- Check TX/RX LEDs if available
- Use logic analyzer if detailed debugging needed
- Monitor serial output for errors

## Troubleshooting

### No Communication with BTT Rodent

1. **Check RS485 wiring:**
   - Verify A+ and B- connections are not swapped
   - Confirm common ground connection
   - Check for loose connections
   - Ensure RS485 transceiver module is powered (usually 3.3V or 5V)

2. **Verify RS485 transceiver:**
   - Check that RTS pin (GPIO 15) is connected to DE/RE pins on MAX485
   - Verify transceiver power supply
   - Test with a multimeter for proper voltage levels

3. **Verify baud rate:**
   - Both devices must use same baud rate (115200)
   - Check BTT Rodent firmware settings

4. **Test RS485 direction control:**
   - Monitor GPIO 15 with oscilloscope/logic analyzer
   - Should go HIGH during transmit, LOW during receive
   - Timing should match UART transmission windows

### Motors Not Moving

1. **Check power supply:**
   - Verify voltage is correct (12V or 24V)
   - Ensure sufficient current capacity

2. **Verify stepper drivers:**
   - Check if BTT Rodent stepper drivers are enabled
   - Adjust VREF if needed
   - Check for overheating

3. **Test step signals:**
   - Use multimeter or oscilloscope
   - Verify pulses on step pins

### ESP32 Not Responding

1. **Check power:**
   - Verify 5V USB power or external 3.3V
   - Check for brownout issues

2. **Re-flash firmware:**
   - Erase flash: `esptool.py erase_flash`
   - Re-upload FluidNC firmware

3. **Check serial connection:**
   - Verify correct COM port
   - Try different USB cable
   - Check for driver issues

## Advanced Configuration

### Adding Limit Switches

```yaml
motor0:
  limit_neg_pin: gpio.36  # Negative limit switch
  limit_pos_pin: gpio.39  # Positive limit switch
  hard_limits: true
  pulloff_mm: 1.000
```

### Enabling Homing

```yaml
homing:
  cycle: 1                # Enable homing
  positive_direction: false
  mpos_mm: 0.000
  feed_mm_per_min: 800.000
  seek_mm_per_min: 2000.000
```

### Multiple Pump Control

To control 4 pumps independently:
1. Use X, Y, Z, A axes for the 4 different pumps
2. Adjust steps_per_mm for each axis as needed
3. Use GCode commands to control each axis separately or in combination

## GCode Examples for Peristaltic Pumps

### Dispense Specific Volume

```gcode
G91              ; Relative mode
G1 X50 F1000    ; Dispense 50mm worth of fluid at 1000mm/min
```

### Continuous Pumping

```gcode
G91              ; Relative mode
G1 X1000 F500   ; Pump for extended distance at lower speed
```

### Multi-Pump Coordination

```gcode
G91                    ; Relative mode
G1 X10 Y20 F1000      ; Pump 1 and 2 simultaneously
G1 X10 Y10 Z15 F1000  ; Pump 1, 2, and 3 together
G1 X10 Y10 Z10 A5 F1000 ; Run all 4 pumps simultaneously with different amounts
```

### Individual Pump Control

```gcode
; Control each pump separately
G91              ; Relative mode
G1 X50 F1000    ; Only Pump 1 (X axis)
G1 Y30 F800     ; Only Pump 2 (Y axis)
G1 Z40 F1200    ; Only Pump 3 (Z axis)
G1 A20 F900     ; Only Pump 4 (A axis)
```

### Sequential Pump Operation

```gcode
; Run pumps in sequence
G91              ; Relative mode
G1 X10 F1000    ; Start with Pump 1
G4 P2000        ; Wait 2 seconds
G1 Y10 F1000    ; Then Pump 2
G4 P2000        ; Wait 2 seconds
G1 Z10 F1000    ; Then Pump 3
G4 P2000        ; Wait 2 seconds
G1 A10 F1000    ; Finally Pump 4
```

### PWM Speed Control

```gcode
M3 S250          ; Set pump speed to 25%
G4 P5000         ; Wait 5 seconds
M3 S500          ; Set pump speed to 50%
G4 P5000         ; Wait 5 seconds
M5               ; Stop pump
```

## Maintenance

1. **Regular Calibration:**
   - Verify steps per mm periodically
   - Check for pump wear affecting accuracy

2. **Check Connections:**
   - Inspect UART wiring regularly
   - Verify power supply stability

3. **Firmware Updates:**
   - Keep FluidNC updated for bug fixes
   - Backup configuration before updates

## Resources

- **FluidNC Documentation:** https://github.com/bdring/FluidNC/wiki
- **FluidNC Discord:** https://discord.gg/fluidnc
- **BTT Rodent Documentation:** https://github.com/bigtreetech/BIGTREETECH-Rodent
- **GCode Reference:** https://linuxcnc.org/docs/html/gcode.html

## Support

For issues specific to:
- **FluidNC:** Visit FluidNC GitHub issues or Discord
- **BTT Rodent:** Check BigTreeTech GitHub repository
- **This Configuration:** Open an issue in this repository

## License

This configuration is provided as-is for use with FluidNC and BTT Rodent hardware.
