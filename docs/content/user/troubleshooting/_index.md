---
title: 'Troubleshooting'
weight: 40
---

This page lists various nileswan issues that may be encountered, as well as solutions.

## Startup problems

### Console does not turn on with cartridge inserted

If no other cartridges work, this implies a fault with the console; fixing your console is outside the scope of this documentation.

If other cartridges work, the nileswan cartridge's contacts may be dirty. Clean up any dirt or grime on the contacts, then try re-inserting the cartridge.

The factory FPGA core data may be corrupt. This is a brick scenario which cannot be recovered in software; as such, you will need to use an external SPI flashing device to resolve it.

Finally, there may be a hardware fault with the device itself.

### Console displays a blank screen after finishing the splash screen sequence

This issue typically points to SPI flash data corruption.

The factory FPGA and IPL data may be intact. If so, follow the [Recovery -> Fixing corrupt update data](recovery#fixing-corrupt-update-data) guide, then install the latest firmware update.

If this alone does not help, the factory FPGA core data itself may be corrupt. In this case, software recovery cannot be guaranteed; as such, you will need to use an external SPI flashing device to resolve it.

## IPL problems

The initial program loader, or IPL, is responsible for loading an user-provided menu program from the removable storage card. In the case of an error, the screen turns black and an error message is displayed, like so:

![](/img/ipl1_error.png)

Here are some common error messages. Note that an error may also point to a software bug, which may need to be reported to the developers.

**Storage not ready**

The removable storage card was not detected. Please try re-inserting it.

**Storage I/O error**

The removable storage card was detected, but an error occured while reading from it. This could point to a hardware fault with the card itself.

**FAT filesystem not found**

The removable storage card was detected, but a FAT16/FAT32 file system was not detected.

nileswan does not currently support exFAT file systems or GPT partition tables. If your card is formatted using exFAT, you will need to manually format it to use FAT32. This is typically a problem for cards larger than 32 GiB.

**File not found**

The menu file (`/NILESWAN/MENU.WS`) was not found on the cartridge.

## Other problems

### Persistent data (save data, date/time, ...) corruption

This issue implies the SRAM and/or MCU are not receiving power.

Make sure a battery is inserted and that the battery holds charge. If that does not resolve the issue, it may point to a software bug or hardware fault.

