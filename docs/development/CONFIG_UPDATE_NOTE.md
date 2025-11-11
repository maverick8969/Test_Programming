# Config.h Update for LED Animation System

## REQUIRED CHANGE TO config.h

The LED animation system adds two new states to the LEDAnimState enum. Update your config.h as follows:

### CURRENT (lines 223-231):
```cpp
// LED animation states
typedef enum {
    LED_ANIM_IDLE,
    LED_ANIM_RECIPE_SELECT,
    LED_ANIM_DOSING,
    LED_ANIM_COMPLETE,
    LED_ANIM_ERROR,
    LED_ANIM_PAUSED
} LEDAnimState;
```

### UPDATE TO:
```cpp
// LED animation states
typedef enum {
    LED_ANIM_BOOT,          // NEW: Boot/startup animation
    LED_ANIM_IDLE,
    LED_ANIM_RECIPE_SELECT,
    LED_ANIM_PREPARING,     // NEW: Pre-check scanning animation
    LED_ANIM_PRIMING,       // NEW: Pump priming flash
    LED_ANIM_DOSING,
    LED_ANIM_PAUSED,
    LED_ANIM_COMPLETE,
    LED_ANIM_ERROR
} LEDAnimState;
```

### REASONING:

1. **LED_ANIM_BOOT** - Added for the startup sequence (smooth activation of all LEDs)
2. **LED_ANIM_PREPARING** - Added for the pre-check phase (scanning animation)
3. **LED_ANIM_PRIMING** - Added for pump priming (flash specific pump)

These states map to your system states:
- `STATE_INIT` → `LED_ANIM_BOOT`
- `STATE_DOSING_PRE_CHECK` → `LED_ANIM_PREPARING`
- `STATE_DOSING_PRIMING` → `LED_ANIM_PRIMING`

### ALTERNATIVE APPROACH:

Instead of modifying config.h, you could use the enum defined in led.h, which is a superset of the original. The led.h enum includes all states and can be used directly. However, for consistency, it's better to update config.h to match.

---

**Note:** This is the ONLY change needed to config.h for LED integration.
