# μController firmware

Firmware for the STM32L052K(6/8)Ux MCU. Its responsibilities include:

- USB communication (CDC, ...),
- real-time clock (time keeping, S-3511A emulation),
- backup EEPROM storage.

For RTC support/EEPROM storage, it is constantly powered by the backup battery, even when the console is powered off.

## Protocol

The MCU is connected on the same SPI bus as the SPI flash.

### Command mode

To initiate a transaction, send two bytes declaring the command ID (bits 0-6) and parameter (bits 7-15, values 0-511).

Each packet has a response. The response always consists of the length (two bytes) and the data.

#### 0x00 - Echo

Test command. The parameter is the number of bytes to echo, followed by the bytes to echo.

#### 0x01 - SPI: Switch to mode

The parameter is the ID of the mode of communication to use going forward:

- 0 - command mode
- 1 - EEPROM emulation mode
- 2 - RTC S-3511A emulation mode

#### 0x02 - SPI: Set maximum frequency

The parameter is the maximum frequency to set:

- 0 - command mode
- 1 - EEPROM emulation mode
- 2 - RTC S-3511A emulation mode

The response is 1 on success, 0 on failure.

#### 0x10 - EEPROM: Set emulation mode

Set the size of the emulated EEPROM:

- 0 - no EEPROM
- 1 - M93LC06
- 2 - M93LC46 compatible
- 3 - M93LC56 compatible
- 4 - M93LC66 compatible
- 5 - M93LC76 compatible
- 6 - M93LC86 compatible

The response is 1 on success, 0 on failure.

#### 0x11 - EEPROM: Erase all data

The response is empty.

#### 0x12 - EEPROM: Read data

The parameter is the number of bytes to read; the following word is the offset.

The response is the bytes read.

#### 0x13 - EEPROM: Write data

The parameter is the number of bytes to write; the following word is the offset, then the bytes to write.

The response is empty.

#### 0x14 - RTC: Send command

The parameter is the packet type to send to the emulated S-3511A, followed by the relevant bytes.

The response is the data returned by the emulated S-3511A.

#### 0x40 - USB: CDC: Read

The parameter is the maximum number of bytes to read. The value 0 is treated as 512 bytes.

The response is the data read from the CDC.

#### 0x41 - USB: CDC: Write

The parameter is the number of bytes to write. The value 0 is treated as 512 bytes.

The response is two bytes in size and is the number of bytes successfully written.

#### 0x42 - USB: HID: Write

The parameter is the length of the packet.

The data sent is two bytes - the result of a keypad scan.

The response is zero bytes in size.

#### 0x7F - Reserved

Reserved to distinguish 0xFF bytes from commands.

### EEPROM mode

The EEPROM mode operates by exchanging words via SPI. It emulates the M93LCx6 interface.

### RTC mode

The RTC mode operates by exchanging bytes via SPI. It emulates the S-3511A interface.
