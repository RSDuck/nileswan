# nileswan

An attempt at creating flash cartridge for Wonderswan.

Expect exciting things here.

## Building instructions

### Requirements

- [OSS CAD Suite](https://github.com/YosysHQ/oss-cad-suite-build) (FPGA core)
  - Requires a modified version of `icepack`, see below for more information
- [Wonderful Toolchain](https://wonderful.asie.pl/wiki/doku.php?id=getting_started) (IPL1, recovery, updater)
- CMake (MCU firmware)
- NASM (IPL0)
- Nim 2.0+ (FPGA core, IPL0)
- Ninja (MCU firmware)
- Python 3.x (SPI images, updater)
  - + `crc` library
- dd, dosfstools, mtools (emulator images)

#### icepack

For the FPGA to be ready in time, the bitstream needs to be loaded at the highest speed possible. I created a [PR](https://github.com/YosysHQ/icestorm/pull/332) for icepack which adds a setting to change this. Without it is not possible to compile the FPGA bitstream.

### Compiling

1. Make sure to fetch the Git submodules: `git submodule update --init --recursive`.
2. Run `make help` to read what build options are possible.
3. Run `make` (or `make ...`) to build the requested components.
