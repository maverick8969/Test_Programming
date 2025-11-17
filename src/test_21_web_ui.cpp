/**
 * Test 21: Web UI for Recipe/Pump Control
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - 4 peristaltic pumps
 * - WiFi connection required
 *
 * Features:
 * - Web-based recipe management and control
 * - Real-time status updates via WebSocket
 * - Individual pump control
 * - Emergency stop functionality
 * - Mobile-responsive interface
 *
 * Build command:
 *   pio run -e test_21_web_ui -t upload -t monitor
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include "pin_definitions.h"

// WiFi Configuration - UPDATE THESE
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASSWORD = "YourWiFiPassword";

#define UartSerial         Serial2

// Web server and WebSocket
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Recipe and pump structures
struct Ingredient {
    char pump;
    float volumeMl;
    float flowRateMlMin;
};

struct Recipe {
    const char* name;
    Ingredient* ingredients;
    int stepCount;
};

// Define recipes
Ingredient cleaningRecipe[] = {
    {'X', 5.0, 30.0},
    {'Y', 5.0, 30.0},
    {'Z', 5.0, 30.0},
    {'A', 5.0, 30.0}
};

Ingredient colorMixRecipe[] = {
    {'X', 10.0, 15.0},  // Cyan base
    {'Y', 5.0, 10.0},   // Magenta
    {'Z', 2.5, 10.0}    // Yellow
};

Ingredient nutrientMixRecipe[] = {
    {'X', 20.0, 25.0},  // Water
    {'Y', 2.0, 5.0},    // Concentrate A
    {'Z', 1.5, 5.0},    // Concentrate B
    {'A', 0.5, 2.0}     // Additive
};

Ingredient customTestRecipe[] = {
    {'X', 15.0, 20.0},
    {'Y', 10.0, 15.0}
};

Recipe recipes[] = {
    {"Cleaning Flush", cleaningRecipe, 4},
    {"Color Mix", colorMixRecipe, 3},
    {"Nutrient Mix", nutrientMixRecipe, 4},
    {"Custom Test", customTestRecipe, 2}
};
const int recipeCount = 4;

enum SystemMode { MODE_IDLE, MODE_EXECUTING, MODE_MANUAL };
SystemMode mode = MODE_IDLE;

int currentRecipe = -1;
int currentStep = 0;
bool waitingForCompletion = false;

const float ML_PER_MM = 0.05;
const float SAFE_TEST_FEEDRATE = 300.0;

// Pump status tracking
struct PumpStatus {
    bool running;
    float currentFlowRate;
    float targetVolume;
    float dispensedVolume;
};
PumpStatus pumpStatus[4] = {{false, 0, 0, 0}, {false, 0, 0, 0}, {false, 0, 0, 0}, {false, 0, 0, 0}};

// System status
String systemState = "Idle";
String lastError = "";

// G-code communication
void sendCommand(const char* cmd) {
    Serial.print("→ ");
    Serial.println(cmd);
    UartSerial.println(cmd);
    UartSerial.flush();
}

// Broadcast status to all WebSocket clients
void broadcastStatus() {
    StaticJsonDocument<512> doc;
    doc["mode"] = (mode == MODE_IDLE) ? "idle" : (mode == MODE_EXECUTING) ? "executing" : "manual";
    doc["systemState"] = systemState;
    doc["currentRecipe"] = currentRecipe;
    doc["currentStep"] = currentStep;
    doc["lastError"] = lastError;

    JsonArray pumps = doc.createNestedArray("pumps");
    for (int i = 0; i < 4; i++) {
        JsonObject pump = pumps.createNestedObject();
        pump["id"] = i;
        pump["running"] = pumpStatus[i].running;
        pump["flowRate"] = pumpStatus[i].currentFlowRate;
        pump["targetVolume"] = pumpStatus[i].targetVolume;
        pump["dispensed"] = pumpStatus[i].dispensedVolume;
    }

    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

// Execute recipe step
void executeRecipeStep(Recipe& recipe, int step) {
    if (step >= recipe.stepCount) {
        Serial.println("\n✓ Recipe complete!");
        mode = MODE_IDLE;
        systemState = "Recipe Complete";
        currentRecipe = -1;
        currentStep = 0;
        broadcastStatus();
        return;
    }

    Ingredient ing = recipe.ingredients[step];
    float distMm = ing.volumeMl / ML_PER_MM;
    float feedRate = ing.flowRateMlMin / ML_PER_MM;

    if (feedRate > SAFE_TEST_FEEDRATE) {
        feedRate = SAFE_TEST_FEEDRATE;
    }

    Serial.println("\n[" + String(recipe.name) + "]");
    Serial.print("Step ");
    Serial.print(step + 1);
    Serial.print("/");
    Serial.println(recipe.stepCount);
    Serial.print("Pump ");
    Serial.print(ing.pump);
    Serial.print(": ");
    Serial.print(ing.volumeMl);
    Serial.print("ml @ ");
    Serial.print(ing.flowRateMlMin);
    Serial.println("ml/min");

    // Update pump status
    int pumpIdx = (ing.pump == 'X') ? 0 : (ing.pump == 'Y') ? 1 : (ing.pump == 'Z') ? 2 : 3;
    pumpStatus[pumpIdx].running = true;
    pumpStatus[pumpIdx].currentFlowRate = ing.flowRateMlMin;
    pumpStatus[pumpIdx].targetVolume = ing.volumeMl;
    pumpStatus[pumpIdx].dispensedVolume = 0;

    systemState = String(recipe.name) + " - Step " + String(step + 1) + "/" + String(recipe.stepCount);

    // Reset position
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "G92 %c0", ing.pump);
    sendCommand(cmd);
    delay(100);

    // Execute
    snprintf(cmd, sizeof(cmd), "G1 %c%.2f F%.1f", ing.pump, distMm, feedRate);
    sendCommand(cmd);

    waitingForCompletion = true;
    broadcastStatus();
}

// Start recipe
void startRecipe(int recipeIndex) {
    if (recipeIndex < 0 || recipeIndex >= recipeCount) {
        Serial.println("Invalid recipe index");
        lastError = "Invalid recipe index";
        broadcastStatus();
        return;
    }

    currentRecipe = recipeIndex;
    currentStep = 0;
    mode = MODE_EXECUTING;

    Serial.println("\nStarting recipe: " + String(recipes[recipeIndex].name));
    systemState = "Starting " + String(recipes[recipeIndex].name);
    broadcastStatus();

    delay(1000);
    executeRecipeStep(recipes[currentRecipe], currentStep);
}

// Stop all pumps
void stopAll() {
    sendCommand("!");
    Serial.println("Emergency stop");
    mode = MODE_IDLE;
    systemState = "Stopped";

    for (int i = 0; i < 4; i++) {
        pumpStatus[i].running = false;
    }

    broadcastStatus();
}

// Start individual pump
void startPump(char pumpAxis, float flowRateMlMin) {
    float feedRate = flowRateMlMin / ML_PER_MM;
    if (feedRate > SAFE_TEST_FEEDRATE) {
        feedRate = SAFE_TEST_FEEDRATE;
    }

    char cmd[64];
    snprintf(cmd, sizeof(cmd), "G92 %c0", pumpAxis);
    sendCommand(cmd);
    delay(100);

    snprintf(cmd, sizeof(cmd), "G91 G1 %c1000 F%.1f", pumpAxis, feedRate);
    sendCommand(cmd);

    int pumpIdx = (pumpAxis == 'X') ? 0 : (pumpAxis == 'Y') ? 1 : (pumpAxis == 'Z') ? 2 : 3;
    pumpStatus[pumpIdx].running = true;
    pumpStatus[pumpIdx].currentFlowRate = flowRateMlMin;

    mode = MODE_MANUAL;
    systemState = "Manual Control - Pump " + String(pumpAxis);

    Serial.printf("Started pump %c at %.1f ml/min\n", pumpAxis, flowRateMlMin);
    broadcastStatus();
}

// WebSocket event handler
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                       void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client #%u connected\n", client->id());
        broadcastStatus();
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    } else if (type == WS_EVT_DATA) {
        // Handle incoming WebSocket data if needed
    }
}

// HTML web page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Pump Control System</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        .header {
            background: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
            margin-bottom: 20px;
        }
        .header h1 {
            color: #333;
            margin-bottom: 10px;
        }
        .status {
            display: flex;
            gap: 10px;
            flex-wrap: wrap;
        }
        .status-badge {
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 14px;
            font-weight: 500;
        }
        .status-idle {
            background: #10b981;
            color: white;
        }
        .status-executing {
            background: #3b82f6;
            color: white;
        }
        .status-manual {
            background: #f59e0b;
            color: white;
        }
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 20px;
        }
        .card {
            background: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        .card h2 {
            color: #333;
            margin-bottom: 15px;
            font-size: 18px;
        }
        .recipe-list {
            display: flex;
            flex-direction: column;
            gap: 10px;
        }
        .recipe-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 12px;
            background: #f3f4f6;
            border-radius: 8px;
            transition: all 0.2s;
        }
        .recipe-item:hover {
            background: #e5e7eb;
        }
        .recipe-name {
            font-weight: 500;
            color: #333;
        }
        .recipe-steps {
            font-size: 12px;
            color: #6b7280;
        }
        button {
            padding: 8px 16px;
            border: none;
            border-radius: 6px;
            font-size: 14px;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.2s;
        }
        .btn-primary {
            background: #3b82f6;
            color: white;
        }
        .btn-primary:hover {
            background: #2563eb;
        }
        .btn-danger {
            background: #ef4444;
            color: white;
        }
        .btn-danger:hover {
            background: #dc2626;
        }
        .btn-success {
            background: #10b981;
            color: white;
        }
        .btn-success:hover {
            background: #059669;
        }
        .emergency-stop {
            background: #dc2626;
            color: white;
            padding: 20px;
            border-radius: 10px;
            text-align: center;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        .emergency-stop button {
            width: 100%;
            padding: 16px;
            font-size: 18px;
            font-weight: bold;
        }
        .pump-control {
            display: flex;
            flex-direction: column;
            gap: 8px;
        }
        .pump-row {
            display: flex;
            align-items: center;
            gap: 10px;
            padding: 10px;
            background: #f3f4f6;
            border-radius: 8px;
        }
        .pump-name {
            flex: 0 0 60px;
            font-weight: 500;
        }
        .pump-status {
            flex: 0 0 80px;
            font-size: 12px;
            padding: 4px 8px;
            border-radius: 12px;
            text-align: center;
        }
        .pump-running {
            background: #10b981;
            color: white;
        }
        .pump-stopped {
            background: #6b7280;
            color: white;
        }
        input[type="range"] {
            flex: 1;
        }
        .flow-value {
            flex: 0 0 80px;
            text-align: right;
            font-size: 14px;
            color: #6b7280;
        }
        .connection-status {
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: 500;
        }
        .connected {
            background: #10b981;
            color: white;
        }
        .disconnected {
            background: #ef4444;
            color: white;
        }
        @media (max-width: 768px) {
            .pump-row {
                flex-direction: column;
                align-items: stretch;
            }
            .pump-name, .pump-status, .flow-value {
                flex: 1;
                text-align: left;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🧪 Peristaltic Pump Control System</h1>
            <div class="status">
                <span id="connectionStatus" class="connection-status disconnected">Disconnected</span>
                <span id="systemStatus" class="status-badge status-idle">Idle</span>
                <span id="currentRecipe" style="display:none;" class="status-badge" style="background:#6b7280;color:white;"></span>
            </div>
        </div>

        <div class="grid">
            <div class="card">
                <h2>📋 Recipes</h2>
                <div id="recipeList" class="recipe-list">
                    <!-- Recipes will be loaded here -->
                </div>
            </div>

            <div class="card">
                <h2>⚙️ Manual Pump Control</h2>
                <div class="pump-control">
                    <div class="pump-row">
                        <span class="pump-name">Pump X</span>
                        <span id="pump0Status" class="pump-status pump-stopped">Stopped</span>
                        <input type="range" id="pump0Flow" min="0" max="100" value="30" step="5">
                        <span class="flow-value"><span id="pump0Value">30</span> ml/min</span>
                        <button class="btn-success" onclick="startPump('X', 0)">Start</button>
                    </div>
                    <div class="pump-row">
                        <span class="pump-name">Pump Y</span>
                        <span id="pump1Status" class="pump-status pump-stopped">Stopped</span>
                        <input type="range" id="pump1Flow" min="0" max="100" value="30" step="5">
                        <span class="flow-value"><span id="pump1Value">30</span> ml/min</span>
                        <button class="btn-success" onclick="startPump('Y', 1)">Start</button>
                    </div>
                    <div class="pump-row">
                        <span class="pump-name">Pump Z</span>
                        <span id="pump2Status" class="pump-status pump-stopped">Stopped</span>
                        <input type="range" id="pump2Flow" min="0" max="100" value="30" step="5">
                        <span class="flow-value"><span id="pump2Value">30</span> ml/min</span>
                        <button class="btn-success" onclick="startPump('Z', 2)">Start</button>
                    </div>
                    <div class="pump-row">
                        <span class="pump-name">Pump A</span>
                        <span id="pump3Status" class="pump-status pump-stopped">Stopped</span>
                        <input type="range" id="pump3Flow" min="0" max="100" value="30" step="5">
                        <span class="flow-value"><span id="pump3Value">30</span> ml/min</span>
                        <button class="btn-success" onclick="startPump('A', 3)">Start</button>
                    </div>
                </div>
            </div>
        </div>

        <div class="emergency-stop">
            <button class="btn-danger" onclick="emergencyStop()">⛔ EMERGENCY STOP</button>
        </div>
    </div>

    <script>
        let ws;
        let wsConnected = false;

        // Initialize WebSocket connection
        function initWebSocket() {
            ws = new WebSocket('ws://' + window.location.hostname + '/ws');

            ws.onopen = function() {
                console.log('WebSocket connected');
                wsConnected = true;
                document.getElementById('connectionStatus').textContent = 'Connected';
                document.getElementById('connectionStatus').className = 'connection-status connected';
            };

            ws.onclose = function() {
                console.log('WebSocket disconnected');
                wsConnected = false;
                document.getElementById('connectionStatus').textContent = 'Disconnected';
                document.getElementById('connectionStatus').className = 'connection-status disconnected';
                setTimeout(initWebSocket, 5000);
            };

            ws.onmessage = function(event) {
                const data = JSON.parse(event.data);
                updateStatus(data);
            };
        }

        // Update UI with status data
        function updateStatus(data) {
            const statusBadge = document.getElementById('systemStatus');
            statusBadge.textContent = data.systemState;

            if (data.mode === 'idle') {
                statusBadge.className = 'status-badge status-idle';
            } else if (data.mode === 'executing') {
                statusBadge.className = 'status-badge status-executing';
            } else {
                statusBadge.className = 'status-badge status-manual';
            }

            // Update pump status
            if (data.pumps) {
                data.pumps.forEach(pump => {
                    const statusEl = document.getElementById('pump' + pump.id + 'Status');
                    if (pump.running) {
                        statusEl.textContent = 'Running';
                        statusEl.className = 'pump-status pump-running';
                    } else {
                        statusEl.textContent = 'Stopped';
                        statusEl.className = 'pump-status pump-stopped';
                    }
                });
            }
        }

        // Start recipe
        function startRecipe(index) {
            fetch('/api/recipe/start/' + index, {method: 'POST'})
                .then(response => response.json())
                .then(data => console.log(data))
                .catch(error => console.error('Error:', error));
        }

        // Start individual pump
        function startPump(axis, index) {
            const flowRate = document.getElementById('pump' + index + 'Flow').value;
            fetch('/api/pump/start', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({pump: axis, flowRate: parseFloat(flowRate)})
            })
            .then(response => response.json())
            .then(data => console.log(data))
            .catch(error => console.error('Error:', error));
        }

        // Emergency stop
        function emergencyStop() {
            fetch('/api/stop', {method: 'POST'})
                .then(response => response.json())
                .then(data => console.log(data))
                .catch(error => console.error('Error:', error));
        }

        // Load recipes
        function loadRecipes() {
            fetch('/api/recipes')
                .then(response => response.json())
                .then(data => {
                    const recipeList = document.getElementById('recipeList');
                    recipeList.innerHTML = '';

                    data.recipes.forEach((recipe, index) => {
                        const item = document.createElement('div');
                        item.className = 'recipe-item';
                        item.innerHTML = `
                            <div>
                                <div class="recipe-name">${recipe.name}</div>
                                <div class="recipe-steps">${recipe.steps} steps</div>
                            </div>
                            <button class="btn-primary" onclick="startRecipe(${index})">Start</button>
                        `;
                        recipeList.appendChild(item);
                    });
                })
                .catch(error => console.error('Error:', error));
        }

        // Update flow rate display
        for (let i = 0; i < 4; i++) {
            document.getElementById('pump' + i + 'Flow').addEventListener('input', function() {
                document.getElementById('pump' + i + 'Value').textContent = this.value;
            });
        }

        // Initialize
        window.addEventListener('load', function() {
            loadRecipes();
            initWebSocket();
        });
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║           Test 21: Web UI for Recipe Control             ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");

    // Initialize UART for pump control
    UartSerial.begin(115200, SERIAL_8N1, UART_TEST_RX_PIN, UART_TEST_TX_PIN);
    Serial.println("✓ UART initialized");

    // Connect to WiFi
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✓ WiFi connected");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n✗ WiFi connection failed");
        Serial.println("Starting in AP mode...");
        WiFi.softAP("PumpControl", "12345678");
        Serial.print("AP IP Address: ");
        Serial.println(WiFi.softAPIP());
    }

    // Setup WebSocket
    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);

    // API Routes
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });

    server.on("/api/recipes", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<512> doc;
        JsonArray recipesJson = doc.createNestedArray("recipes");

        for (int i = 0; i < recipeCount; i++) {
            JsonObject recipe = recipesJson.createNestedObject();
            recipe["name"] = recipes[i].name;
            recipe["steps"] = recipes[i].stepCount;
        }

        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    server.on("/api/recipe/start/*", HTTP_POST, [](AsyncWebServerRequest *request) {
        String path = request->url();
        int recipeIndex = path.substring(path.lastIndexOf('/') + 1).toInt();

        startRecipe(recipeIndex);

        request->send(200, "application/json", "{\"success\":true}");
    });

    server.on("/api/pump/start", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            StaticJsonDocument<128> doc;
            deserializeJson(doc, data);

            const char* pump = doc["pump"];
            float flowRate = doc["flowRate"];

            startPump(pump[0], flowRate);

            request->send(200, "application/json", "{\"success\":true}");
        });

    server.on("/api/stop", HTTP_POST, [](AsyncWebServerRequest *request) {
        stopAll();
        request->send(200, "application/json", "{\"success\":true}");
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<512> doc;
        doc["mode"] = (mode == MODE_IDLE) ? "idle" : (mode == MODE_EXECUTING) ? "executing" : "manual";
        doc["systemState"] = systemState;
        doc["currentRecipe"] = currentRecipe;
        doc["currentStep"] = currentStep;

        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    server.begin();
    Serial.println("✓ Web server started");
    Serial.println("\nAccess the web UI at:");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("http://" + WiFi.localIP().toString());
    } else {
        Serial.println("http://" + WiFi.softAPIP().toString());
    }
    Serial.println("\nSystem ready!");

    // Query initial status
    sendCommand("?");
}

void loop() {
    ws.cleanupClients();

    // Process UART responses
    if (UartSerial.available()) {
        String response = UartSerial.readStringUntil('\n');
        response.trim();
        Serial.print("← ");
        Serial.println(response);

        // Parse system state from response
        if (response.startsWith("<")) {
            if (response.indexOf("Idle") >= 0) {
                systemState = "Idle";
            } else if (response.indexOf("Run") >= 0) {
                systemState = "Running";
            }
        }

        // Check completion
        if (waitingForCompletion && response.indexOf("Idle") >= 0) {
            waitingForCompletion = false;
            currentStep++;
            delay(500);
            if (mode == MODE_EXECUTING) {
                executeRecipeStep(recipes[currentRecipe], currentStep);
            }
        }
    }

    // Periodic status update
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 1000) {
        lastUpdate = millis();
        broadcastStatus();
    }

    delay(1);
}
