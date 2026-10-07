# 2048 Game CYD Vibe Coding Demo

PlatformIO firmware for the ESP32-2432S028R Cheap Yellow Display (CYD).

## Hardware

- ESP32-2432S028R with 240x320 ILI9341 TFT
- ILI9341 TFT SPI (SCK GPIO 14, MOSI GPIO 13, MISO GPIO 12, CS GPIO 15, DC GPIO 2, backlight GPIO 21)
- XPT2046 touch controller using the CYD example's bit-banged wiring (SCK GPIO 25, MOSI GPIO 32, MISO GPIO 39, CS GPIO 33, IRQ GPIO 36)
- RGB LED: red GPIO 4, green GPIO 16, blue GPIO 17 (active-low)
- Buzzer: GPIO 26

The display uses the same TFT_eSPI library, `ILI9341_2` driver, SPI pin mapping, RGB order, and inversion settings as the known-working CYD example in `R6-drone/CYD`. Touch uses a local copy of that example's bit-banged XPT2046 driver, avoiding conflicts with the TFT SPI bus. The default `esp32dev` target is intentional: CYD boards are commonly sold with different module/flash labels even though the CYD uses an ESP32-WROOM-compatible Arduino target.

## Build and upload

1. Install PlatformIO in VS Code.
2. Open this folder as the PlatformIO project.
3. Update `upload_port` in `platformio.ini` if PlatformIO does not detect the CYD automatically.
4. Run **PlatformIO: Build**, then **PlatformIO: Upload**.
5. Open a 115200 baud serial monitor.

Always do a clean build after changing the display driver:

```text
PlatformIO: Clean
PlatformIO: Build
PlatformIO: Upload
```

## Display setup

The display uses `TFT_eSPI`, because the working CYD example uses the same library and configuration. The important settings are in [`include/User_Setup.h`](include/User_Setup.h) and are also passed as PlatformIO build flags so they are visible while TFT_eSPI itself is compiled:

```cpp
#define ILI9341_2_DRIVER
#define TFT_RGB_ORDER TFT_RGB
#define TFT_WIDTH 240
#define TFT_HEIGHT 320

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST -1
#define TFT_BL 21
#define TFT_INVERSION_ON
```

The `ILI9341_2` driver, RGB order, and inversion setting are important for this particular CYD panel. Using the wrong SPI pins or driver can produce a white screen, black screen, or noisy/garbled pixels.

The display is initialized in `setup()` before the game is drawn:

```cpp
display.init();
display.invertDisplay(true);
display.setRotation(0);
digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
```

Rotation `0` keeps the display in portrait mode, matching the 240x320 game layout.

## Touchscreen setup

The CYD's XPT2046 touch controller is not read through TFT_eSPI. It uses the local [`XPT2046_Bitbang_Slim`](lib/XPT2046_Bitbang_Slim) driver, copied from the known-working CYD example. This avoids sharing the TFT SPI bus and allows the touch pins used by this board:

```cpp
XPT2046_Bitbang touchscreen(32, 39, 25, 33, 240, 320);
```

The arguments are:

```text
MOSI = GPIO 32
MISO = GPIO 39
CLK  = GPIO 25
CS   = GPIO 33
```

The driver reads the XPT2046 pressure first. A pressure value below 100 is treated as no touch. For a valid press it reads raw X and Y values, then maps them into screen coordinates:

```cpp
TouchPoint point = touchscreen.getTouch();
if (point.zRaw != 0) {
  // point.x and point.y are screen coordinates
}
```

This panel reports both axes reversed relative to the portrait display, so the driver corrects them with:

```cpp
x = width - 1 - map(rawX, 0, 4095, 0, width - 1);
y = height - 1 - map(rawY, 0, 4095, 0, height - 1);
```

The expected screen coordinate system is:

```text
TOP OF SCREEN
(0,0) ------------------------------ (239,0)
  |              2048                 |
  |              GAME                 |
  |                                   |
(0,319) -------------------------- (239,319)
BOTTOM OF SCREEN
```

