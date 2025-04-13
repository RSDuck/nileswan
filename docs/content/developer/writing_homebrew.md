---
title: 'Writing homebrew'
weight: 2
---

This article provides a brief overview of the basics of writing homebrew for nileswan. For more detailed information, make sure to study the other articles in the developer guide.

## Libraries

Users of the Wonderful toolchain can use [libnile](https://github.com/49bitcat/libnile), an official C library providing access to most of the cartridge's functionality. Documentation for libnile is available [here](https://49bitcat.com/docs/libnile/).

## Unlocking

After launching a program, nileswan cartridge functionality is locked by default. There are two separate unlocks required to take advantage of all its features, listed below.

### I/O unlock

By default, nileswan I/O ports are locked, that is not writable by software. One can unlock these I/O ports by writing the magic value `0xDD` to port `0xE2` (`POW_CNT`).

{{< hint type=note >}}
One can also write `0x00DD` to disable any cartridge behavior emulation features enabled by the menu program by default.
{{< /hint >}}

Note that this also performs all the operations of this value: disables TF card power, enables 24 MHz clock power, enables SRAM access, disables EEPROM emulation, flash emulation, et cetera. If you're using some of these features differently (such as not making use of the 24 MHz clock), it is recommended to follow this with a write that disables unused functionality.

In libnile, this corresponds to the `nile_io_unlock();` function.

### Bank unlock

However, this doesn't take care of banking.
By default, a mask is applied which prohibits accessing banks outside of the cartridge image's ROM and RAM space by mirroring that space.

In order to allow conveniently accessing nileswan-exclusive banks, functionality is provided to ignore this mask on a per-area basis (SRAM, ROM0, ROM1) via the `BANK_MASK` I/O port. In addition, for ROM banks, one has to use the extended 2003 banking ports (`0xD0`-`0xD5`), as their range on nileswan extends past 8 bits.

{{< hint type=caution >}}
ROM0 and ROM1 are typically allocated from bank `0xFF` down; however, cartridge images are loaded from `0x00` up. Disabling the bank mask on these areas can cause invalid data to be read unless the bank registers are adjusted to match.
{{< /hint >}}

In libnile, the `nile_bank_unlock();` function is provided, which automatically adjusts and unlocks SRAM, ROM0 and ROM1.
