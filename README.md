# 2048GameCYDVibeCodingDemo

Key Features

    Responsive Touch Controls: Smooth swipe gestures powered by the XPT2046 touch controller with customizable noise filtering and deadzones.

    Vibe Power-Ups:

        ↩️ Undo: Revert your last move.

        💣 Bomb: Tap to destroy any unwanted tile on the grid.

        ⚡ 2x Multiplier: Instantly double the value of your highest active tile.

    Hardware Integration:

        Reactive RGB LED: Dynamic color pulsing based on active board combos and tile values.

        Piezo Audio SFX: Real-time audio chimes for swipes, tile merges, power-up usage, and game over sequences.

    Flicker-Free Performance: Off-screen sprite rendering via LovyanGFX for smooth frame updates.

    Persistent High Score: Uses ESP32 Preferences.h to store your top score in non-volatile flash memory across reboots.

Hardware Stack

    Microcontroller: ESP32-D0WDQ6 (ESP32-2432S028R)

    Display: 2.8" SPI TFT (240x320 resolution)

    Libraries: LovyanGFX, Preferences