Touch diagnostics are printed to the serial monitor:

```text
[TOUCH DOWN] mapped=(120,160) raw=(2048,2100) pressure=900
[TOUCH MOVE] mapped=(125,160) raw=(2060,2100) pressure=950
[TOUCH UP] mapped=(125,160) duration=240ms action=tap
```

These messages distinguish hardware/calibration problems from game-input problems. If only `[TOUCH IDLE]` appears, no pressure is being detected. If raw values change but mapped positions are wrong, adjust the mapping in [`XPT2046_Bitbang.cpp`](lib/XPT2046_Bitbang_Slim/XPT2046_Bitbang.cpp).

## Tap and swipe handling

The main loop samples touch approximately every 40 ms. It remembers the first point, tracks movement, and waits for release:

```cpp
if (movement >= 28 pixels) {
  // swipe
} else {
  // tap
}
```

This prevents a held finger from repeatedly activating a power-up. On release, taps are sent to `tap()` and gestures are sent to `swipe()`.

The touch controller's physical axes are rotated relative to the game board. The gesture layer compensates for that rotation:

```text
physical right -> game right
physical left  -> game left
physical down  -> game down
physical up    -> game up
```

The compensation is separate from coordinate mapping. Coordinate mapping makes taps land at the correct screen location; gesture compensation makes swipes move tiles in the expected direction.

## Game logic

The board and rules are isolated in [`game_logic.cpp`](src/game_logic.cpp) and [`game_logic.h`](include/game_logic.h). The game stores a 4x4 array of `uint16_t` tile values:

```cpp
using Board = uint16_t[4][4];
```

At reset, two tiles are placed. A valid move:

1. Reads each row or column.
2. Removes empty cells.
3. Merges adjacent equal values once.
4. Moves the remaining values toward the selected edge.
5. Adds the merge score.
6. Stores the previous board for undo.
7. Adds a new `2` or occasional `4`.

`MoveResult` reports whether the board changed, how many merges happened, the largest merge value, and the score delta. The UI uses that result for sound, LED feedback, combo scoring, and redraws.

## Current game features

- 4x4 2048 board
- Swipe movement in all four directions
- Score and persistent best score using ESP32 `Preferences`
- Three undo uses per run
- Two bomb uses per run
- One highest-tile multiplier per run
- Combo multiplier up to 4x
- Tile numbers rendered in each occupied square
- Dynamic background color based on the highest tile
- Buzzer feedback for moves, merges, and power-ups
- Active-low RGB LED feedback
- `YOU WIN!` overlay when 2048 is reached
- `GAME OVER` overlay when no moves remain
- Tap-to-restart after win or game over

## Current controls

- Swipe on the grid to move tiles.
- After `YOU WIN!` or `GAME OVER`, tap the screen to start a new game.

Power-up controls are intentionally hidden and disabled while the core board, tile rendering, and win/loss flow are being validated.

## What is shown on the screen

The top row contains three power-up buttons:

```text
UNDO       Reverts the previous valid move. Starts with 3 uses.
BOMB       Activates target mode, then removes one tapped tile. Starts with 2 uses.
2X UP      Doubles the highest tile on the board. Starts with 1 use.
```

Each button displays its remaining uses. The BOMB button turns red while target mode is active. After activating BOMB, tap a non-empty tile in the grid.

The bottom `COMBO` label and bar show the current consecutive-merge multiplier. A merge within two seconds of the previous merge increases the multiplier up to `X4`. The cyan bar is a visual activity meter based on the current score; it is not a second score or a health bar.

Each colored grid square contains its numeric tile value (`2`, `4`, `8`, and so on). Empty squares intentionally have no number.

## Controls

- Swipe on the grid to move tiles.
- Tap **UNDO**, **BOMB**, or **2x UP** in the top bar.
- When BOMB is active, tap a non-empty tile.
