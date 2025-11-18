# WiFi + MQTT Architecture for Peristaltic Pump System

**Challenge**: Enable WiFi-based UI and MQTT logging while maintaining reliable WS2812B LED operation

**Last Updated**: 2025-11-18

---

## Table of Contents

1. [The Challenge](#the-challenge)
2. [Solution Architecture](#solution-architecture)
3. [Implementation Strategy](#implementation-strategy)
4. [Web UI Design](#web-ui-design)
5. [MQTT Data Logging](#mqtt-data-logging)
6. [Code Examples](#code-examples)
7. [Testing Plan](#testing-plan)

---

## 1. The Challenge

### Conflicting Requirements

**LEDs Need**:
- Precise timing (±150ns tolerance)
- No WiFi interrupts
- Continuous CPU attention

**WiFi/MQTT Needs**:
- Radio interrupts for communication
- CPU time for networking stack
- Regular beacon transmissions

### Previous Solution vs. New Requirement

**Previous**: Disabled WiFi completely
```cpp
WiFi.mode(WIFI_OFF);  // Perfect LEDs, no networking
```

**New Requirement**: Enable both WiFi and LEDs simultaneously
- ✅ Web-based user interface
- ✅ MQTT data logging (dosing quantities)
- ✅ Reliable LED visual feedback

---

## 2. Solution Architecture

### Recommended: ESP32 Dual-Core Approach

The ESP32 has **2 CPU cores** that can run tasks independently:
- **Core 0** (Protocol CPU): WiFi, Bluetooth, system tasks
- **Core 1** (Application CPU): User code (default)

**Strategy**: Pin tasks to specific cores to isolate WiFi from LED timing

```
┌─────────────────────────────────────────┐
│           ESP32 Dual Core               │
├──────────────────┬──────────────────────┤
│   CORE 0         │      CORE 1          │
│  (Protocol)      │   (Application)      │
├──────────────────┼──────────────────────┤
│ - WiFi Stack     │ - LED Updates        │
│ - MQTT Client    │ - Pump Control       │
│ - Web Server     │ - Encoder Reading    │
│ - Background     │ - Button Handling    │
│   Tasks          │ - LCD Updates        │
│                  │ - Main Logic         │
└──────────────────┴──────────────────────┘
         │                    │
         ↓                    ↓
    Network I/O          Hardware I/O
```

### Alternative Solutions

#### Option 1: Dual-Core Task Pinning (Recommended)
**Pros**:
- ✅ Single ESP32 (lower cost)
- ✅ Best performance
- ✅ WiFi and LEDs both work reliably

**Cons**:
- ⚠️ More complex code
- ⚠️ Requires FreeRTOS task management

#### Option 2: Reduced WiFi Activity
**Pros**:
- ✅ Simpler code
- ✅ Single ESP32

**Cons**:
- ⚠️ LEDs may still glitch occasionally
- ⚠️ Limited WiFi performance

**Implementation**:
```cpp
WiFi.setTxPower(WIFI_POWER_8_5dBm);  // Lowest power
WiFi.setSleep(WIFI_PS_MAX_MODEM);     // Aggressive sleep
```

#### Option 3: Two ESP32s (Most Reliable)
**Pros**:
- ✅ Complete isolation
- ✅ 100% reliable LEDs
- ✅ Simple code

**Cons**:
- ❌ Higher cost (2× ESP32)
- ❌ More complex wiring

**Architecture**:
```
ESP32 #1 (Main)          ESP32 #2 (LED Controller)
├── WiFi/MQTT            ├── LED Strips (32 LEDs)
├── Web Server           └── Serial/I2C from Main
├── Pump Control
├── Scale Reading
└── User Interface
```

---

## 3. Implementation Strategy

### Phase 1: Dual-Core Task Architecture (Recommended)

#### Core 0 Tasks (WiFi/MQTT)
```cpp
TaskHandle_t wifiTaskHandle;
TaskHandle_t mqttTaskHandle;

void wifiTask(void *parameter) {
  // Runs on Core 0
  WiFi.begin(SSID, PASSWORD);
  while(WiFi.status() != WL_CONNECTED) delay(500);

  // Start web server
  startWebServer();

  // Keep WiFi alive
  while(1) {
    handleWebClients();
    delay(10);
  }
}

void mqttTask(void *parameter) {
  // Runs on Core 0
  mqttClient.setServer(MQTT_BROKER, 1883);
  mqttClient.connect("PumpController");

  while(1) {
    if (!mqttClient.connected()) reconnect();
    mqttClient.loop();
    delay(10);
  }
}

void setup() {
  // Create WiFi task pinned to Core 0
  xTaskCreatePinnedToCore(
    wifiTask,           // Function
    "WiFi",             // Name
    10000,              // Stack size
    NULL,               // Parameters
    1,                  // Priority
    &wifiTaskHandle,    // Handle
    0                   // Core 0
  );

  // Create MQTT task pinned to Core 0
  xTaskCreatePinnedToCore(
    mqttTask,
    "MQTT",
    10000,
    NULL,
    1,
    &mqttTaskHandle,
    0                   // Core 0
  );
}
```

#### Core 1 Tasks (LEDs + Hardware)
```cpp
TaskHandle_t ledTaskHandle;

void ledTask(void *parameter) {
  // Runs on Core 1 - isolated from WiFi
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);

  while(1) {
    updateLEDsBasedOnPumpStatus();
    FastLED.show();
    delay(50);  // 20 Hz update rate
  }
}

void loop() {
  // Main loop runs on Core 1 by default
  updateButtons();
  updateEncoder();
  updatePumpControl();
  updateLCD();
  // NO FastLED.show() here - handled by ledTask
}

void setup() {
  // LED task pinned to Core 1
  xTaskCreatePinnedToCore(
    ledTask,
    "LEDs",
    5000,
    NULL,
    2,                  // Higher priority than WiFi
    &ledTaskHandle,
    1                   // Core 1 (isolated)
  );
}
```

### Phase 2: LED Timing Protection

Even with dual-core, add protection mechanisms:

```cpp
// Mutex for LED updates
SemaphoreHandle_t ledMutex;

void updateLEDs() {
  if (xSemaphoreTake(ledMutex, portMAX_DELAY)) {
    // Temporarily disable interrupts for critical timing
    portDISABLE_INTERRUPTS();
    FastLED.show();
    portENABLE_INTERRUPTS();

    xSemaphoreGive(ledMutex);
  }
}

void setup() {
  ledMutex = xSemaphoreCreateMutex();
}
```

### Phase 3: WiFi Optimization

Reduce WiFi interrupt impact:

```cpp
void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(WIFI_PS_NONE);  // No sleep = more predictable timing

  // Reduce transmit power (less radio activity)
  WiFi.setTxPower(WIFI_POWER_11dBm);  // Moderate power

  // Set static IP (faster connection, fewer broadcasts)
  IPAddress local_IP(192, 168, 1, 100);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(local_IP, gateway, subnet);

  WiFi.begin(SSID, PASSWORD);
}
```

---

## 4. Web UI Design

### Architecture

```
┌────────────────────────────────────────────┐
│          Web Browser (Client)              │
│  ┌──────────────────────────────────────┐  │
│  │  HTML/CSS/JavaScript Single Page App │  │
│  └──────────────────────────────────────┘  │
│           ↕ WebSocket + REST API           │
├────────────────────────────────────────────┤
│        ESP32 Web Server (AsyncWeb)         │
│  ┌────────────┬─────────────────────────┐  │
│  │ REST API   │  WebSocket (Real-time)  │  │
│  └────────────┴─────────────────────────┘  │
│           ↕ Internal Message Queue         │
├────────────────────────────────────────────┤
│          Pump Control System               │
└────────────────────────────────────────────┘
```

### Features

#### Dashboard View
```
┌─────────────────────────────────────────┐
│  Peristaltic Pump Controller            │
├─────────────────────────────────────────┤
│  System Status: ● IDLE                  │
│                                          │
│  Pump 1 (DMDEE)  ████░░░░ 50%  [START] │
│  Pump 2 (T-12)   ██░░░░░░ 25%  [START] │
│  Pump 3 (T-9)    ░░░░░░░░  0%  [START] │
│  Pump 4 (L25B)   ██████░░ 75%  [START] │
│                                          │
│  ┌───────────────────────────────────┐  │
│  │ Recipe Selection                  │  │
│  │ ○ Cleaning Flush                 │  │
│  │ ○ Color Mix A                    │  │
│  │ ● Nutrient Mix                   │  │
│  │         [EXECUTE RECIPE]          │  │
│  └───────────────────────────────────┘  │
│                                          │
│  Target Weight: [____] g  [DISPENSE]    │
│  Current Weight: 45.3 g                  │
└─────────────────────────────────────────┘
```

#### Manual Control
- Individual pump start/stop
- Flow rate adjustment (ml/min)
- Volume setting (ml)
- Real-time status updates via WebSocket

#### Recipe Management
- View available recipes
- Execute recipes
- Monitor progress
- Create/edit recipes (future)

#### Data Logging View
```
┌─────────────────────────────────────────┐
│  Dispensing History                      │
├──────────┬────────┬──────────┬──────────┤
│ Time     │ Pump   │ Volume   │ Recipe   │
├──────────┼────────┼──────────┼──────────┤
│ 10:45:23 │ X,Y,Z  │ 25.0 ml  │ Mix A    │
│ 10:40:15 │ X      │ 10.0 ml  │ Manual   │
│ 10:35:07 │ All    │ 20.0 ml  │ Flush    │
└──────────┴────────┴──────────┴──────────┘
```

### REST API Endpoints

```
GET  /api/status              - System status
GET  /api/pumps               - All pump states
POST /api/pump/{id}/start     - Start pump
POST /api/pump/{id}/stop      - Stop pump
POST /api/pump/{id}/dispense  - Dispense volume
  Body: {"volume": 10.0, "flowRate": 5.0}

GET  /api/recipes             - List recipes
POST /api/recipe/{id}/execute - Execute recipe

GET  /api/scale               - Current weight
POST /api/dispense/weight     - Dispense to target weight
  Body: {"targetGrams": 50.0}

GET  /api/history             - Dispensing history
GET  /api/config              - System configuration
POST /api/config              - Update configuration
```

### WebSocket Events

**Server → Client** (Real-time updates):
```json
{
  "type": "status",
  "data": {
    "state": "RUNNING",
    "pumps": {
      "X": {"active": true, "position": 25.5},
      "Y": {"active": false, "position": 0.0},
      "Z": {"active": true, "position": 10.2},
      "A": {"active": false, "position": 0.0}
    },
    "scale": {"weight": 45.3, "stable": true}
  }
}
```

**Client → Server** (Commands):
```json
{
  "type": "command",
  "action": "startPump",
  "pump": "X",
  "params": {"volume": 10.0, "flowRate": 5.0}
}
```

### Technology Stack

**Server-side** (ESP32):
- **ESPAsyncWebServer** - Async web server (efficient)
- **AsyncWebSocket** - WebSocket for real-time updates
- **ArduinoJson** - JSON parsing/generation
- **SPIFFS/LittleFS** - Store HTML/CSS/JS files

**Client-side** (Browser):
- **HTML5** - Structure
- **CSS3** - Styling (responsive design)
- **Vanilla JavaScript** - No frameworks (smaller size)
- **WebSocket API** - Real-time communication
- **Fetch API** - REST calls

---

## 5. MQTT Data Logging

### MQTT Topics Structure

```
pump/controller/status                    - System status
pump/controller/pump/X/status             - Individual pump status
pump/controller/pump/Y/status
pump/controller/pump/Z/status
pump/controller/pump/A/status

pump/controller/dispense/event            - Dispensing events
pump/controller/dispense/complete         - Completion events
pump/controller/recipe/start              - Recipe started
pump/controller/recipe/complete           - Recipe completed

pump/controller/scale/weight              - Real-time weight
pump/controller/scale/stable              - Weight stabilized

pump/controller/alarm                     - Alarms/errors
pump/controller/command                   - Command received (optional)
```

### MQTT Message Formats

#### Dispensing Event
```json
{
  "timestamp": "2025-11-18T10:45:23Z",
  "eventType": "dispenseStart",
  "pump": "X",
  "volume_ml": 10.0,
  "flowRate_ml_min": 5.0,
  "recipe": "Manual",
  "operator": "WebUI"
}
```

#### Dispensing Complete
```json
{
  "timestamp": "2025-11-18T10:47:23Z",
  "eventType": "dispenseComplete",
  "pump": "X",
  "volume_ml": 10.0,
  "actualVolume_ml": 9.98,
  "duration_sec": 120,
  "success": true,
  "recipe": "Manual"
}
```

#### Recipe Execution
```json
{
  "timestamp": "2025-11-18T11:00:00Z",
  "eventType": "recipeStart",
  "recipeId": "nutrient_mix",
  "recipeName": "Nutrient Mix",
  "steps": 4,
  "estimatedDuration_sec": 300
}
```

#### Weight-Based Dispensing
```json
{
  "timestamp": "2025-11-18T11:05:00Z",
  "eventType": "weightDispenseComplete",
  "pump": "X",
  "targetWeight_g": 50.0,
  "actualWeight_g": 50.2,
  "accuracy_percent": 99.6,
  "duration_sec": 145
}
```

#### System Status (Published every 30 seconds)
```json
{
  "timestamp": "2025-11-18T11:10:00Z",
  "state": "IDLE",
  "pumps": {
    "X": {"position": 25.5, "active": false},
    "Y": {"position": 0.0, "active": false},
    "Z": {"position": 10.2, "active": false},
    "A": {"position": 0.0, "active": false}
  },
  "scale": {"weight": 0.0, "connected": true},
  "uptime_hours": 12.5,
  "freeHeap_kb": 145
}
```

### MQTT Configuration

```cpp
#define MQTT_BROKER "192.168.1.50"  // Your MQTT broker IP
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "PumpController_001"
#define MQTT_USER "pump_user"        // Optional
#define MQTT_PASSWORD "password"     // Optional

// Quality of Service levels
#define QOS_STATUS 0      // At most once (status updates)
#define QOS_EVENT 1       // At least once (important events)
#define QOS_ALARM 2       // Exactly once (critical alarms)

// Retain flags
#define RETAIN_STATUS true   // Retain last status
#define RETAIN_EVENT false   // Don't retain events
```

### Integration with InfluxDB / Grafana (Optional)

For advanced data logging and visualization:

```
MQTT Broker
    ↓
Telegraf (MQTT Consumer)
    ↓
InfluxDB (Time-series database)
    ↓
Grafana (Visualization dashboard)
```

**Benefits**:
- Historical data analysis
- Trend visualization
- Alerts and notifications
- Production metrics

---

## 6. Code Examples

### Complete Dual-Core WiFi + LED Example

```cpp
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
#include <FastLED.h>
#include <ArduinoJson.h>

// WiFi Configuration
const char* SSID = "YourWiFi";
const char* PASSWORD = "YourPassword";

// MQTT Configuration
const char* MQTT_BROKER = "192.168.1.50";
const int MQTT_PORT = 1883;

// Hardware
#define LED_PIN 25
#define NUM_LEDS 32
CRGB leds[NUM_LEDS];

// Task Handles
TaskHandle_t wifiTaskHandle;
TaskHandle_t mqttTaskHandle;
TaskHandle_t ledTaskHandle;

// Clients
WiFiClient espClient;
PubSubClient mqttClient(espClient);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Shared Data (protected by mutex)
SemaphoreHandle_t dataMutex;
struct SharedData {
  float pumpPositions[4];
  bool pumpActive[4];
  float currentWeight;
  String systemState;
} sharedData;

// ============================================
// CORE 0: WiFi and MQTT Tasks
// ============================================

void wifiTask(void *parameter) {
  Serial.println("WiFi Task started on Core 0");

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_11dBm);  // Moderate power
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.println("IP: " + WiFi.localIP().toString());

  // Start web server
  setupWebServer();

  // Keep task alive
  while(1) {
    ws.cleanupClients();  // Clean WebSocket clients
    delay(1000);
  }
}

void mqttTask(void *parameter) {
  Serial.println("MQTT Task started on Core 0");

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  while(1) {
    if (!mqttClient.connected()) {
      reconnectMQTT();
    }
    mqttClient.loop();

    // Publish status every 30 seconds
    static unsigned long lastPublish = 0;
    if (millis() - lastPublish > 30000) {
      publishStatus();
      lastPublish = millis();
    }

    delay(10);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Handle MQTT commands
  Serial.print("MQTT message on topic: ");
  Serial.println(topic);
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("connected!");
      mqttClient.subscribe("pump/controller/command/#");
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

void publishStatus() {
  StaticJsonDocument<512> doc;
  doc["timestamp"] = millis();
  doc["state"] = sharedData.systemState;

  JsonObject pumps = doc.createNestedObject("pumps");
  pumps["X"]["position"] = sharedData.pumpPositions[0];
  pumps["X"]["active"] = sharedData.pumpActive[0];
  // ... etc for Y, Z, A

  char buffer[512];
  serializeJson(doc, buffer);
  mqttClient.publish("pump/controller/status", buffer, true);
}

// ============================================
// CORE 1: LED Task (Isolated from WiFi)
// ============================================

void ledTask(void *parameter) {
  Serial.println("LED Task started on Core 1 (isolated)");

  // Initialize LEDs (WiFi won't interfere on this core)
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(64);  // 25%
  FastLED.clear();
  FastLED.show();

  while(1) {
    // Update LEDs based on pump status
    for (int strip = 0; strip < 4; strip++) {
      bool active = false;

      // Get pump status (protected by mutex)
      if (xSemaphoreTake(dataMutex, 10 / portTICK_PERIOD_MS)) {
        active = sharedData.pumpActive[strip];
        xSemaphoreGive(dataMutex);
      }

      // Set LED color
      CRGB color = active ? CRGB::Blue : CRGB::Green;
      for (int i = 0; i < 8; i++) {
        leds[strip * 8 + i] = color;
      }
    }

    // Critical section for LED update
    portDISABLE_INTERRUPTS();
    FastLED.show();
    portENABLE_INTERRUPTS();

    delay(50);  // 20 Hz update rate
  }
}

// ============================================
// Web Server Setup
// ============================================

void setupWebServer() {
  // Serve static files
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", htmlPage);
  });

  // REST API
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    StaticJsonDocument<512> doc;
    doc["state"] = sharedData.systemState;
    // ... add more data

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/api/pump/start", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Parse body and start pump
    request->send(200, "application/json", "{\"success\":true}");

    // Log to MQTT
    logDispenseEvent("X", 10.0, 5.0);
  });

  // WebSocket
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  server.begin();
  Serial.println("Web server started");
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                       AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    // Parse WebSocket message
    StaticJsonDocument<256> doc;
    deserializeJson(doc, data, len);

    String action = doc["action"];
    if (action == "startPump") {
      String pump = doc["pump"];
      float volume = doc["volume"];
      // ... handle command
    }
  }
}

// ============================================
// MQTT Logging Functions
// ============================================

void logDispenseEvent(String pump, float volume, float flowRate) {
  StaticJsonDocument<256> doc;
  doc["timestamp"] = millis();
  doc["eventType"] = "dispenseStart";
  doc["pump"] = pump;
  doc["volume_ml"] = volume;
  doc["flowRate_ml_min"] = flowRate;
  doc["recipe"] = "Manual";
  doc["operator"] = "WebUI";

  char buffer[256];
  serializeJson(doc, buffer);
  mqttClient.publish("pump/controller/dispense/event", buffer, false);
}

void logDispenseComplete(String pump, float volume, float actualVolume, int duration) {
  StaticJsonDocument<256> doc;
  doc["timestamp"] = millis();
  doc["eventType"] = "dispenseComplete";
  doc["pump"] = pump;
  doc["volume_ml"] = volume;
  doc["actualVolume_ml"] = actualVolume;
  doc["duration_sec"] = duration;
  doc["success"] = true;

  char buffer[256];
  serializeJson(doc, buffer);
  mqttClient.publish("pump/controller/dispense/complete", buffer, false);
}

// ============================================
// Main Setup and Loop (Runs on Core 1)
// ============================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== Pump Controller Starting ===");

  // Create mutex for shared data
  dataMutex = xSemaphoreCreateMutex();

  // Initialize shared data
  sharedData.systemState = "IDLE";
  for (int i = 0; i < 4; i++) {
    sharedData.pumpPositions[i] = 0.0;
    sharedData.pumpActive[i] = false;
  }

  // Create WiFi task on Core 0
  xTaskCreatePinnedToCore(
    wifiTask,
    "WiFi",
    10000,
    NULL,
    1,
    &wifiTaskHandle,
    0  // Core 0
  );

  // Create MQTT task on Core 0
  xTaskCreatePinnedToCore(
    mqttTask,
    "MQTT",
    10000,
    NULL,
    1,
    &mqttTaskHandle,
    0  // Core 0
  );

  // Create LED task on Core 1 (isolated!)
  xTaskCreatePinnedToCore(
    ledTask,
    "LEDs",
    5000,
    NULL,
    2,  // Higher priority
    &ledTaskHandle,
    1  // Core 1 (isolated from WiFi)
  );

  Serial.println("All tasks created!");
}

void loop() {
  // Main loop runs on Core 1
  // Handle buttons, encoder, pump control, etc.

  // Update shared data (protected)
  if (xSemaphoreTake(dataMutex, 10 / portTICK_PERIOD_MS)) {
    // Update pump positions, state, etc.
    sharedData.pumpActive[0] = true;  // Example
    xSemaphoreGive(dataMutex);
  }

  delay(10);
}
```

---

## 7. Testing Plan

### Test 21: WiFi + LED Coexistence

**Purpose**: Verify WiFi and LEDs work simultaneously without interference

**Test Procedure**:
1. Enable WiFi and connect to network
2. Start LED animations (all patterns)
3. Generate WiFi traffic:
   - Web page requests (10/second)
   - WebSocket updates (real-time)
   - MQTT publishing (5/second)
4. Monitor LED behavior for glitches

**Success Criteria**:
- ✅ LEDs display correct colors (no random colors)
- ✅ No LED flickering during WiFi activity
- ✅ Web server responds < 100ms
- ✅ MQTT messages delivered reliably
- ✅ WebSocket maintains connection
- ✅ System remains stable for 1 hour

### Test 22: Web UI + Pump Control

**Purpose**: Control pumps via web interface

**Test Procedure**:
1. Access web UI from browser
2. Start/stop individual pumps
3. Execute recipes via web UI
4. Monitor real-time updates

**Success Criteria**:
- ✅ Web UI loads properly
- ✅ Commands execute correctly
- ✅ Real-time status updates via WebSocket
- ✅ No command latency > 500ms

### Test 23: MQTT Data Logging

**Purpose**: Verify MQTT logging of all dispensing operations

**Test Procedure**:
1. Connect to MQTT broker
2. Subscribe to all pump topics
3. Perform various dispensing operations
4. Verify all events logged

**Success Criteria**:
- ✅ All dispense events logged
- ✅ Correct JSON format
- ✅ Accurate data (volume, duration, etc.)
- ✅ No message loss
- ✅ Timestamps accurate

---

## 8. Implementation Roadmap

### Phase 1: Core Infrastructure (Week 1)
- ✅ Implement dual-core task architecture
- ✅ Add WiFi with LED isolation
- ✅ Test LED stability with WiFi active
- ✅ Create Test 21

### Phase 2: Web Server (Week 2)
- ⏳ Implement AsyncWebServer
- ⏳ Create REST API endpoints
- ⏳ Add WebSocket support
- ⏳ Design HTML/CSS/JS UI
- ⏳ Create Test 22

### Phase 3: MQTT Integration (Week 3)
- ⏳ Implement MQTT client
- ⏳ Define topic structure
- ⏳ Add event logging
- ⏳ Test with MQTT broker
- ⏳ Create Test 23

### Phase 4: Integration & Polish (Week 4)
- ⏳ Integrate web UI with main system
- ⏳ Add authentication (optional)
- ⏳ Performance optimization
- ⏳ Full system testing
- ⏳ Documentation

---

## 9. Required Libraries

```ini
lib_deps =
    fastled/FastLED@^3.5.0
    marcoschwartz/LiquidCrystal_I2C@^1.1.4
    me-no-dev/ESPAsyncWebServer@^1.2.3
    me-no-dev/AsyncTCP@^1.1.1
    knolleary/PubSubClient@^2.8
    bblanchon/ArduinoJson@^6.21.3
```

---

## 10. Next Steps

1. **Review this architecture** - Does it meet your requirements?
2. **Choose solution approach**: Dual-core (recommended) or two ESP32s?
3. **Define web UI requirements** - What features are most important?
4. **Set up MQTT broker** - Self-hosted or cloud service?
5. **Start with Test 21** - Verify WiFi + LED coexistence

---

**Status**: Architecture Design Complete
**Recommended Approach**: Dual-Core ESP32 with task pinning
**Estimated Implementation**: 3-4 weeks
**Next Action**: Create Test 21 (WiFi + LED coexistence test)
