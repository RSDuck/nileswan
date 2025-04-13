---
title: 'Memory bank map'
weight: 15
---

### ROM (ROM0, ROM1, ROML)

| Bank(s) | Description |
|---------|-------------|
|  0-127  | First PSRAM (8 MB) |
| 128-255 | Second PSRAM (8 MB, optional) |
| 256-499 | Unused (open bus) |
|   500   | Bootrom mirror (pin strap/PCv2 boot location) |
| 501-509 | Unused (open bus) |
|   510   | SPI RX buffer (read only, 512 bytes mirrored) |
|   511   | Bootrom (WS/WSC boot location) |

The PSRAM banks are writable by using the self flash mode (port 0xCE), which maps ROM at the point in the address space at which RAM usually sits (`0x10000`-`0x1FFFF`).

Note that while it is technically possible to read the SPI RX buffer and bootrom in the RAM bank, the values read will be incorrect - the RAM bank only supports byte accesses, while these banks only support 16-bit wide word accesses.

## RAM

|Bank(s)| Description |
|-------|-------------|
| 0-7   | SRAM (512 KB) |
| 8-13  | Unused (open bus) |
| 14    | IPC buffer (512 bytes mirrored) |
| 15    | SPI TX buffer (write only, 512 bytes mirrored) |

## IPC buffer

This area is used for inter-process communication by software that targets the nileswan.

|Address| Size | Description |
|-------|------|-------------|
| 0x000 |    2 | 0xAA55 if area valid |
| 0x002 |    1 | Cold boot entrypoint |
| 0x003 |    1 | TF card status |
| 0x004 |    4 | Reserved |
| 0x008 |   24 | Cold boot register backup: AX, BX, CX, DX, SP, BP, SI, DI, DS, ES, SS, FLAGS |
| 0x020 |  184 | Cold boot I/O port backup: 0x00 ~ 0xB7 |
| 0x0D8 |    8 | Reserved |
| 0x0E0 |  288 | Free to use for programs |

### Cold boot entrypoint

* 0 - FFFF:0000 (standard - WS/WSC)
* 1 - 4000:0000 (alternate #1)
* 2 - 4000:0010 (alternate #2 - PCv2)

### TF card status

    7  bit  0
    ---- ----
    bttt tttt
    |||| ||||
    |+++-++++- TF card type
    |          - 0x00: no card
    |          - 0x01: MMC (older)
    |          - 0x02: MMC (newer)
    |          - 0x04: TF (older)
    |          - 0x08: TF (newer)
    +--------- Card uses block instead of byte addressing

When disabling removable storage card power, this byte should also be set to `0`; otherwise, filesystem drivers may fail to work correctly.

## SPI RX/TX buffer

The SPI RX and TX buffer are double-buffered. This means one buffer is visible to the console, while the other is used by the FPGA for facilitating an ongoing SPI transfer. These buffers can be swapped using `SPI_CNT`.
