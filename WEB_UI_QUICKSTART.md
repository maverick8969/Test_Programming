# Web UI Quick Start Guide

## 🚀 Get Started in 3 Steps

### Step 1: Configure WiFi

Edit `src/test_21_web_ui.cpp` (lines 36-37):

```cpp
const char* WIFI_SSID = "YourWiFiSSID";        // ← Change this
const char* WIFI_PASSWORD = "YourWiFiPassword"; // ← Change this
```

### Step 2: Upload to ESP32

```bash
pio run -e test_21_web_ui -t upload -t monitor
```

### Step 3: Open Web UI

Wait for the IP address in the serial monitor:

```
✓ WiFi connected
IP Address: 192.168.1.100        ← Your IP will be here
```

Then open in your browser:
```
http://192.168.1.100
```

## 📱 Features

### Recipe Control
- Browse 4 pre-defined recipes
- Click "Start" to execute any recipe
- Automatic step-by-step execution
- Real-time progress updates

### Manual Pump Control
- Control each pump individually (X, Y, Z, A)
- Adjust flow rate: 0-100 ml/min
- Instant start/stop control
- Live status indicators

### Emergency Stop
- Large red button always visible
- Instantly stops all pumps
- One-click safety feature

## 🔧 WiFi Troubleshooting

### Can't Connect to WiFi?

The system automatically creates a WiFi hotspot if connection fails:

```
SSID: PumpControl
Password: 12345678
IP: http://192.168.4.1
```

1. Connect your phone/computer to "PumpControl" WiFi
2. Open browser to `http://192.168.4.1`
3. Done!

## 📋 Pre-Installed Recipes

1. **Cleaning Flush** - All 4 pumps, 5ml each @ 30ml/min
2. **Color Mix** - 3-step color mixing
3. **Nutrient Mix** - 4-step precision dosing
4. **Custom Test** - 2-step test recipe

## ⚙️ API Examples

### Start Recipe via Command Line

```bash
# Start recipe 0 (Cleaning Flush)
curl -X POST http://192.168.1.100/api/recipe/start/0
```

### Manual Pump Control

```bash
# Start pump X at 30 ml/min
curl -X POST http://192.168.1.100/api/pump/start \
  -H "Content-Type: application/json" \
  -d '{"pump":"X","flowRate":30}'
```

### Emergency Stop

```bash
curl -X POST http://192.168.1.100/api/stop
```

### Get Status

```bash
curl http://192.168.1.100/api/status
```

## 🎯 Customizing Recipes

Add your own recipes in `src/test_21_web_ui.cpp`:

```cpp
// 1. Define your recipe
Ingredient myRecipe[] = {
    {'X', 20.0, 40.0},  // Pump X: 20ml @ 40ml/min
    {'Y', 10.0, 25.0}   // Pump Y: 10ml @ 25ml/min
};

// 2. Add to the recipes array (around line 86)
Recipe recipes[] = {
    {"Cleaning Flush", cleaningRecipe, 4},
    {"Color Mix", colorMixRecipe, 3},
    {"Nutrient Mix", nutrientMixRecipe, 4},
    {"Custom Test", customTestRecipe, 2},
    {"My Recipe", myRecipe, 2}  // ← Add this
};

// 3. Update the count
const int recipeCount = 5;  // ← Change from 4 to 5
```

Then recompile and upload:
```bash
pio run -e test_21_web_ui -t upload
```

## 📱 Mobile Access

The interface is fully mobile-responsive:
- Works on iPhone, iPad, Android phones/tablets
- Touch-friendly controls
- Responsive layout adapts to screen size

Simply open the IP address in your mobile browser!

## 🔒 Safety Features

- ✅ Maximum flow rate limited to 300 mm/min
- ✅ Emergency stop always accessible
- ✅ Real-time status monitoring
- ✅ Automatic system state management
- ✅ Error reporting and recovery

## 📖 Full Documentation

For complete documentation, see:
- [Web UI Guide](docs/WEB_UI_GUIDE.md) - Complete usage guide
- [G-code Reference](docs/reference/GCODE_COMMAND_REFERENCE.md) - Command reference
- [Hardware Overview](docs/hardware/HARDWARE_OVERVIEW.md) - System architecture

## 🆘 Common Issues

### No Serial Output?
```bash
# Check USB port
ls /dev/ttyUSB*

# Monitor with correct port
pio device monitor --port /dev/ttyUSB0
```

### Can't Upload?
Hold the BOOT button on ESP32 while uploading.

### Web Page Won't Load?
1. Ping the IP address: `ping 192.168.1.100`
2. Check you're on the same network
3. Try AP mode (PumpControl WiFi hotspot)

### Pumps Not Moving?
1. Check UART connection (GPIO 16/17)
2. Verify BTT Rodent board is powered
3. Look for `→` and `←` in serial monitor

## 💡 Tips

- **Bookmark the IP** - Save the web UI URL to your phone home screen
- **Keep Serial Monitor Open** - Detailed logs help troubleshooting
- **Test with Water First** - Always test recipes before using chemicals
- **Monitor Free Heap** - Check memory usage if adding features

## 🎨 UI Preview

```
┌─────────────────────────────────────────┐
│ 🧪 Peristaltic Pump Control System     │
│ [Connected] [Idle]                      │
├─────────────────────────────────────────┤
│ 📋 Recipes                              │
│ ┌─────────────────────────────────────┐ │
│ │ Cleaning Flush        [Start]       │ │
│ │ Color Mix            [Start]        │ │
│ │ Nutrient Mix         [Start]        │ │
│ │ Custom Test          [Start]        │ │
│ └─────────────────────────────────────┘ │
│                                         │
│ ⚙️ Manual Pump Control                  │
│ ┌─────────────────────────────────────┐ │
│ │ Pump X [●●●●●○○○○○] 30ml/min [Start]│ │
│ │ Pump Y [●●●●●○○○○○] 30ml/min [Start]│ │
│ │ Pump Z [●●●●●○○○○○] 30ml/min [Start]│ │
│ │ Pump A [●●●●●○○○○○] 30ml/min [Start]│ │
│ └─────────────────────────────────────┘ │
│                                         │
│ ┌─────────────────────────────────────┐ │
│ │    ⛔ EMERGENCY STOP                │ │
│ └─────────────────────────────────────┘ │
└─────────────────────────────────────────┘
```

---

**Ready to start?** Upload the code and open your browser!

```bash
pio run -e test_21_web_ui -t upload -t monitor
```

For detailed documentation, see [WEB_UI_GUIDE.md](docs/WEB_UI_GUIDE.md)
