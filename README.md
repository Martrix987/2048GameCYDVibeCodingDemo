# 2048 Game CYD Vibe Coding Demo

An Arduino/PlatformIO 2048 game for the ESP32-2432S028R Cheap Yellow Display (CYD).

The project is currently focused on making the core game reliable on the real hardware:

- 4x4 2048 board
- CYD ILI9341 display
- CYD XPT2046 touchscreen
- Swipe controls in all four directions
- Visible tile values with custom bitmap digits
- Tile colors based on their values
- Score and persistent best score
- Win and game-over screens
- RGB LED and buzzer feedback
- Serial touchscreen diagnostics

## Hardware

- ESP32-2432S028R CYD
- 240x320 ILI9341 TFT
- XPT2046 resistive touchscreen
- RGB LED
- Buzzer

The exact wiring, display driver setup, touch implementation, coordinate mapping, rendering details, and troubleshooting steps are documented in [`TECHNICAL_README.md`](TECHNICAL_README.md).

## Quick start

1. Install PlatformIO in VS Code.
2. Open this folder as a PlatformIO project.
3. Connect the CYD over USB.
4. Run **PlatformIO: Clean**.
5. Run **PlatformIO: Build**.
6. Run **PlatformIO: Upload**.
7. Open a serial monitor at `115200` baud.

If PlatformIO does not find the board automatically, set `upload_port` in [`platformio.ini`](platformio.ini).

## How to play

- Swipe on the board to move the tiles.
- Matching adjacent tiles merge into their doubled value.
- Each valid move adds a new tile.
- Reach `2048` to win.
- If no empty cell or legal merge remains, the game is over.
- Tap the win or game-over screen to restart.

Power-up controls are intentionally disabled in the current core-game build while board rendering and game-state behavior are being validated.

## On-screen layout

- Header: title, score, and best score.
- Center: 4x4 tile grid.
- Bottom: combo indicator and score activity bar.
- End state: large `YOU WIN!` or `GAME OVER` screen with restart instructions.

Tile colors change as values increase, from cream (`2`) through orange/red, gold, and violet (`1024`/`2048`). Empty cells are dark gray.

## Project structure

```text
src/main.cpp                         Hardware integration and UI
src/game_logic.cpp                   Board rules and scoring
include/game_logic.h                 Game API and board types
include/User_Setup.h                 TFT_eSPI display setup
lib/XPT2046_Bitbang_Slim/            CYD touchscreen driver
test/game_logic_test.cpp             Native game-logic test
platformio.ini                       Build environments and dependencies
TECHNICAL_README.md                  Detailed implementation reference
```

## Troubleshooting

If the display is noisy or blank, perform a clean build and upload. If touch is not responding, open the serial monitor and look for `[TOUCH DOWN]`, `[TOUCH MOVE]`, and `[TOUCH UP]` messages. The full diagnostic interpretation guide is in [`TECHNICAL_README.md`](TECHNICAL_README.md).
