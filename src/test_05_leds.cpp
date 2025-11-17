/**
 * Test 05: WS2812B Addressable RGB LEDs
 *
 * Hardware:
 * - 32 WS2812B LEDs (4 strips × 8 LEDs each)
 * - Data pin: GPIO 25
 * - Power: 5V (ensure adequate current capability)
 *
 * LED Strip Mapping (for future pump control):
 * - Strip 0 (LEDs 0-7):   Pump 1 → Cyan
 * - Strip 1 (LEDs 8-15):  Pump 2 → Magenta
 * - Strip 2 (LEDs 16-23): Pump 3 → Yellow
 * - Strip 3 (LEDs 24-31): Pump 4 → White
 *
 * Test Patterns:
 * 1. All Off
 * 2. Solid Colors (Red → Green → Blue → White)
 * 3. Rainbow Pattern
 * 4. Chase Effect
 * 5. Per-Strip Control (show pump assignment)
 *
 * IMPORTANT - LED Data Corruption Fix:
 * WS2812B LEDs require precise timing (±150ns tolerance). On ESP32, WiFi and
 * Bluetooth radio activity causes timing jitter that corrupts LED data.
 * This test now:
 *   1. Disables WiFi and Bluetooth before LED initialization
 *   2. Clears LED buffer to remove garbage data
 *   3. Adds stabilization delay for RMT peripheral
 * These changes prevent random colors and data corruption on GPIO25.
 *
 * POWER SUPPLY WARNING - Logic Level Mismatch:
 * WS2812B requires data HIGH ≥ 0.7×VDD. ESP32 outputs 3.3V max.
 *   - ESP32 5V regulator (under load): ~4.5V → threshold 3.15V → Works! ✓
 *   - External 5V supply: 5.0V → threshold 3.5V → 3.3V is marginal! ✗
 *
 * If LEDs are erratic on external 5V supply, fix in this order:
 *   1. CRITICAL: Connect ESP32 GND to 5V supply negative/GND to LED GND
 *      Without common ground reference, LEDs won't work at all!
 *      (GND = DC negative terminal, not earth ground)
 *   2. Add 1N4001 diode in LED 5V+ line (drops to 4.3V, solves logic level)
 *   3. OR: Add 74HCT245 level shifter (best for production, ~$0.50)
 *   4. OR: Add 330Ω resistor on GPIO25 data line (reduces reflections)
 *
 * Build command:
 *   pio run -e test_05_leds -t upload -t monitor
 */

#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include "esp_bt.h"
#include "pin_definitions.h"

// LED Configuration
#define NUM_LEDS        32
#define LEDS_PER_STRIP  8
#define NUM_STRIPS      4
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB
#define BRIGHTNESS      64      // 0-255, start at 25% for testing
#define MAX_BRIGHTNESS  255

// LED array
CRGB leds[NUM_LEDS];

// Pattern state
uint8_t currentPattern = 0;
uint8_t totalPatterns = 5;
unsigned long lastPatternChange = 0;
unsigned long patternDuration = 3000;  // 3 seconds per pattern
uint8_t animationStep = 0;

// Pump color assignments (for future use)
const CRGB pumpColors[NUM_STRIPS] = {
    CRGB::Cyan,      // Pump 1
    CRGB::Magenta,   // Pump 2
    CRGB::Yellow,    // Pump 3
    CRGB::White      // Pump 4
};

const char* pumpNames[NUM_STRIPS] = {
    "Pump 1", "Pump 2", "Pump 3", "Pump 4"
};

/**
 * Set color for specific LED strip (0-3)
 */
void setStripColor(uint8_t stripNum, CRGB color) {
    if (stripNum >= NUM_STRIPS) return;

    uint16_t startLED = stripNum * LEDS_PER_STRIP;
    uint16_t endLED = startLED + LEDS_PER_STRIP;

    for (uint16_t i = startLED; i < endLED; i++) {
        leds[i] = color;
    }
}

/**
 * Set all LEDs to one color
 */
void setAllColor(CRGB color) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        leds[i] = color;
    }
}

/**
 * Pattern 0: All Off
 */
void patternAllOff() {
    setAllColor(CRGB::Black);
}

/**
 * Pattern 1: Solid Colors (cycle through)
 */
void patternSolidColors() {
    CRGB colors[] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::White};
    uint8_t colorIndex = (animationStep / 30) % 4;  // Change every 30 frames (~1 sec)
    setAllColor(colors[colorIndex]);
}

/**
 * Pattern 2: Rainbow
 */
void patternRainbow() {
    uint8_t hue = animationStep * 2;  // Full spectrum over ~128 frames
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        leds[i] = CHSV(hue + (i * 256 / NUM_LEDS), 255, 255);
    }
}

/**
 * Pattern 3: Chase Effect
 */
void patternChase() {
    setAllColor(CRGB::Black);
    uint8_t pos = animationStep % NUM_LEDS;
    leds[pos] = CRGB::Blue;
    if (pos > 0) leds[pos - 1] = CRGB::DarkBlue;
    if (pos > 1) leds[pos - 2] = CRGB::Navy;
}

/**
 * Pattern 4: Per-Strip Control (show pump assignments)
 */
void patternPerStrip() {
    for (uint8_t strip = 0; strip < NUM_STRIPS; strip++) {
        // Pulse brightness for current strip
        uint8_t brightness = 255;
        if ((animationStep / 20) % NUM_STRIPS == strip) {
            brightness = beatsin8(60, 100, 255);  // Pulse the active strip
        }

        CRGB color = pumpColors[strip];
        color.nscale8(brightness);
        setStripColor(strip, color);
    }
}

