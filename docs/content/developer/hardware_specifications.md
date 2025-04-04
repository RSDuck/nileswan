---
title: 'Hardware overview'
weight: 0
---

nileswan emulates the following hardware:

- Up to 16 MiB of ROM space
- Up to 512 KiB of RAM space, with battery back-up
- 2001 and 2003-compatible I/O ports, including port 0xCE
- Up to 2 KiB of EEPROM space (M93LC46, M93LC76 and M93LC86 compatible), with back-up
- S-3511A compatible real-time clock

In addition, nileswan provides the following native hardware facilities for user homebrew:

- Removable storage card, via SPI interface
- USB port for CDC serial transfers, via MCU and SPI interface

## Limitations

Most of these limitations should not impact end users; however, documenting them may be of use to homebrew developers.

- The 8-bit ROM access width is not supported. This may be resolved in the future with a firmware update.
- 32 KiB SRAM is currently mapped as if it were 64 KiB SRAM - mirroring is not emulated. This may be resolved in the future with a firmware update.
- EEPROM write and erase timings are not guaranteed to match real hardware. This may be improved in the future with firmware updates. Note that, in general, EEPROM write and erase timings are not guaranteed to be consistent across cartridges.
- RTC command timings are slightly slower than on real hardware. This is a compromise between accuracy and power consumption.
- Due to hardware limitations, the emulation of the MBM29DL400TC NOR flash chip used on certain cartridges is very limited. Writes are correctly passed through, but other commands, including erases, may be stubbed or unimplemented.
- The ability to set jumpers using GPIO pins is not yet implemented. This will be resolved in the future with a firmware update.
- No infrared transmitter or transceiver is present on the cartridge board.
- No sonar is connected to the cartridge board.
