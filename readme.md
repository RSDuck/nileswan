# nileswan

An attempt at creating flash cartridge for Wonderswan.

Expect exciting things here.

## Status

Currently a prototype PCB is done which houses an FPGA, 8 MB of PSRAM and an SD card reader. I had the PCB manufactured and put together one board, which I'm using for development of the HDL and firmware and fault finding.

The PCB is still far from done and features like saving are still missing completely from the hardware side. The entire board layout will be mostly redone.

## Errata/Notes for third PCB iteration
- TF card power needs a pull down resistor otherwise it will be enabled during the configuration of the FPGA.
- DI and DO are swapped for the SPI flash!!!!!
- Inrush current is still a problem. Removing both 1 μF capacitors of the load switch seems to be necessary? Maybe replace them with 0.2 nF or a similar value. The problem is that if the voltage curve is too sharp the power supply gives up, while the FPGA doesn't initialise fast enough if the voltage rises too slowly. Technically it also violates spec (rise time needs to be between 0.1 and 10 V/ms). Partial solution: the load switch enable is currently directly tied to the input power via a pull up resistor. The load switch already sees 1.2 V as logic high. By using a voltage divider the load switch is only enabled once the power supply has reached a more stable value.
 