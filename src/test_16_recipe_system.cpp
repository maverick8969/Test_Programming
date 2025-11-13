/**
 * Test 16: Recipe/Formula Execution System
 *
 * Hardware:
 * - BTT Rodent V1.1 board running FluidNC (UART mode)
 * - ESP32 Dev Module
 * - 4 peristaltic pumps
 * - LCD display for status
 * - Direct UART connection (GPIO 16/17)
 *
 * Purpose:
 * - Test complex recipe execution
 * - Store and recall multiple formulas
 * - Execute multi-step mixing procedures
 * - Provide user feedback during execution
 *
 * Build command:
 *   pio run -e test_16_recipe_system -t upload -t monitor
 */

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "pin_definitions.h"

#define UartSerial         Serial2

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 16, 2);

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

Recipe recipes[] = {
    {"Cleaning Flush", cleaningRecipe, 4},
    {"Color Mix", colorMixRecipe, 3},
    {"Nutrient Mix", nutrientMixRecipe, 4}
};
const int recipeCount = 3;

int currentRecipe = -1;
int currentStep = 0;
bool executing = false;
bool waitingForCompletion = false;

const float ML_PER_MM = 0.05;

void sendCommand(const char* cmd) {
    Serial.print("→ ");
    Serial.println(cmd);
    UartSerial.println(cmd);
    UartSerial.flush();
}

void updateLCD(const char* line1, const char* line2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
}

void executeRecipeStep(Recipe& recipe, int step) {
    if (step >= recipe.stepCount) {
        Serial.println("\n✓ Recipe complete!");
        updateLCD("Recipe Complete!", recipe.name);
        executing = false;
        return;
    }

    Ingredient ing = recipe.ingredients[step];
    float distMm = ing.volumeMl / ML_PER_MM;
    float feedRate = ing.flowRateMlMin / ML_PER_MM;

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

    // Update LCD
    char lcdLine1[17], lcdLine2[17];
    snprintf(lcdLine1, sizeof(lcdLine1), "%s %d/%d", recipe.name, step + 1, recipe.stepCount);
    snprintf(lcdLine2, sizeof(lcdLine2), "Pump %c: %.1fml", ing.pump, ing.volumeMl);
    updateLCD(lcdLine1, lcdLine2);

    // Reset position
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "G92 %c0", ing.pump);
    sendCommand(cmd);
    delay(100);

    // Execute
    snprintf(cmd, sizeof(cmd), "G1 %c%.2f F%.1f", ing.pump, distMm, feedRate);
    sendCommand(cmd);

    waitingForCompletion = true;
}

void startRecipe(int recipeIndex) {
    if (recipeIndex < 0 || recipeIndex >= recipeCount) {
        Serial.println("Invalid recipe index");
        return;
    }

    currentRecipe = recipeIndex;
    currentStep = 0;
    executing = true;

    Serial.println("\nStarting recipe: " + String(recipes[recipeIndex].name));
    updateLCD("Starting:", recipes[recipeIndex].name);
    delay(1000);

    executeRecipeStep(recipes[currentRecipe], currentStep);
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║         Test 16: Recipe/Formula Execution System          ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");

    // Initialize LCD
    Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN);
    lcd.init();
    lcd.backlight();
    updateLCD("Recipe System", "Ready");
    Serial.println("✓ LCD initialized");

    // Initialize UART
    UartSerial.begin(115200, SERIAL_8N1, UART_TEST_RX_PIN, UART_TEST_TX_PIN);
    Serial.println("✓ UART initialized\n");

    Serial.println("Available Recipes:");
    for (int i = 0; i < recipeCount; i++) {
        Serial.print("  ");
        Serial.print(i + 1);
        Serial.print(" - ");
        Serial.print(recipes[i].name);
        Serial.print(" (");
        Serial.print(recipes[i].stepCount);
        Serial.println(" steps)");
    }

    Serial.println("\nCommands:");
    Serial.println("  1-3 - Execute recipe");
    Serial.println("  s - Status\n");
}

void loop() {
    // Handle user commands
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        int recipeNum = input.toInt();
        if (recipeNum >= 1 && recipeNum <= recipeCount) {
            startRecipe(recipeNum - 1);
        } else if (input == "s") {
            sendCommand("?");
        }
    }

    // Process responses
    if (UartSerial.available()) {
        String response = UartSerial.readStringUntil('\n');
        response.trim();
        Serial.print("← ");
        Serial.println(response);

        // Check completion
        if (waitingForCompletion && response.indexOf("Idle") >= 0) {
            waitingForCompletion = false;
            currentStep++;
            delay(500);
            if (executing) {
                executeRecipeStep(recipes[currentRecipe], currentStep);
            }
        }
    }

    delay(100);
}
