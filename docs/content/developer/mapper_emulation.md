---
title: 'Mapper emulation'
weight: 20
---

## EEPROM

If 2001 mapper registers are enabled in `POW_CNT`, the registers associated with EEPROM (`0xC4`-`0xC8`) are accessible and writeable. The FPGA emulates the entire EEPROM including keeping its content of up to 2 KB in internal RAM.

This is necessary as the interface does not provide an reliable way to tell when a read is done and thus typically software only relies on waiting for a fixed amount. The μC is unable to respond with such low latency.

EEPROMs of different sizes have small differences in command scheme and mask addresses to their respective size. `EMU_CNT` can be used to set the size of the emulated EEPROM. Alternatively `EMU_CNT` can be configured to emulate the absence of an EEPROM chip.

To store save data across power cycles write and erase commands are additionally output as-is via SPI to the μC. First the contents of `CART_SERIAL_COM` is output from the highest to lowest bit, for write commands `CART_SERIAL_DATA` is output from highest to lowest bit too.

## RTC

Whenever 2003 mapper registers are enabled via `POW_CNT` RTC registers can be accessed (`0xCA` and `0xCB`). It performs like a real 2003 mapper and provides a serial interfaces specifically made for the S-3511A RTC chip. It is connected to the μC which emulates the S-3511A.

## Flash

The PSRAM banks are writable by using the self flash mode (port 0xCE), which maps PSRAM at the point in the address space at which RAM usually sits (`0x10000`-`0x1FFFF`).

Normally, this is accessible as regular RAM; the flash emulation mode can be enabled to inhibit writes which are not a result of valid flash write commands.
