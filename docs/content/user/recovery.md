---
title: 'Recovery'
weight: 50
---

nileswan provides facilities to recover the cartridge in case of an unsuccessful flash operaiton or data corruption,
as well as recover the console's internal EEPROM.

## Cartridge recovery

### Fixing corrupt update data

The IPL1 (second stage bootloader) or updated FPGA bitstream data may be corrupted as the result of a failed update.
This can manifest as a blank screen or as a crash on the initial splash screen (before the menu program is loaded).

One can force the use of the factory IPL1 and FPGA bitstream by proceeding as follows:

1. Hold the button at the top of the cartridge.
2. While holding the button, turn on the console.
3. Keep holding the button until the initial splash screen appears.

From there, one can use any firmware update to reflash the update data.

## Console recovery

Some console issues are caused by internal EEPROM corruption:

- a freeze on start can indicate corrupt splash screen data,
- LCD display issues can indicate corrupt LCD timing data on SC consoles with a TFT panel.

### Fixing corrupt splash screen data

To solve the first issue, one can perform the following operation:

1. Hold the button at the top of the cartridge.
2. While holding the button, turn on the console.

This forces the console's IPL to skip any splash screen data in the internal EEPROM.

To permanently solve this issue, one can use the `Internal EEPROM recovery -> Disable custom splash` option in the cartridge recovery program.

### Fixing TFT panel display issues

To solve the second issue, one can use the `Internal EEPROM recovery -> Restore TFT panel data` option in the cartridge recovery program.
