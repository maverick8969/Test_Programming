# Peristaltic Pump Control System

A comprehensive 4-pump peristaltic pumping system controlled by ESP32, featuring weight-based dosing, recipe management, and professional control interface.

## 🚀 Quick Start

### For Testing (Start Here!)

```bash
# Install PlatformIO
pip install platformio

# Navigate to project
cd Test_Programming

# Run first test (blink)
pio run -e test_00_blink -t upload -t monitor
```

**See [`test/README.md`](test/README.md) for complete test instructions!**

---

## 📋 Project Overview

### System Architecture

```
ESP32 Main Controller
├── RS485 → BTT Rodent V1.1 (4× Stepper Motor Drivers)
│   ├── Motor 1 (X-axis) → Pump 1 - DMDEE
│   ├── Motor 2 (Y-axis) → Pump 2 - T-12
│   ├── Motor 3 (Z-axis) → Pump 3 - T-9
│   └── Motor 4 (A-axis) → Pump 4 - L25B
├── RS232 → Digital Scale (weight monitoring)
├── I2C → 1602 LCD Display (user interface)
├── RMT → WS2812B LEDs (32 LEDs, 4 strips)
└── GPIO → Buttons (START, STOP, MODE) + Encoder (SELECT)
```

### Key Features

- ✅ **4 Independent Pumps** - Simultaneous or sequential operation
- ✅ **Weight-Based Dosing** - Accurate dispensing using digital scale
- ✅ **Recipe Management** - Store and execute multi-pump recipes
- ✅ **Visual Feedback** - 32 RGB LEDs (8 per pump) + LCD display
- ✅ **Professional Control** - Rotary encoder + buttons
- ✅ **G-code Control** - Industry-standard CNC commands
- ✅ **Incremental Development** - Well-tested modular approach

---

## 📚 Documentation

| Document | Description |
|----------|-------------|
| **[DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md)** | **START HERE** - Complete 8-phase development roadmap |
| [test/README.md](test/README.md) | Test programs guide (Phase 1-3) |
| [docs/hardware/HARDWARE_OVERVIEW.md](docs/hardware/HARDWARE_OVERVIEW.md) | Complete hardware specifications |
| [docs/hardware/WIRING_GUIDE.md](docs/hardware/WIRING_GUIDE.md) | Step-by-step wiring instructions |
| [docs/setup/FLUIDNC_SETUP_GUIDE.md](docs/setup/FLUIDNC_SETUP_GUIDE.md) | BTT Rodent configuration |
| [docs/reference/GCODE_COMMAND_REFERENCE.md](docs/reference/GCODE_COMMAND_REFERENCE.md) | G-code commands for pump control |

---

## 🛠️ Hardware Requirements

### Core Components

| Component | Model | Quantity | Purpose |
|-----------|-------|----------|---------|
| ESP32 Dev Board | ESP32-WROOM-32 | 1 | Main controller |
| BTT Rodent V1.1 | FluidNC-compatible | 1 | Motor controller |
| Stepper Motors | NEMA 17 | 4 | Pump actuation |
| Digital Scale | RS232 output | 1 | Weight monitoring |
| LED Strips | WS2812B (5V) | 4 strips | Visual feedback (32 LEDs) |
| LCD Display | 1602 I2C | 1 | User interface |
| Rotary Encoder | KY-040 or similar | 1 | Menu navigation + SELECT |
| Push Buttons | Momentary NO | 3 | START, STOP, MODE |
| **RS485 Module** | **MAX485 or auto-direction** | **1** | **ESP32 ↔ Rodent (REQUIRED)** |
| **RS232 Module** | **MAX3232** | **1** | **ESP32 ↔ Scale (REQUIRED)** |

### ⚠️ Critical: Level Converters Required

**RS485 Transceiver (MAX485 or auto-direction module):**
- Converts ESP32 TTL (3.3V) ↔ RS485 differential (A+/B-)
- **Required for BTT Rodent communication**
- Auto-direction module recommended (simpler, no RTS control)

**RS232 Transceiver (MAX3232):**
- Converts ESP32 TTL (3.3V) ↔ RS232 (±12V)
- **Required for digital scale communication**
- **NEVER connect RS232 directly to ESP32 - will destroy GPIO!**

---

## 🏗️ Development Phases

### Phase 1: Basic Hardware Testing ✅ READY TO USE
- Test 00: Blink and serial output
- Test 01: Push buttons with debouncing
- Test 02: Rotary encoder + SELECT button

