---
title: 'Boot procedure'
weight: 30
---

nileswan uses a two-stage boot loader.

{{< mermaid class="text-center" >}}
flowchart LR
    C[Console IPL] -->|FPGA core| A0[IPL0]
    A0 -->|SPI flash| A1[IPL1 /boot/]
    A0 -->|SPI flash - X3 + B held| A3[IPL1 /recovery/]
    A3 -->|SPI flash| A4[Recovery]
    A1 -->|TF card| A2[MENU.WS]
{{< /mermaid >}}

## IPL0

The first-stage boot loader is 512 bytes in size. It is stored in the factory FPGA core.

Its goal is to:

- reset the nileswan hardware,
- store the console register and I/O port state in console internal RAM,
- load IPL1 from SPI flash to console internal RAM.

## IPL1

The second-stage boot loader is up to `0x37C0` (14272) bytes in size. It is stored in the SPI flash. It uses a special 16-byte header:

|Offset|Size| Description |
|------|----|-------------|
| 0x0  | 2  | Load address (typically 0x0040) |
| 0x2  | 2  | Number of 512-byte sectors to load from SPI flash |
| 0x4  | 12 | Reserved |

It has two variants:

### IPL1 (boot)

The "boot" variant is bundled with the storage card driver. Its goal is to:

- validate the IPC memory area and populate it if necessary,
- switch to the update-provided FPGA core (unless the factory version of IPL1 is forced by holding the on-cartridge button),
- load `/NILESWAN/MENU.WS` from the storage card.

### IPL1 (recovery)

The "recovery" variant instead reserves space for testing functionality, such as:

- performing a test of onboard PSRAM and SRAM memory,
- allowing to load a dedicated recovery program from SPI flash.

## Recovery

The recovery program is up to 256 KiB in size (or 192 KiB for the factory-provided version). It is formatted like a standard WS cartridge image.

It is better documented as part of the [User Guide](/user/troubleshooting/recovery).

## MENU.WS

The menu program is loaded to the final banks of PSRAM (bank `0xFF` downwards). It is formatted like a standard WS cartridge image.
