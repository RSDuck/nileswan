---
title: 'I/O port interface'
weight: 10
---
| Address|Width|    Name    | Description |
|--------|-----|------------|-------------|
| `0xE0` |  2  | SPI_CNT    | SPI control register |
| `0xE2` |  1  | POW_CNT    | Power control |
| `0xE3` |  1  | WARMBOOT_CNT | Trigger FPGA warmboot |
| `0xE4` |  2  | BANK_MASK  | Mask for bank index |
| `0xE6` |  1  | EMU_CNT  | Controls EEPROM size |

## SPI interface

**`0xE0` - `SPI_CNT` (16 bit)**
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

## Power/system control

**`0xE2` - `POW_CNT` (8 bit)**
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

If `0xFD` is written to `POW_CNT` it will reset the entire register even if nileswan registers are disabled. This way the nileswan registers can be brought back after disabling them.

Disabling a range of I/O registers only changes the visibility. E.g. the upper banking bits of the 2003 mapper continue to apply even if the registers used to change them are not accessible anymore.

The μC reset line bit directly connects to the nRST pin of the microcontroller.

μC BOOT0 controls whether the microcontroller starts from bootloader ROM or programmable flash (see STM32 documentation). The line is shared with busy line for emulated EEPROM operations and is open drain idling at low.

If SRAM is enabled SRAM (banks 0-7) may be selected when accessing the RAM area.

** `0xE6` - `EMU_CNT`

| Bit(s) | Description |
|------|------|
|0-1|Emulated EEPROM size (0=128B, 1=1KB, 2=2KB, 3=no EEPROM connected (default))|
|2|Flash emulation enable (0=disabled (default), 1=enabled)|
|3-7|Unused/0|

See section on EEPROM for details on EEPROM size.

When flash emulation the FPGA will provide minimal emulation of the programming sequences of parallel NOR flash memory for PSRAM accesses.

** `0xE7` - `WARMBOOT_CNT`

| Bit(s) | Description |
|------|------|
|0-1|Warmboot image to boot (0-3)|

After writing the FPGA will load one of four FPGA cores from SPI flash depending on the value written. See SPI flash layout on where the images lie. The port is write-only.

The port may only be written while not running code from the cartridge. After writing a wait of approximately 20 ms is necessary until the cartridge responds again.

## Banking

**`0xE4` - `BANK_MASK` (16 bit)**
| Bit(s) | Description |
|------|------|
|0-8|Mask to be applied to ROM bank index|
|9|Use mask for accesses in ROM0 area (0 = no, 1 = yes (default))|
|10|Use mask for accesses in ROM1 area (0 = no, 1 = yes (default))|
|11|Use mask for accesses in RAM area (0 = no, 1 = yes (default))|
|12-15|Mask to be applied to RAM bank index|

To allow booting from bootrom the masks are initialised with all bits set.

Bank mask is always applied to the extended ROM bank (starting from 0x40000).

## EEPROM

If 2001 mapper registers are enabled in `POW_CNT`, the registers associated with EEPROM (`0xC4`-`0xC8`) are accessible and writeable. The FPGA emulates the entire EEPROM including keeping its content of up to 2 KB in internal RAM.

This is necessary as the interface does not provide an reliable way to tell when a read is done and thus typically software only relies on waiting for a fixed amount. The μC is unable to respond with such low latency.

EEPROMs of different sizes have small differences in command scheme and mask addresses to their respective size. `EMU_CNT` can be used to set the size of the emulated EEPROM. Alternatively `EMU_CNT` can be configured to emulate the absence of an EEPROM chip.

To store save data across power cycles write and erase commands are additionally output as-is via SPI to the μC. First the contents of `CART_SERIAL_COM` is output from the highest to lowest bit, for write commands `CART_SERIAL_DATA` is output from highest to lowest bit too.

## RTC

Whenever 2003 mapper registers are enabled via `POW_CNT` RTC registers can be accessed (`0xCA` and `0xCB`). It performs like a real 2003 mapper and provides a serial interfaces specifically made for the S-3511A RTC chip. It is connected to the μC which emulates the S-3511A.

## Memory bank layout

### ROM (ROM0, ROM1, ROML)

| Bank(s) | Description |
|---------|-------------|
|  0-127  | First PSRAM (8 MB) |
| 128-255 | Second PSRAM (8 MB, optional) |
| 256-499 | Unused (open bus) |
|   500   | Bootrom mirror (Pocket Challange V2 boot location) |
| 501-509 | Unused (open bus) |
|   510   | SPI RX buffer (read only, 512 bytes mirrored) |
|   511   | Bootrom (WonderSwan boot location) |

The PSRAM banks are writable by using the self flash mode (port 0xCE), which maps ROM at the point in the address space at which RAM usually sits (`0x10000`-`0x1FFFF`).

Note that while it is technically possible to read the SPI RX buffer and bootrom in the RAM bank, the values read will be incorrect - the RAM bank only supports byte accesses, while these banks only support 16-bit wide word accesses.

### RAM

|Bank(s)| Description |
|-------|-------------|
| 0-7   | SRAM (512 KB) |
| 8-13  | Unused (open bus) |
| 14    | IPC buffer (512 bytes mirrored) |
| 15    | SPI TX buffer (write only, 512 bytes mirrored) |

## Memory banks

### RAM bank 14: IPC

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

#### Cold boot entrypoint

* 0 - FFFF:0000 (standard - WS/WSC)
* 1 - 4000:0000 (alternate #1)
* 2 - 4000:0010 (alternate #2 - PCv2)

#### TF card status

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

### ROM bank 510, RAM bank 15: SPI buffers

The SPI RX and TX buffer are double-buffered. This means one buffer is visible to the console, while the other is used by the FPGA for facilitating an ongoing SPI transfer. These buffers can be swapped using `SPI_CNT`.