**Status:** ✅ Code complete - ready to test!

### Phase 2: Communication Peripherals 🚧 Coming Soon
- Test 03: I2C scanner
- Test 04: LCD display
- Test 05: WS2812B LED strips
- Test 06: Digital scale (RS232)

### Phase 3: RS485 Motor Control 🚧 Coming Soon
- Test 07: RS485 communication
- Test 08: G-code commands
- Test 09: Single motor test
- Test 10: All motors test

### Phase 4-7: Integration 🚧 Future
- Pump control logic
- User interface integration
- Recipe management
- Main application

### Phase 8: Advanced Features 🔮 Optional
- WiFi/MQTT integration
- Data logging
- Web interface

**See [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md) for complete timeline and details.**

---

## 🎯 Getting Started

### Step 1: Install PlatformIO

**Option A: VSCode Extension (Recommended)**
1. Install VSCode
2. Install PlatformIO IDE extension
3. Open this project folder

**Option B: Command Line**
```bash
pip install platformio
```

### Step 2: Hardware Setup

**For Phase 1 Testing (Minimum Setup):**
1. ESP32 dev board
2. USB cable
3. 3× Push buttons (optional for Test 00)
4. 1× Rotary encoder (optional for Test 00)
5. Breadboard and jumper wires

**Wiring for Phase 1:**
```
START Button:    GPIO 13 ──[Button]── GND
MODE Button:     GPIO 14 ──[Button]── GND
STOP Button:     GPIO 33 ──[Button]── GND
Encoder CLK:     GPIO 26
Encoder DT:      GPIO 27
Encoder SW:      GPIO 12 (SELECT)
```

### Step 3: Run First Test

```bash
# Test 00: Blink (no hardware needed except ESP32)
pio run -e test_00_blink -t upload -t monitor

# Test 01: Buttons (requires 3 buttons wired)
pio run -e test_01_buttons -t upload -t monitor

# Test 02: Encoder (requires rotary encoder)
pio run -e test_02_encoder -t upload -t monitor
```

**Expected result:** See [`test/README.md`](test/README.md) for detailed success criteria.

---

## 📐 Pin Mapping

Complete pin mapping is defined in [`test/common/pin_definitions.h`](test/common/pin_definitions.h)

| Function | GPIO | Notes |
|----------|------|-------|
| **Control Inputs** | | |
| START Button | 13 | Active LOW, internal pull-up |
| MODE Button | 14 | Active LOW, internal pull-up |
| STOP Button | 33 | Active LOW, internal pull-up |
| Encoder CLK | 26 | Internal pull-up |
| Encoder DT | 27 | Internal pull-up |
| Encoder SW (SELECT) | 12 | Active LOW, internal pull-up |
| **Communication** | | |
| RS485 TX (Rodent) | 2 | UART1 → MAX485 DI |
| RS485 RX (Rodent) | 4 | UART1 ← MAX485 RO |
| RS485 RTS (Rodent) | 15 | Direction control (if manual) |
| RS232 RX (Scale) | 16 | UART2 ← MAX3232 R1OUT |
| RS232 TX (Scale) | 17 | UART2 → MAX3232 T1IN |
| **Display & LEDs** | | |
| LCD SDA | 21 | I2C data |
| LCD SCL | 22 | I2C clock |
| LED Data | 25 | WS2812B data (RMT) |

---

## 🔧 PlatformIO Commands

### Build and Upload
```bash
# Build only
pio run -e test_00_blink

# Build and upload
pio run -e test_00_blink -t upload

# Build, upload, and monitor
pio run -e test_00_blink -t upload -t monitor

# Clean build
pio run -t clean
```

### Monitoring
```bash
# Serial monitor (115200 baud)
pio device monitor

# Exit monitor: Ctrl + ]
```

### List Environments
```bash
# Show all test environments
pio run --list-targets
```

---

## 📊 Project Structure

