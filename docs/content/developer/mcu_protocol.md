---
title: 'MCU firmware protocol'
weight: 30
---

This page documents protocols used to communicate with the on-board STM32 MCU via SPI.

## Bootloader protocol

This is the default mode after resetting with BOOT0 pulled high.

For details on this protocol, please refer to the ST Application note AN4286.

## Native mode

This is the default mode after resetting with BOOT0 pulled low.

To initiate a transaction, send a 16-bit little endian value (two bytes) declaring the command ID (in bits 0-6) and parameter (bits 7-15, values 0-511).

Every packet except "Switch to mode" has a response. The response always consists of the length (two bytes) and the data.

### 0x00 - Echo

Test command. The parameter is the number of bytes to echo, followed by the bytes to echo.

### 0x01 - MCU: Switch to mode

The parameter is the ID of the mode of communication to use going forward:

- `00` - command mode
- `01` - EEPROM emulation mode
- `02` - RTC S-3511A emulation mode
- `FF` - standby mode (requires reset to respond to SPI again)

### 0x02 - SPI: Set maximum frequency

The parameter is the maximum frequency to set:

- `00` - 384 KHz
- `01` - 6 MHz

The response is 1 on success, 0 on failure.

### 0x03 - MCU: Get unique ID

The response is the unique ID of the chip.

### 0x10 - EEPROM: Set emulation mode

Set the size of the emulated EEPROM:

- `00` - no EEPROM
- `01` - M93LC06
- `02` - M93LC46 compatible
- `03` - M93LC56 compatible
- `04` - M93LC66 compatible
- `05` - M93LC76 compatible
- `06` - M93LC86 compatible

The response is 1 on success, 0 on failure.

### 0x11 - EEPROM: Erase all data

The response is empty.

### 0x12 - EEPROM: Read data

The parameter is the number of words to read; the following word is the offset.

The response is the bytes read.

### 0x13 - EEPROM: Write data

The parameter is the number of words to write; the following word is the offset, then the words to write.

The response is empty.

### 0x14 - RTC: Send command

The parameter is the packet type to send to the emulated S-3511A, followed by the relevant bytes.

The response is the data returned by the emulated S-3511A.

### 0x15 - EEPROM: Get emulation mode

The response is 1 byte - the EEPROM emulation mode.

### 0x16 - MCU: Set save ID

The parameter is ignored; followed by four bytes of the save ID.

The response is 1 on success, 0 on failure.

### 0x17 - MCU: Get save ID

The parameter is ignored.

The response is four bytes of the save ID.

### 0x40 - USB: CDC: Read

The parameter is the maximum number of bytes to read. The value 0 is treated as 512 bytes.

The response is the data read from the CDC.

### 0x41 - USB: CDC: Write

The parameter is the number of bytes to write. The value 0 is treated as 512 bytes.

The response is two bytes in size and is the number of bytes successfully written.

### 0x42 - USB: HID: Write

The parameter is the length of the packet.

The data sent is two bytes - the result of a keypad scan.

The response is zero bytes in size.

### 0x7F - Reserved

Reserved to distinguish 0xFF bytes from commands.
