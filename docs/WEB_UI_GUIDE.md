# Web UI Guide - Peristaltic Pump Control System

## Overview

The Web UI provides a modern, mobile-responsive interface for controlling your peristaltic pump system through any web browser. Access recipe management, manual pump control, and real-time system monitoring from your phone, tablet, or computer.

## Features

- 📱 **Mobile-Responsive Design** - Works on any device
- 🔄 **Real-Time Updates** - WebSocket connection for live status
- 📋 **Recipe Management** - Browse and execute recipes with one click
- ⚙️ **Manual Pump Control** - Individual pump control with adjustable flow rates
- ⛔ **Emergency Stop** - Instant stop button always available
- 🎨 **Modern UI** - Clean, professional interface with visual feedback

## Quick Start

### 1. Configure WiFi

Edit `src/test_21_web_ui.cpp` and update your WiFi credentials:

```cpp
const char* WIFI_SSID = "YourWiFiSSID";        // Your WiFi network name
const char* WIFI_PASSWORD = "YourWiFiPassword"; // Your WiFi password
```

### 2. Upload the Program

```bash
pio run -e test_21_web_ui -t upload -t monitor
```

### 3. Find the IP Address

Watch the serial monitor for the IP address:

```
✓ WiFi connected
IP Address: 192.168.1.100

Access the web UI at:
http://192.168.1.100

System ready!
```

### 4. Open the Web UI

Open your browser and navigate to the IP address shown in the serial monitor.

## Using the Web UI

### Main Interface Sections

#### 1. Header / Status Bar

The header displays:
- **Connection Status**: Green (Connected) or Red (Disconnected)
- **System Status**: Current mode (Idle, Executing, Manual)
- **Current Recipe**: Shows active recipe when executing

#### 2. Recipe Panel

Browse and execute pre-defined recipes:

**Available Recipes:**
- **Cleaning Flush** - Runs all 4 pumps sequentially (5ml each @ 30ml/min)
- **Color Mix** - 3-step color mixing recipe
- **Nutrient Mix** - 4-step nutrient mixing with precise dosing
- **Custom Test** - 2-step test recipe

**To Start a Recipe:**
1. Click the "Start" button next to the desired recipe
2. The system will execute each step sequentially
3. Monitor progress in the status bar
4. Recipe completes automatically

#### 3. Manual Pump Control

Control individual pumps manually:

**For Each Pump (X, Y, Z, A):**
1. Adjust the flow rate slider (0-100 ml/min)
2. Click "Start" to begin pumping
3. Use Emergency Stop to halt

**Pump Status Indicators:**
- **Green (Running)** - Pump is actively dispensing
- **Gray (Stopped)** - Pump is idle

#### 4. Emergency Stop

Large red button at the bottom:
- Immediately stops all pumps
- Available at all times
- Resets system to idle state

## API Reference

The Web UI communicates with the ESP32 via REST API and WebSocket.

### REST Endpoints

#### Get Recipe List
```
GET /api/recipes
```
Returns list of available recipes.

**Response:**
```json
{
  "recipes": [
    {
      "name": "Cleaning Flush",
      "steps": 4
    },
    ...
  ]
}
```

#### Start Recipe
```
POST /api/recipe/start/{index}
```
Start recipe by index (0-based).

**Example:**
```javascript
fetch('/api/recipe/start/0', {method: 'POST'})
```

#### Start Individual Pump
```
POST /api/pump/start
Content-Type: application/json

{
  "pump": "X",       // Pump axis: X, Y, Z, or A
  "flowRate": 30.0   // Flow rate in ml/min
}
```

#### Emergency Stop
```
POST /api/stop
```
Stops all pumps immediately.

#### Get System Status
```
GET /api/status
```
Returns current system status.

**Response:**
```json
{
  "mode": "idle",
  "systemState": "Idle",
  "currentRecipe": -1,
  "currentStep": 0
}
```

### WebSocket Updates

Connect to `ws://{IP_ADDRESS}/ws` for real-time status updates.

