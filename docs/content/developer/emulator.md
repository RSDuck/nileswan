---
title: 'Device emulator'
weight: 5
---

In addition to physical hardware, a device emulator can be used for testing nileswan software. The current device
emulator is a fork of the [Mednafen](https://mednafen.github.io/) emulator; however, it is maintained by us, so
please do not ask Mednafen's developers questions about it.

The main advantage of the device emulator is that it allows much more rapid test cycles on a development computer. In addition,
stepping and breakpoint functionality is provided for debugging. The main disadvantage is that the emulator only implements
a limited subset of both the console and the cartridge's functionality.

## Installation

1. [Download](https://github.com/49bitcat/nileswan-medem/releases) or [compile](https://github.com/49bitcat/nileswan-medem) the device emulator.
2. Prepare the emulator files. This can be done by either:
    - [downloading]() a ready-made build of the package,
    - [compiling]() the package from source by using the `make dist-emu` command; in this case, the package will be placed in the `out/emulator` directory.
3. Place the emulator files in a directory of your choosing. You should have the following files:
    - `nileswan.ipl0` - a build of the IPL0 (first-stage loader),
    - `nileswan.spi` - a replica of the SPI flash image,
    - `nileswan.img` - a pre-formatted FAT32 image, emulating the removable storage card.
4. Use a tool of choice (for example, `mount -o loop nileswan.img ...` on Linux) to mount the pre-formatted FAT32 image and edit its contents.
5. Run the emulator: `nileswan-medem nileswan.ipl0`.

## Supported features

| Feature | Supported? | Notes |
|---------|------------|-------|
| SPI interface | Yes | |
| Power control | Yes | |
| IPC area | Yes | |
| SPI removable storage | Partial | Some commands only, <= 2 GiB only |
| SPI flash | Partial | Some commands only |
| MCU bootloader | Stubbed | MCU flash not stored |
| MCU native | Partial | Some commands only |
| MCU EEPROM | No | |
| MCU RTC | No | |
| MCU CDC transfer | Yes | Use `wswan.excomm` config option |
