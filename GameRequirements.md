# Requirements Spec: 2048 "Vibe Edition" (CYD / ESP32-2432S028R)

Target Hardware: ESP32-2432S028R (Cheap Yellow Display / CYD)
Display: 2.8" TFT (240x320 portrait mode)
Peripherals: XPT2046 Touch, Onboard RGB LED (Red: GPIO 4, Green: GPIO 16, Blue: GPIO 17), Onboard Buzzer (GPIO 26)
Library: TFT_eSPI (configured with the proven CYD `ILI9341_2` setup)

## 1. Screen & UI Layout Adjustments (240 x 320 px)
- Power-up Bar (Y: 55px to 80px):
  - 3 Touchable Power-up Icon Boxes (40x22px each):
    - [ UNDO ] (Uses left: 3 per game)
    - [ BOMB ] (Uses left: 2 per game)
    - [ 2x UP ] (Uses left: 1 per game)
- Game Grid Region (Shifted down to Y: 85px to 305px):
  - 4x4 Grid centered horizontally (215x215px box).
  - Cell Dimensions: 46x46px per tile with 5px spacing.
- Footer / Combo Meter (Y: 310px to 320px):
  - Animated progress bar filling up as moves are made without stalling.

## 2. Power-up Mechanics
- UNDO (Revert Move):
  - Stores `previous_grid[4][4]` and `previous_score` before each valid turn.
  - Tapping [ UNDO ] restores the previous state (max 3 uses per run).
- BOMB (Tile Eraser):
  - Tapping [ BOMB ] activates "Target Mode".
  - The next tile tapped on the grid is destroyed immediately.
  - Spawns a mini red particle explosion on the sprite buffer.
- 2x MULTIPLIER (Tile Doubler):
  - Tapping [ 2x UP ] doubles the value of the highest tile currently on the grid (e.g., turns a 256 into a 512).

## 3. Combo & High-Vibe Mechanics
- Combo System:
  - Performing consecutive merges within 2 seconds increments a `combo_multiplier` (x2, x3, x4).
  - Score bonus = `merged_value * combo_multiplier`.
  - Floating combo text appears temporarily over the merged cell ("2x COMBO!").
- Screen Shake & FX:
  - Merging 128+ tiles triggers a subtle 3-frame horizontal screen jitter on the display buffer.
  - Floating "+Value" numbers fade upward on merge.

## 4. Onboard Hardware Integration (CYD Hardware)
- RGB LED Sync (GPIO 4, 16, 17 - active LOW):
  - Standard move: Subtle white pulse.
  - High merge (128–512): Bright Cyan / Gold flash.
  - Mega merge (1024–2048): Rainbow strobe for 300ms.
  - Game Over: Solid Red fading to dim.
- Buzzer Audio Feedback (GPIO 26 PWM / tone):
  - Swipe move: Short low click (150 Hz, 10ms).
  - Tile merge: Rising pitch chime (e.g., 440 Hz -> 880 Hz).
  - Power-up used: Sci-fi sweep tone.
  - Game Over: Descending 3-note melody.

## 5. Dynamic Background Theme
- Ambient Color Shifting:
  - Background slowly shifts color temperature depending on the highest tile on board:
    - 2 to 64: Chill Dark Slate (#1e1e24)
    - 128 to 512: Warm Sunset / Amber tint (#2a1a1f)
    - 1024+: Cyberpunk Neon Violet tint (#1f0e2b)