**Status Message Format:**
```json
{
  "mode": "executing",
  "systemState": "Color Mix - Step 2/3",
  "currentRecipe": 1,
  "currentStep": 1,
  "lastError": "",
  "pumps": [
    {
      "id": 0,
      "running": true,
      "flowRate": 15.0,
      "targetVolume": 10.0,
      "dispensed": 5.2
    },
    ...
  ]
}
```

## Customizing Recipes

Recipes are defined in `src/test_21_web_ui.cpp`.

### Recipe Structure

```cpp
Ingredient myRecipe[] = {
    {'X', 10.0, 20.0},  // Pump X: 10ml @ 20ml/min
    {'Y', 5.0, 15.0},   // Pump Y: 5ml @ 15ml/min
    {'Z', 7.5, 25.0}    // Pump Z: 7.5ml @ 25ml/min
};
```

### Adding a New Recipe

1. Define the ingredient array:
```cpp
Ingredient myNewRecipe[] = {
    {'X', 15.0, 30.0},
    {'A', 5.0, 10.0}
};
```

2. Add to recipes array:
```cpp
Recipe recipes[] = {
    {"Cleaning Flush", cleaningRecipe, 4},
    {"Color Mix", colorMixRecipe, 3},
    {"Nutrient Mix", nutrientMixRecipe, 4},
    {"Custom Test", customTestRecipe, 2},
    {"My New Recipe", myNewRecipe, 2}  // Add this line
};
const int recipeCount = 5;  // Update count
```

3. Recompile and upload:
```bash
pio run -e test_21_web_ui -t upload
```

## Troubleshooting

### WiFi Won't Connect

**Solution 1: Check Credentials**
- Verify SSID and password are correct
- Check for typos and case sensitivity

**Solution 2: Use Access Point Mode**

If WiFi connection fails, the ESP32 automatically creates its own WiFi network:

```
SSID: PumpControl
Password: 12345678
IP Address: 192.168.4.1
```

Connect to this network and access: `http://192.168.4.1`

### Can't Access Web UI

**Check Serial Monitor:**
```bash
pio device monitor
```

Look for:
- "✓ WiFi connected" message
- IP address output
- "✓ Web server started" confirmation

**Common Issues:**
1. **Wrong IP Address** - Use the exact IP shown in serial monitor
2. **Different Network** - Ensure your device is on the same WiFi network
3. **Firewall** - Check if firewall is blocking port 80

### WebSocket Disconnected

The connection status indicator will show red if disconnected.

**Auto-Reconnect:**
- WebSocket automatically reconnects every 5 seconds
- Refresh the page to force immediate reconnection

**If it persists:**
1. Check ESP32 is powered and running
2. Verify network connectivity
3. Check serial monitor for errors

### Pumps Not Responding

**Verify UART Connection:**
- Check BTT Rodent board is powered
- Verify UART wiring (GPIO 16/17)
- Look for `→` and `←` messages in serial monitor

**Check G-code Responses:**
```
→ G1 X100 F100
← ok
```

If no `←` responses, check UART connection.

### Recipe Won't Start

**Check System State:**
- System must be in "Idle" mode to start new recipe
- Use Emergency Stop to reset if needed

**Verify Recipe Index:**
- Recipe numbers are 0-based (first recipe is index 0)
- Check recipe count matches array length

## Safety Notes

⚠️ **Important Safety Guidelines:**

1. **Emergency Stop**
   - Always accessible at bottom of page
   - Stops all pumps immediately
   - Use if anything goes wrong

2. **Flow Rate Limits**
   - Maximum feedrate is limited to 300 mm/min for safety
   - Adjust `SAFE_TEST_FEEDRATE` in code if needed

3. **Monitor Serial Output**
   - Keep serial monitor open during operation
   - Watch for error messages or unusual responses
   - Provides detailed logging of all commands

4. **Test Before Production**
   - Test all recipes with water first
   - Verify volumes are correct
   - Check for leaks or mechanical issues

