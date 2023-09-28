# nileswan

An attempt at creating flash cartridge for Wonderswan.

Expect exciting things here.

The PCB files are currently untested.

## Errata/Notes for next PCB iteration

- When assembling the PCB completely and connecting power to it is not possible to write to the SPI flash. The reason for this is that the FPGA blocks the SPI bus the whole time, trying to read its bitstream. CRESET_B needs to be exposed via the programming header (by pulling it low the FPGA stops trying to configure itself).
- Label the LEDs!
- The clk and standby pins of the 25 MHz oscillator are swapped on the FPGA
- Better programming connector