/**
 * Display current pattern on LEDs
 */
void updatePattern() {
    switch (currentPattern) {
        case 0: patternAllOff(); break;
        case 1: patternSolidColors(); break;
        case 2: patternRainbow(); break;
        case 3: patternChase(); break;
        case 4: patternPerStrip(); break;
        default: patternAllOff(); break;
    }

    FastLED.show();
    animationStep++;
}

/**
 * Print current pattern info
 */
void printPatternInfo() {
    Serial.println("\n============================================================");
    Serial.print("Pattern "); Serial.print(currentPattern + 1);
    Serial.print("/"); Serial.print(totalPatterns);
    Serial.print(": ");

    switch (currentPattern) {
        case 0:
            Serial.println("All Off");
            Serial.println("→ All LEDs should be off");
            break;
        case 1:
            Serial.println("Solid Colors");
            Serial.println("→ All LEDs cycle: Red → Green → Blue → White");
            break;
        case 2:
            Serial.println("Rainbow");
            Serial.println("→ Smooth rainbow spectrum across all LEDs");
            break;
        case 3:
            Serial.println("Chase Effect");
            Serial.println("→ Single blue LED chasing around the strip");
            break;
        case 4:
            Serial.println("Per-Strip Control (Pump Assignments)");
            Serial.println("→ Each strip shows its assigned pump color:");
            for (uint8_t i = 0; i < NUM_STRIPS; i++) {
                Serial.print("   Strip "); Serial.print(i);
                Serial.print(" (LEDs "); Serial.print(i * LEDS_PER_STRIP);
                Serial.print("-"); Serial.print((i + 1) * LEDS_PER_STRIP - 1);
                Serial.print("): "); Serial.print(pumpNames[i]);
                Serial.print(" → ");

                CRGB color = pumpColors[i];
                if (color == CRGB::Cyan) Serial.println("Cyan");
                else if (color == CRGB::Magenta) Serial.println("Magenta");
                else if (color == CRGB::Yellow) Serial.println("Yellow");
                else if (color == CRGB::White) Serial.println("White");
            }
            Serial.println("→ Strips pulse when highlighted");
            break;
    }

    Serial.println("============================================================");
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║         Test 05: WS2812B Addressable RGB LEDs             ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");

    // Disable WiFi and Bluetooth to prevent timing interference with WS2812B
    Serial.println("\n[Disabling Wireless Radios]");
    WiFi.mode(WIFI_OFF);
    btStop();
    Serial.println("✓ WiFi and Bluetooth disabled (prevents LED data corruption)");

    // Hardware configuration
    Serial.println("\n[Hardware Configuration]");
    Serial.print("LED Count:        "); Serial.println(NUM_LEDS);
    Serial.print("Strips:           "); Serial.println(NUM_STRIPS);
    Serial.print("LEDs per Strip:   "); Serial.println(LEDS_PER_STRIP);
    Serial.print("Data Pin:         GPIO "); Serial.println(LED_DATA_PIN);
    Serial.print("LED Type:         WS2812B");
    Serial.print(" ("); Serial.print(COLOR_ORDER == GRB ? "GRB" : "RGB"); Serial.println(")");
    Serial.print("Brightness:       "); Serial.print(BRIGHTNESS);
    Serial.print("/"); Serial.print(MAX_BRIGHTNESS);
    Serial.print(" ("); Serial.print((BRIGHTNESS * 100) / MAX_BRIGHTNESS); Serial.println("%)");

    // Initialize FastLED
    Serial.println("\n[Initializing FastLED]");
    FastLED.addLeds<LED_TYPE, LED_DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.setMaxRefreshRate(120);  // Limit refresh rate

    // Clear any garbage data from LED buffer
    FastLED.clear(true);  // true = immediate show
    delay(50);  // Give RMT peripheral time to stabilize

    Serial.println("✓ FastLED initialized and buffer cleared");

    // Test all LEDs white
    Serial.println("\n[LED Test]");
    Serial.println("Testing all LEDs white for 1 second...");
    setAllColor(CRGB::White);
    FastLED.show();
    delay(1000);

    // Turn off
    setAllColor(CRGB::Black);
    FastLED.show();
    Serial.println("✓ LED test complete");

    Serial.println("\n[Pattern Test Starting]");
    Serial.print("Pattern duration: "); Serial.print(patternDuration / 1000);
    Serial.println(" seconds each");
    Serial.println("Patterns will cycle automatically...\n");

    printPatternInfo();
    lastPatternChange = millis();
}

void loop() {
    // Update LED pattern animation
    updatePattern();
    delay(33);  // ~30 FPS

    // Change pattern every patternDuration
    if (millis() - lastPatternChange >= patternDuration) {
        currentPattern = (currentPattern + 1) % totalPatterns;
        animationStep = 0;
        lastPatternChange = millis();
        printPatternInfo();
    }

    // Memory monitoring every 5 seconds
    static unsigned long lastMemCheck = 0;
    if (millis() - lastMemCheck >= 5000) {
        Serial.print("[Memory] Free heap: ");
        Serial.print(ESP.getFreeHeap() / 1024.0, 1);
        Serial.print(" KB | Pattern: ");
        Serial.print(currentPattern + 1);
        Serial.print("/");
        Serial.println(totalPatterns);
        lastMemCheck = millis();
    }
}
