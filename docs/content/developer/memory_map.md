---
title: 'Memory bank map'
weight: 15
---

## ROM (bank 0, bank 1, linear bank)

| Bank(s) | Description |
|---------|-------------|
|  0-127  | First PSRAM (8 MB) |
| 128-255 | Second PSRAM (8 MB) |
| 256-499 | Reserved (open bus) |
|   500   | Bootrom mirror (pin strap/PCv2 boot location) |
| 501-509 | Reserved (open bus) |
|   510   | SPI RX buffer (read only, 512 bytes mirrored) |
|   511   | Bootrom (WS/WSC boot location) |

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
    |          - 0: no card
    |          - 1: MMCv3
    |          - 2: MMCv4 (> 2 GB)
    |          - 3: TF
    |          - 4: TF (> 2 GB)
    +--------- Card uses block instead of byte addressing

## SPI RX/TX buffer

The SPI RX and TX buffer are double-buffered. This means one buffer is visible to the console, while the other is used by the FPGA for facilitating an ongoing SPI transfer. These buffers can be swapped using `SPI_CNT`.
