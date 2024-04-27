# nileswan

An attempt at creating flash cartridge for Wonderswan.

Expect exciting things here.

## Status

A third iteration of the PCB is done which contains 2*8 MB of PSRAM, 512 KB of SRAM and a microcontroller with USB. It has been tested to work with the caveats listed in the errata.

## icepack

For the FPGA to be ready in time the bitstream needs to be loaded at the highest speed possible. To achieve this icepack currently needs to be patched in this spot: https://github.com/YosysHQ/icestorm/blob/1a40ae75d4eebee9cce73a2c4d634fd42ed0110f/icepack/icepack.cc#L623 to
```c++
this->freqrange = "high";
```

Eventually I'll create a PR for this but I've been too lazy.

## Errata/Notes for fourth PCB iteration
- SWD for the microcontroller is not connected
- The microcontroller needs an interrupt line to be connected to the FPGA
- The screw hole in the PCB doesn't work. It needs to be larger so that an entire screw post can fit through it.
- Pin 1 of AP22966DC8 is wrongly marked on the in the IBOM leading me to solder it in the wrong way for the second time!!!!
- The weak pull up during FPGA init seems to be not enough to keep the MCU deselected. A dedicated 10k pull fixes this.
- /RESET from the cartridge bus is not connected to the FPGA. Thus power cycling the WonderSwan while keeping the FPGA active is not possible (useful to then save the emulated EEPROM).
- The link on the silkscreen is wrong