# ESP-IDF Native Setup Guide

Due to PlatformIO integration issues, this project now uses **native ESP-IDF tooling** for more reliable builds.

## 🚀 Quick Start

### 1. Install ESP-IDF

**Linux/macOS:**
```bash
# Clone ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git ~/esp-idf

# Install dependencies
cd ~/esp-idf
./install.sh esp32

# Set up environment (run this in every new terminal)
. ~/esp-idf/export.sh
```

**Windows:**
Follow: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/windows-setup.html

### 2. Build and Flash

```bash
# Go to project directory
cd Test_Programming

# Build test 00 (blink)
./build.sh test_00_blink build

# Flash to ESP32
./build.sh test_00_blink flash

# Monitor serial output
./build.sh test_00_blink monitor

# Or do all at once
./build.sh test_00_blink all
```

## 📋 Available Tests

```bash
./build.sh test_00_blink all    # Blink and serial output
./build.sh test_01_buttons all  # Button testing
./build.sh test_02_encoder all  # Rotary encoder
```

## 🔧 Build Commands

The `build.sh` script supports:

```bash
./build.sh <test_name> <action>
```

**Actions:**
- `build` - Build only
- `flash` - Build and flash to ESP32
- `monitor` - Open serial monitor
- `clean` - Clean build artifacts
- `all` - Build, flash, and monitor (most common)

**Examples:**
```bash
# Build only
./build.sh test_00_blink build

# Flash without rebuilding (if already built)
idf.py flash

# Monitor only
idf.py monitor

# Exit monitor: Ctrl + ]
```

## 🐛 Troubleshooting

### ESP-IDF not found
```
ERROR: ESP-IDF not found!
```
**Solution:** Run `. ~/esp-idf/export.sh` in your terminal

### Permission denied
```
Permission denied: /dev/ttyUSB0
```
**Solution:**
```bash
sudo usermod -a -G dialout $USER
# Then log out and back in
```

### Port not found
```
Error: Could not open port
```
**Solution:**
```bash
# List ports
ls /dev/tty*

# Specify port manually
idf.py -p /dev/ttyUSB0 flash monitor
```

### First build is slow
**This is normal!** First build compiles entire ESP-IDF (~10-15 minutes).
Subsequent builds are fast (30 seconds - 2 minutes).

## 📖 Manual Commands

If you prefer not to use `build.sh`:

```bash
# Copy test file manually
cp test/test_00_blink/test_00_blink.c main/main.c
cp test/common/pin_definitions.h main/

# Build with ESP-IDF
idf.py build

# Flash
idf.py flash

# Monitor
idf.py monitor

# All together
idf.py build flash monitor
```

## 🔄 Switching Tests

Each test requires copying its source to `main/main.c`:

```bash
# Switch to test 01 (buttons)
cp test/test_01_buttons/test_01_buttons.c main/main.c
cp test/common/pin_definitions.h main/
idf.py build flash monitor

# Or use the build script
./build.sh test_01_buttons all
```

## ⚙️ Configuration

### Change Serial Port
```bash
idf.py menuconfig
# Navigate to: Serial flasher config → Default serial port
```

### Change Baud Rate
```bash
idf.py menuconfig
# Navigate to: Component config → ESP System Settings → UART console baud rate
```

### Optimize Build Speed
```bash
# Use ninja instead of make
idf.py build -G Ninja

# Parallel builds (use all CPU cores)
idf.py build -j$(nproc)
```

## 📚 ESP-IDF Resources

- **Official Docs:** https://docs.espressif.com/projects/esp-idf/
- **API Reference:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/
- **Examples:** https://github.com/espressif/esp-idf/tree/master/examples
- **Forum:** https://esp32.com/

## 🔁 Reverting to PlatformIO (Optional)

If you want to try PlatformIO again later:

```bash
# PlatformIO commands still work (if you fix the issues)
pio run -e test_00_blink -t upload -t monitor
```

But ESP-IDF native is recommended for this project.

---

## ✅ Why ESP-IDF Native?

**Pros:**
- ✅ Official Espressif tooling - most reliable
- ✅ Better documentation
- ✅ Fewer integration issues
- ✅ Full control over build system
- ✅ Faster incremental builds
- ✅ Access to latest ESP-IDF features

**Cons:**
- ❌ Manual environment setup (`. ~/esp-idf/export.sh`)
- ❌ Need to switch tests manually (or use build.sh)
- ❌ Different workflow than PlatformIO

---

**Current Status:** ✅ Ready to build with ESP-IDF!

**Next Steps:**
1. Install ESP-IDF (if not already)
2. Run: `. ~/esp-idf/export.sh`
3. Run: `./build.sh test_00_blink all`
4. Watch LED blink! 🎉
