# 2048 Game CYD: Technical Reference

This document describes the implementation details of the CYD 2048 game. It is intended as an example for configuring the display, reading the touchscreen, processing gestures, rendering the board, and connecting game logic to ESP32 hardware.

## 1. PlatformIO configuration

The project uses the `cyd` environment in [`platformio.ini`](platformio.ini):

```ini
[env:cyd]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    bodmer/TFT_eSPI@^2.5.43
```

`esp32dev` is intentional. The CYD uses an ESP32-WROOM-compatible Arduino target even though boards are sold under different CYD module names.

The `native` environment builds only the platform-independent game logic:

```ini
[env:native]
platform = native
test_framework = unity
build_src_filter = -<*> +<game_logic.cpp>
```

## 2. CYD display wiring and driver

The known-working CYD display configuration is:

| Signal | GPIO |
|---|---:|
| TFT SCK | 14 |
| TFT MOSI | 13 |
| TFT MISO | 12 |
| TFT CS | 15 |
| TFT DC | 2 |
| TFT reset | -1 |
| TFT backlight | 21 |

The setup is in [`include/User_Setup.h`](include/User_Setup.h), and the same values are supplied as build flags so TFT_eSPI sees them while compiling its own source files:

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

`ILI9341_2_DRIVER`, RGB order, and inversion are specific to this panel configuration. Incorrect settings can produce a white screen, black screen, or noisy pixels.

The display is initialized before the first frame:

```cpp
display.init();
display.invertDisplay(true);
display.setRotation(0);
pinMode(TFT_BL, OUTPUT);
digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
```

Rotation `0` is portrait mode, so the logical display is 240 pixels wide by 320 pixels high.

## 3. CYD touchscreen implementation

The XPT2046 controller is read with the local bit-banged driver in [`lib/XPT2046_Bitbang_Slim`](lib/XPT2046_Bitbang_Slim), based on the working CYD example. It deliberately does not share the TFT SPI bus.

| Signal | GPIO |
|---|---:|
| Touch MOSI | 32 |
| Touch MISO | 39 |
| Touch clock | 25 |
| Touch CS | 33 |
| Touch IRQ | 36 |

The current implementation polls the controller, so the IRQ pin is documented hardware information but is not required by the polling code.

The driver is constructed for the portrait display:

```cpp
XPT2046_Bitbang touchscreen(32, 39, 25, 33, 240, 320);
```

### Touch protocol

The XPT2046 commands are:

```cpp
READ_X  = 0x91
READ_Y  = 0xD1
READ_Z1 = 0xB1
READ_Z2 = 0xC1
```

The driver:

1. Pulls CS low.
2. Reads `Z1` and `Z2`.
3. Calculates pressure as `Z1 + 4095 - Z2`.
4. Treats pressure below `100` as no touch.
5. Reads raw X and Y.
6. Releases CS.
7. Maps raw 12-bit values to the 240x320 screen.

This panel reports both axes reversed compared with the portrait display, so the final mapping is:

```cpp
x = width - 1 - map(rawX, 0, 4095, 0, width - 1);
y = height - 1 - map(rawY, 0, 4095, 0, height - 1);
```

The coordinate system is:

```text
(0,0)  TOP-LEFT -------------------- (239,0)
  |                                      |
  |              GAME BOARD              |
  |                                      |
(0,319) BOTTOM-LEFT --------------- (239,319)
```

## 4. Touch diagnostics

The main loop samples the touchscreen approximately every 40 ms and logs state changes:

```text
[TOUCH IDLE] no touch detected
[TOUCH DOWN] mapped=(120,160) raw=(2048,2100) pressure=900
[TOUCH MOVE] mapped=(125,160) raw=(2060,2100) pressure=950
[TOUCH UP] mapped=(125,160) duration=240ms action=tap
```

Interpretation:

- Only `TOUCH IDLE`: no pressure is being detected; check CS, MISO, power, or wiring.
- Raw values change but mapped values are wrong: adjust the mapping in `XPT2046_Bitbang.cpp`.
- Correct mapped coordinates but wrong swipe direction: adjust gesture rotation in `swipe()`.
- `TOUCH DOWN` followed immediately by `TOUCH UP`: the press is being detected but may be too short or noisy.

## 5. Tap and swipe recognition

The first valid touch becomes the gesture start point. While pressed, the code records the latest position. On release:

