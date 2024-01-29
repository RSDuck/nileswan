# Programming environment

## Register map

| Addr  | Name | Description  |
|-------|-----|----|
| `0xE0` | SPI_CNT | SPI control register |
| `0xE1` | SPI_LEN | SPI data length |
| `0xE2` | POW_CNT | Power control |
| `0xE4` | SEG_MASK0 | Mask for ROM segment index |
| `0xE5` | SEG_MASK1 | Mask for RAM segment index |

## Segments

**ROM**

| Segment(s) | Description |
|------------|-------------|
| 0-127 | PSRAM (8 MB) |
| 128-255 | Reserved for PSRAM expansion to 16 MB |
| 256 | SPI RX buffer (read only, 512 bytes mirrored) |
| 257-510 | Unused (open bus) |
| 511 | Bootrom |

**RAM**

| Segment(s) | Description |
|------------|-------------|
| 0-7 | SRAM (512 KB) |
| 8 | SPI TX buffer (write only, 512 bytes mirrored) |
| 9-15 | Unused (open bus) |

SPI RX and TX buffer are double buffered with only one currently visible. This is contrable via `SPI_CNT`.

## Registers

### SPI

**`0xE0` - `SPI_CNT` (8 bit)**
| Bit(s) | Description |
|------|------|
|0|MSB of SPI transfer length|
|1|Unusued/0|
|2|Transfer speed (0 = 25 MHz/1 = 390.625 kHz)|
|3|Controls chip select line of the current device (0 = /CS is high, 1 = /CS is low)|
|4|SPI device (0 = SPI flash, 1 = TF)|
|5|Memory mapped RX buffer index (0-1)|
|6|Memory mapped TX buffer index (0-1)|
|7|Start/busy, when written (0=nothing, 1=start transfer), when read (0=idle, 1=busy)|

Rationale for separating device select from chip select: The TF is hooked up on a separate SPI bus so that it does not interfere while the FPGA initialise. Additionally it is necessary to be eable to send data while the TF card is deselected. Having both on a separate bus also allows safely cutting power to the TF card.

During transfer the data in the TX buffer currently not mapped

**`0xE1` - `SPI_CNT` (8 bit)**
| Bit(s) | Description |
|------|------|
|0-7|Length of SPI transfer in bytes|

All values are locked while transfer is in progress.

### Power

**`0xE2` - `POW_CNT` (8 bit)**
| Bit(s) | Description |
|------|------|
|0|Enable 25 MHz high frequency clock. If disabled SPI will stop working. (0=off, 1=on)|
|1|Enable TF power (0=off, 1=on)|
|2-7|Unused/0|

By default the HF oscillator is enabled and TF power is disabled.

### Segment control

**`0xE4` - `SEG_MASK0` (8 bit)**
| Bit(s) | Description |
|------|------|
|0-7|Bits 0-7 of mask to be applied to ROM segment index|

**`0xE5` - `SEG_MASK1` (8 bit)**
| Bit(s) | Description |
|------|------|
|0|Bit 8 of mask to be applied to ROM segment index|
|1-3|Unused/0|
|4-7|Mask to be applied to ROM segment index|

To allow booting from bootrom the masks are initialised with all bits set.
