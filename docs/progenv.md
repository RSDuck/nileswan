# Programming environment

## Register map

| Addr | Width  | Name | Description  |
|------|-|-----|----|
| `0xE0` |2 | SPI_CNT | SPI control register |
| `0xE2`|1 | POW_CNT | Power control |
| `0xE3` |1| NILE_IRQ | Controls nileswan IRQ |
| `0xE4`|2 | BANK_MASK | Mask for bank index |

## Banks

**ROM**

| Bank(s) | Description |
|------------|-------------|
| 0-127 | First PSRAM (8 MB) |
| 128-255 | Second PSRAM (8 MB, optional) |
| 256-509 | Unused (open bus) |
| 500 | Bootrom mirror (Pocket Challange V2 boot location) |
| 501-509 | Unused (open bus) |
| 510 | SPI RX buffer (read only, 512 bytes mirrored) |
| 511 | Bootrom (WonderSwan boot location)|

**RAM**

| Bank(s) | Description |
|------------|-------------|
| 0-7 | SRAM (512 KB) |
| 8-13 | Unused (open bus) |
| 14 | IPC buffer (512 bytes mirrored) |
| 15 | SPI TX buffer (write only, 512 bytes mirrored) |

SPI RX and TX buffer are double buffered with only one currently visible. This is controlable via `SPI_CNT`.

The PSRAM is writeable by using the self flash mode, which maps ROM at the point in the address space at which RAM usually sits (`0x10000`-`0x1FFFF`). While it is technically possible to read the SPI RX buffer and bootrom in this state, the read values will be wrong. These memories only support 16-bit wide accesses.

## Registers

### SPI

**`0xE0` - `SPI_CNT` (16 bit)**
| Bit(s) | Description |
|------|------|
|0-8|SPI transfer length in bytes minus one|
|9-10|Mode (0 = write, 1 = read, 2 = exchange, 3 = wait and read) |
|11|Transfer speed (0 = 24 MHz "high frequency clock", 1 = 384 kHz from cartbus)|
|12-13|Channel select/chip select (0=no device selected and output to TF channel, 1=select TF, 2=select flash, 3=select μC)|
|14|Memory mapped RX and TX buffer index (0-1)|
|15|Start/busy, when written (0=abort transfer, 1=start transfer), when read (0=idle, 1=transfering)|

* In read mode all output serial bits are 1. The incoming bits are stored in the RX buffer.
* In write mode bits from the TX buffer are output. The incoming serial bits are discarded.
* In exchange mode the TX buffer is output, while simultanously the RX buffer is populated with incoming data.
* Wait and read mode behaves like read mode except bytes are only stored with the first byte received which is not 0xFF.

During transfer the data in the TX buffer currently not mapped into the address space is sent out. The received data is stored in the RX buffer currently not memory mapped. The transfer always starts from the beginning of the buffers.

All values besides the transfer abort are read only while a transfer is in progress.

The selected clock will also be used for EEPROM and RTC serial transactions.

### Power

**`0xE2` - `POW_CNT` (8 bit)**
| Bit(s) | Description |
|------|------|
|0|Enable 24 MHz high frequency clock. (0=off, 1=on (default))|
|1|Enable TF power (0=off (default), 1=on)|
|2|Enable nileswan I/O registers (0=off, 1=on (default))|
|3|Enable Bandai 2003 banking (0=off, 1=on (default))|
|4|Emulated serial device mode (0=RTC (default), 1=EEPROM)|
|5-6|Unused/0|
|7|μC reset line|

Once the nileswan I/O registers are disabled, they can only be brought back via hardware reset.

Depending on the selected serial device mode the appropriate I/O registers for RTC (`RTC_CTRL` and `RTC_DATA`) or EEPROM (`CART_SERIAL_DATA`, `CART_SERIAL_COM` and `CART_SERIAL_CTRL`) become valid or invalid.

The μC reset line bit directly connects to the nRST pin of the microcontroller.

### Interrupts

**`0xE3` - `NILE_IRQ` (8 bit)**
| Bit(s) | Description |
|------|------|
|0|Enable SPI IRQ generation|
|1|SPI IRQ status, when read (0 = no IRQ, 1 = IRQ), when written (0 = nothing, 1 = acknowledge IRQ)|
|2-7|Unused/0|

If SPI IRQ generation is enabled an IRQ will be generated whenever the the busy bit of `SPI_CNT` changes from 1 to 0. It is signalled and acknowledgeable via the SPI IRQ status bit.

### Banking control

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