# MCAI Arcade

A native, offline-first arcade for the **M5Stack Cardputer ADV**.

## Build

Install PlatformIO, then run:

```sh
pio run
pio run -t upload
```

The project targets the Cardputer ADV using the M5Cardputer Arduino library. It is a standalone firmware build; it does not replace MCAI-OS.

## Controls

- Keyboard letters/numbers: navigate and play
- WASD / IJKL / arrow keys: movement where supported
- Enter: select / confirm
- Backspace or Esc: pause / return
- R: restart the current round

## Game library

The arcade is structured around ten playable game engines and ten challenge variants per engine (100 selectable challenges). Variants change goals, board layouts, speed, timing and difficulty; they are not represented as 100 unrelated game engines.

All games run locally. No Wi-Fi, account, ads or downloads are required.

## Status

This is an initial hardware-targeted implementation. Build it with PlatformIO and test on the actual Cardputer ADV before treating it as a release.