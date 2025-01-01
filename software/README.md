# software

Console-side software used by the nileswan's boot/manufacturing process.

## ipl0

The first stage bootloader; up to 512 bytes, stored in FPGA memory. Its goal is to:

- preserve initial boot state in console RAM for the second stage bootloader,
- choose the second stage bootloader to load (boot, safe),
- load the second stage bootloader from SPI flash to console RAM,
- jump to the second stage bootloader.

## ipl1

The second stage bootloader; up to ~15 kilobytes, stored in SPI flash. It contains
a few sub-variants:

### boot

This is the default second stage bootloader. Its goal is to:

- initialize the IPC memory layout,
- initialize the storage card inserted into the console,
- load a program (`/NILESWAN/MENU.WS`) to PSRAM,
- jump to PSRAM.

### safe

Its goal is to provide recovery options in case the user requests them by holding
the on-cartridge button at boot. This is still being worked on.

## libnile

Common library for nileswan functionality, developed in a separate repository.

## updater

This is the firmware updater program source code. It can be combined with a
manifest file to create an on-console flashable firmware upgrade.
