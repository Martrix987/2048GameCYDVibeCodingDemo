# 2048 Game CYD Vibe Coding Demo

PlatformIO firmware for the ESP32-2432S028R Cheap Yellow Display (CYD).

## Hardware

- ESP32-2432S028R with 240x320 ILI9341 TFT
- ILI9341 TFT SPI (SCK GPIO 14, MOSI GPIO 13, MISO GPIO 12, CS GPIO 15, DC GPIO 2, backlight GPIO 21)
- XPT2046 touch controller using the CYD example's separate pin wiring (SCK GPIO 25, MOSI GPIO 32, MISO GPIO 39, CS GPIO 33, IRQ GPIO 36)
- RGB LED: red GPIO 4, green GPIO 16, blue GPIO 17 (active-low)
- Buzzer: GPIO 26

The display uses the same TFT_eSPI library, `ILI9341_2` driver, SPI pin mapping, RGB order, and inversion settings as the known-working CYD example in `R6-drone/CYD`. Touch uses the example's bit-banged pin wiring, avoiding conflicts with the TFT SPI bus. The default `esp32dev` target is intentional: CYD boards are commonly sold with different module/flash labels even though the CYD uses an ESP32-WROOM-compatible Arduino target.

## Build and upload

1. Install PlatformIO in VS Code.
2. Open this folder as the PlatformIO project.
3. Update `upload_port` in `platformio.ini` if PlatformIO does not detect the CYD automatically.
4. Run **PlatformIO: Build**, then **PlatformIO: Upload**.
5. Open a 115200 baud serial monitor.

If touch coordinates are mirrored or offset, adjust `x_min`, `x_max`, `y_min`, and `y_max` in `src/main.cpp` for the individual panel.

## Controls

- Swipe on the grid to move tiles.
- Tap **UNDO**, **BOMB**, or **2x UP** in the top bar.
- When BOMB is active, tap a non-empty tile.