```
Test_Programming/
├── platformio.ini              # PlatformIO configuration
├── test/                       # Test programs (ESP-IDF)
│   ├── common/                # Shared headers
│   │   └── pin_definitions.h  # Complete pin mapping
│   ├── test_00_blink/         # Phase 1 tests
│   ├── test_01_buttons/
│   ├── test_02_encoder/
│   └── README.md              # Test documentation
├── docs/                      # Documentation
│   ├── hardware/              # Hardware guides
│   │   ├── HARDWARE_OVERVIEW.md
│   │   └── WIRING_GUIDE.md
│   ├── setup/                # Setup guides
│   │   ├── FLUIDNC_SETUP_GUIDE.md
│   │   └── SCALE_SETUP_GUIDE.md
│   ├── reference/            # Reference docs
│   │   └── GCODE_COMMAND_REFERENCE.md
│   └── integration/          # Integration guides
├── DEVELOPMENT_PLAN.md       # Complete development roadmap
└── README.md                 # This file
```

---

## ⚙️ Configuration

### Framework: ESP-IDF
This project uses **ESP-IDF** (not Arduino) for:
- ✅ Better performance and control
- ✅ Full FreeRTOS features
- ✅ Professional-grade code
- ✅ Better for production systems

### Board Configuration
```ini
[env]
platform = espressif32
board = esp32dev
framework = espidf
monitor_speed = 115200
```

### Switching Between Tests
Edit `platformio.ini` to change default environment:
```ini
[platformio]
default_envs = test_00_blink  # Change this to desired test
```

Or specify environment when building:
```bash
pio run -e test_01_buttons -t upload
```

---

## 🐛 Troubleshooting

### Common Issues

**Upload fails:**
```bash
# Hold BOOT button while uploading
# Or try:
pio run -e test_XX --upload-port /dev/ttyUSB0 -t upload
```

**Serial monitor shows garbage:**
```bash
# Verify baud rate
pio device monitor --baud 115200
```

**GPIO not working:**
- Check if GPIO is input-only (34, 35, 36, 39)
- Verify no conflicts with strapping pins
- Check power and ground connections

**"File not found" errors:**
- Run clean build: `pio run -t clean`
- Delete `.pio` folder: `rm -rf .pio`
- Rebuild: `pio run -e test_XX`

For detailed troubleshooting, see [`test/README.md`](test/README.md)

---

## 🔒 Safety

### ⚠️ Before Powering On

1. **Double-check voltage levels:**
   - ESP32 GPIO: 3.3V max (NOT 5V tolerant!)
   - RS232: ±12V (requires MAX3232 converter)
   - RS485: Differential (requires MAX485 converter)

2. **Verify level converters installed:**
   - [ ] MAX3232 for scale (RS232 → 3.3V TTL)
   - [ ] MAX485 for Rodent (RS485 → 3.3V TTL)

3. **Check connections:**
   - [ ] All devices share common ground
   - [ ] No shorts between power and ground
   - [ ] Proper polarity on all components

4. **Never connect directly:**
   - ❌ RS232 (±12V) → ESP32 GPIO (will destroy!)
   - ❌ RS485 (A+/B-) → ESP32 GPIO (won't work!)
   - ✅ Always use appropriate level converter!

---

## 📈 Development Timeline

| Phase | Time Estimate | Status |
|-------|---------------|--------|
| Phase 1: Basic I/O | 1-2 hours | ✅ Code Ready |
| Phase 2: Peripherals | 2-3 hours | 🚧 Coming Soon |
| Phase 3: Motor Control | 3-4 hours | 🚧 Coming Soon |
| Phase 4: Pump Logic | 3-4 hours | 🚧 Future |
| Phase 5: UI Integration | 3-4 hours | 🚧 Future |
| Phase 6: Recipes | 3-4 hours | 🚧 Future |
| Phase 7: Main App | 4-6 hours | 🚧 Future |
| Phase 8: Advanced | 9-13 hours | 🔮 Optional |
| **MVP Total** | **19-27 hours** | |

---

## 📝 License

[Add your license here]

---

## 🤝 Contributing

Contributions welcome! Please:
1. Follow existing code style
2. Test thoroughly before submitting
3. Document all changes
4. Update relevant README files

---

## 📞 Support

- **Documentation:** See `docs/` folder
- **Hardware Issues:** Check `docs/hardware/WIRING_GUIDE.md`
- **ESP-IDF Help:** https://docs.espressif.com/projects/esp-idf/

---

## ✨ Credits

- **Framework:** ESP-IDF by Espressif
- **Motor Controller:** FluidNC by Bart Dring
- **Hardware:** BTT Rodent V1.1 by BigTreeTech

---

**Version:** 1.0
**Last Updated:** 2025-11-11
**Status:** Phase 1 Ready for Testing
**Framework:** ESP-IDF (PlatformIO)