5. **Network Security**
   - Web UI has no authentication by default
   - Only use on trusted networks
   - Consider adding authentication for production use

## Advanced Configuration

### Changing WiFi Mode

**Station Mode (Default):**
```cpp
WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
```

**Access Point Mode Only:**
```cpp
// Comment out WiFi.begin() and use:
WiFi.softAP("PumpControl", "your_password_here");
```

### Pump Calibration

Adjust the calibration constant for accurate dispensing:

```cpp
const float ML_PER_MM = 0.05;  // Adjust based on your pump calibration
```

**To Calibrate:**
1. Command pump to dispense 100mm: `G1 X100 F100`
2. Measure actual volume dispensed
3. Calculate: `ML_PER_MM = volume_dispensed / 100`
4. Update constant and recompile

### WebSocket Update Interval

Change status broadcast frequency:

```cpp
// In loop(), change interval (default 1000ms)
if (millis() - lastUpdate > 1000) {  // Change to 500 for faster updates
    lastUpdate = millis();
    broadcastStatus();
}
```

### Adding Authentication

For production use, consider adding basic authentication:

```cpp
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if(!request->authenticate("admin", "your_password")) {
        return request->requestAuthentication();
    }
    request->send_P(200, "text/html", index_html);
});
```

## Mobile App Development

The REST API and WebSocket interface can be used to build native mobile apps:

**Technologies:**
- React Native + WebSocket
- Flutter + web_socket_channel
- Native iOS/Android with WebSocket libraries

**Example WebSocket Connection (JavaScript):**
```javascript
const ws = new WebSocket('ws://192.168.1.100/ws');

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log('Status:', data);
};
```

## Integration with Home Automation

### Home Assistant

Add to `configuration.yaml`:

```yaml
rest_command:
  pump_recipe_start:
    url: http://192.168.1.100/api/recipe/start/{{ recipe_id }}
    method: POST

  pump_emergency_stop:
    url: http://192.168.1.100/api/stop
    method: POST
```

### MQTT Bridge (Future Enhancement)

Consider adding MQTT support for integration with:
- Node-RED
- Home Assistant
- OpenHAB
- Custom automation systems

## Performance Notes

### Memory Usage

The web UI uses:
- ~80KB Flash (program storage)
- ~15KB RAM (during operation)
- WebSocket: ~1KB per connected client

**Recommendations:**
- Limit to 5 simultaneous WebSocket connections
- Monitor free heap: `Serial.println(ESP.getFreeHeap());`

### Network Performance

- WebSocket updates: 1 per second
- Typical latency: 10-50ms on local network
- REST API response: 5-20ms

## Development & Debugging

### Enable Debug Output

Add to `setup()`:
```cpp
Serial.setDebugOutput(true);
```

### Monitor WebSocket Traffic

Use browser developer tools:
1. Open DevTools (F12)
2. Go to Network tab
3. Filter by "WS" (WebSocket)
4. Watch messages in real-time

### Check Free Memory

Add to `loop()`:
```cpp
static unsigned long lastMemCheck = 0;
if (millis() - lastMemCheck > 10000) {
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    lastMemCheck = millis();
}
```

## Support

For issues or questions:

1. **Check Serial Monitor** - Most issues show error messages
2. **Review Documentation** - See `/docs/` folder
3. **Test UART Connection** - Run test_08_uart to verify communication
4. **Test WiFi** - Run a simple ESP32 WiFi example to verify network

## Version History

- **v1.0** - Initial web UI release
  - Recipe management
  - Manual pump control
  - WebSocket status updates
  - Emergency stop
  - Mobile-responsive design

## Future Enhancements

Planned features:
- [ ] User authentication
- [ ] Recipe editor in web UI
- [ ] Data logging and export
- [ ] Scale integration display
- [ ] LED status visualization
- [ ] MQTT support
- [ ] Recipe scheduling
- [ ] Multi-language support

---

**Last Updated:** 2025-11-17
**Compatible With:** test_21_web_ui
**Framework:** Arduino (ESP32)