- Movement of at least 28 pixels is a swipe.
- Movement below 28 pixels is a tap.

This release-based design prevents a held finger from activating an action repeatedly.

The touch panel's physical axes required a gesture rotation. The game layer compensates so the user-facing behavior is:

```text
physical right -> move tiles right
physical left  -> move tiles left
physical down  -> move tiles down
physical up    -> move tiles up
```

Coordinate mapping and gesture mapping are separate:

- Coordinate mapping controls where taps land.
- Gesture mapping controls which direction tiles move.

## 6. Board layout and rendering

The logical layout is portrait 240x320:

```text
Header:       y 0..47
Hint:         y 48..77
Grid:         x 12..216, y 85..288
Footer:       y 300..318
```

The grid uses four 46-pixel cells with 5-pixel gaps:

```cpp
GRID_X = 12
GRID_Y = 85
CELL = 46
GAP = 5
```

The board is redrawn after every valid action. The background changes based on the highest tile:

- Below `128`: dark slate
- `128` through `512`: warm tint
- `1024` and above: violet tint

### Custom tile numbers

Tile digits are not rendered with TFT_eSPI fonts. They use a local 5x7 bitmap table in `main.cpp`. Each set bit becomes a filled rectangle:

```cpp
void drawDigit(uint8_t digit, int x, int y,
               uint16_t ink, uint16_t paper, uint8_t scale);
```

`drawValue()` converts a tile value to text, centers the digits, and draws them with scale `3`. This makes tile values independent of font configuration and avoids the earlier blank-number problem.

## 7. Tile color palette

`tileColor()` maps each standard tile to a distinct RGB color:

| Value | Color |
|---:|---|
| 0 | dark gray |
| 2 | cream |
| 4 | pale gold |
| 8 | orange |
| 16 | orange-red |
| 32 | coral |
| 64 | red |
| 128 | gold |
| 256 | bright gold |
| 512 | amber |
| 1024 | violet |
| 2048 | deep violet |

Values above 2048 use a dark violet fallback.

## 8. Game logic

The board API is declared in [`include/game_logic.h`](include/game_logic.h) and implemented in [`src/game_logic.cpp`](src/game_logic.cpp).

```cpp
using Board = uint16_t[4][4];
enum class Direction : uint8_t { Up, Right, Down, Left };
```

At reset:

- The board is cleared.
- Score is reset.
- Three power-up counters are reset.
- Two random tiles are placed.

For each valid move, the game:

1. Copies the current board for undo.
2. Extracts each row or column.
3. Removes empty cells.
4. Merges adjacent equal values once.
5. Writes the compacted line back.
6. Adds the merge score.
7. Adds a random `2` or occasional `4`.

`MoveResult` contains:

- `changed`
- `mergeValue`
- `mergeCount`
- `scoreDelta`

`canMove()` returns true if there is an empty cell or an adjacent equal pair.

## 9. Current game state and end conditions

`gameFinished` is maintained by the UI:

- It becomes true when the highest tile reaches `2048`.
- It becomes true after a valid move leaves no legal move.
- Swipes are ignored while finished.
- A tap while finished resets the board.

The end screen paints the full display dark, draws a large border, and displays either `YOU WIN!` or `GAME OVER` plus `TAP TO RESTART`.

## 10. ESP32 feedback hardware

The RGB LED is active-low:

| Channel | GPIO |
|---|---:|
| Red | 4 |
| Green | 16 |
| Blue | 17 |

The buzzer is on GPIO 26. The current build uses short tones for movement, merges, and power-up code paths. Power-up controls are hidden while the core game is being validated.

## 11. Build and upload

Use a clean build after changing TFT_eSPI configuration:

```text
PlatformIO: Clean
PlatformIO: Build
PlatformIO: Upload
```

Or from a terminal:

```powershell
& 'C:\Users\marni\.platformio\penv\Scripts\platformio.exe' run -e cyd -t clean
& 'C:\Users\marni\.platformio\penv\Scripts\platformio.exe' run -e cyd
```

The generated firmware is:

```text
.pio/build/cyd/firmware.bin
```

## 12. Validation

The CYD environment has been successfully compiled with PlatformIO. The native test environment requires a host GCC/G++ toolchain. If GCC/G++ is installed, run:

```text
pio test -e native
```

The hardware-specific display and touch behavior must ultimately be verified by uploading to the physical CYD and checking the serial diagnostics.
