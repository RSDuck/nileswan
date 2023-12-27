# nileswan

An attempt at creating flash cartridge for Wonderswan.

Expect exciting things here.

## Status

Currently a prototype PCB is done which houses an FPGA, 8 MB of PSRAM and an SD card reader. I had the PCB manufactured and put together one board, which I'm using for development of the HDL and firmware and fault finding.

The PCB is still far from done and features like saving are still missing completely from the hardware side. The entire board layout will be mostly redone.

## Errata/Notes for next PCB iteration

- When assembling the PCB completely and connecting power to it is not possible to write to the SPI flash. The reason for this is that the FPGA blocks the SPI bus the whole time, trying to read its bitstream. CRESET_B needs to be exposed via the programming header (by pulling it low the FPGA stops trying to configure itself).
- Label the LEDs!
- The clk and standby pins of the 25 MHz oscillator are swapped on the FPGA (not that bad, but preferably clk would go directly into a global buffer)
- Better programming connector (probably TAG connect which could also be put on the backside, saving precious space on the front side)
- A0-A15 of the bus (bytewise addressing) is wired to the PSRAM A0-A15 (word wise)
- PSRAM /LB and /UB need to be wired individually to the FPGA, so that it's possible to write to it in 8-bit quantities with it mapped as SRAM (with a little help from the FPGA). For the time being software support is necessary.
- Most likely because of inrush current monochrome Wonderswans will only warm start with the nileswan (i.e. by turning it on and then off and then on again). A load switch with slew rate control could probably fix this
- Contact alignment seems to be still an issue