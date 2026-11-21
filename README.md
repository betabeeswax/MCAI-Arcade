# MCAI Arcade

An offline-first native arcade project for the **M5Stack Cardputer ADV**.

## Build

Install PlatformIO, then run:

```sh
pio run
pio run -t upload
```

The PlatformIO target is `m5stack-cardputer` and uses the M5Cardputer Arduino library. The GitHub Actions workflow builds the firmware on pushes and uploads the resulting `firmware.bin` as an artifact.

## Controls

- W/S or arrow keys: navigate the game list
- A/D: move by seven entries in the list
- Enter: launch / confirm
- WASD or arrows: game-specific movement
- Backspace or Esc: return to the game list
- R: restart the current round
- Number keys: enter answers in number/memory challenges

## Game library

The first implementation includes **10 playable game engines with 10 difficulty/challenge levels each (100 selectable challenges)**:

1. Snake
2. Pong
3. Breakout
4. Dodge
5. Reaction Test
6. Math Rush
7. Memory Sequence
8. Number Hunt
9. Tic-Tac-Toe against a CPU opponent
10. Target Tap

The 100 entries are challenge variants, not 100 unrelated game engines. Each mode adjusts difficulty and/or round goals. High scores are stored in ESP32 Preferences and survive restarts.

## Hardware / release status

This is an initial Cardputer ADV-targeted implementation. The source and automated build workflow are committed, but the firmware still needs a successful CI build and hands-on testing on the actual Cardputer ADV before it should be called a tested release. It is standalone firmware and does not replace or modify MCAI-OS.

No Wi-Fi, account, ads, or downloads are needed while playing.
