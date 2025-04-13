---
title: 'I/O port map'
weight: 10
---
|    Address     |Width|    Name    | Description |
|----------------|-----|------------|-------------|
| `0xE0`         |  2  | SPI_CNT    | SPI control register |
| `0xE2`         |  1  | POW_CNT    | Power control |
| `0xE3`         |  1  | EMU_CNT  | Behavior control (EEPROM, flash emulation, ...) |
| `0xE4`         |  2  | BANK_MASK  | Mask for bank index |
| `0xE6` (read)  |  1  | BOARD_REVISION | Board revision |
| `0xE6` (write) |  1  | WARMBOOT_CNT | Trigger FPGA warmboot |
| `0xE8`         |  1  | IRQ_ENABLE | Cartridge IRQ enable |
| `0xE9`         |  1  | IRQ_STATUS | Cartridge IRQ status |

## SPI interface

**`0xE0` - `SPI_CNT` (16-bit, read/write)**
| Bit(s) | Description |
|------|------|
|0-8|SPI transfer length in bytes minus one|
|9-10|Mode (0 = write, 1 = read, 2 = exchange, 3 = wait and read) |
|11|Transfer speed (0 = 24 MHz "high frequency clock", 1 = cartridge bus clock, usually 384 KHz) |
|12-13|Channel select/chip select (0 = no device selected and output to TF channel, 1 = select TF, 2 = select flash, 3 = select μC)|
|14|Memory mapped RX and TX buffer index (0-1)|
|15|Start/busy, when written (0 = abort transfer, 1 = start transfer), when read (0 = idle, 1 = transferring)|

* In read mode all output serial bits are 1. The incoming bits are stored in the RX buffer.
* In write mode bits from the TX buffer are output. The incoming serial bits are discarded.
* In exchange mode the TX buffer is output, while simultaneously the RX buffer is populated with incoming data.
* Wait and read mode behaves like read mode except bytes are only stored with the first byte received which is not 0xFF.

During transfer the data in the TX buffer currently not mapped into the address space is sent out. The received data is stored in the RX buffer currently not memory mapped. The transfer always starts from the beginning of the buffers.

All values besides the transfer abort are read only while a transfer is in progress.

Transfer aborting is not immediate for internal reasons and to allow the transfer to end cleanly on a byte boundary. After an abort is issued the busy bit will continue to be high until the abort is finally completed.

For EEPROM or RTC SPI communication to work, the cartridge serial clock has to be selected.

## Cartridge control

**`0xE2` - `POW_CNT` (8-bit, read/write)**

| Bit(s) | Description |
|------|------|
|0|Enable 24 MHz high frequency clock. (0=off, 1=on (default))|
|1|Enable TF power (0=off (default), 1=on)|
|2|Enable nileswan I/O registers (0=off, 1=on (default))|
|3|Enable 2001 mapper-specific I/O registers (0=off, 1=on (default))|
|4|Enable 2003 mapper-specific I/O registers (0=off, 1=on (default))|
|5|Pull μC BOOT0/μC busy high (0=no/μC may communicate via it, 1=yes)|
|6|Enable SRAM (0=off, 1=on (default))|
|7|μC reset line|

If `0xDD` is written to `POW_CNT` it will reset the entire register even if nileswan registers are disabled. This allows unlocking the nileswan I/O ports after they have been disabled.

Disabling a range of I/O registers only changes their visibility; it does not disable their functiion. E.g. the upper banking bits of the 2003 mapper continue to apply even if the registers used to change them are not accessible anymore.

The μC reset line bit is connected directly to the nRST pin of the microcontroller.

μC BOOT0 controls whether the microcontroller starts from bootloader ROM or programmable flash (see STM32 documentation). The line is shared with busy line for emulated EEPROM operations and is open drain idling at low.

If SRAM is enabled SRAM (banks 0-7) may be selected when accessing the RAM area.

**`0xE3` - `EMU_CNT` (8-bit, read/write)**

| Bit(s) | Description |
|------|------|
|0-1|Emulated EEPROM size (0=128B, 1=1KB, 2=2KB, 3=no EEPROM connected (default))|
|2|Flash emulation enable (0=disabled (default), 1=enabled)|
|3|Emulated ROM bus width (0=16-bit (default), 1=8-bit)|
|4-7|Unused/0|

See section on EEPROM for details on EEPROM size.

When flash emulation the FPGA will provide minimal emulation of the programming sequences of parallel NOR flash memory for PSRAM accesses.

**`0xE6` - `BOARD_REVISION` (8-bit, read)**

| Bit(s) | Description |
|------|------|
|0-7|Board revision|

Used for distinguishing boards during firmware updates.

|Value | Description |
|------|-------------|
| 0x00 | rev. 6 |
| 0x01 | rev. 7 |

**`0xE6` - `WARMBOOT_CNT` (8-bit, write)**

| Bit(s) | Description |
|------|------|
|0-1|Warmboot image to boot (0-3)|

After writing the FPGA will load one of four FPGA cores from SPI flash depending on the value written. The core image locations are documented in the SPI flash layout.

It may only be written while not running code from the cartridge. After writing, a wait of approximately 20 milliseconds is necessary until the cartridge responds again.

{{< hint type=important >}}
If your FPGA core image does not [enable](https://github.com/YosysHQ/icestorm/pull/332) a faster frequency range,
the wait may need to be as high as 53 milliseconds.
{{< /hint >}}

## Interrupt handling

**`0xE8` - `IRQ_ENABLE` (8-bit, read/write)**

| Bit(s) | Description |
|------|------|
|0|Pass MCU IRQ line|
|1-7|Unused/0|

**`0xE9` - `IRQ_STATUS` (8-bit, read/write)**

| Bit(s) | Description |
|------|------|
|0|MCU IRQ line|
|1-7|Unused/0|

## Banking

**`0xE4` - `BANK_MASK` (16-bit, read/write)**
| Bit(s) | Description |
|------|------|
|0-8|Mask to be applied to ROM bank index|
|9|Use mask for accesses in ROM0 area (0 = no, 1 = yes (default))|
|10|Use mask for accesses in ROM1 area (0 = no, 1 = yes (default))|
|11|Use mask for accesses in RAM area (0 = no, 1 = yes (default))|
|12-15|Mask to be applied to RAM bank index|

To allow booting from bootrom the masks are initialised with all bits set.

Bank mask is always applied to the extended ROM bank (starting from 0x40000).
