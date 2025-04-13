---
title: 'Quickstart'
weight: 0
---

To set up nileswan, you're going to need a removable storage card, sometimes referred to as a TF card, formatted
using the FAT16 or FAT32 file system.

{{< hint type=note >}}
nileswan supports cards of up to 2 terabytes in size. However, cards over 32 gigabytes may be formatted using the exFAT file system by default, particularly on Windows. The exFAT file system is not currently supported by nileswan.
{{< /hint >}}

You're also going to need a *menu program*. This is the first program launched by nileswan's boot loader. While we recommend using one of the following:

* **[swanshell](https://github.com/asiekierka/swanshell/releases)** - the default main menu program

{{< hint type=caution >}}
The boot loader doesn't support loading arbitrary programs. Don't use this functionality as an auto-boot workaround - you may be disappointed.
{{< /hint >}}

Download the `.ZIP` file and extract its contents to the storage card. You should have the folder `NILESWAN` on the storage card, and a  `MENU.WS` file inside that folder. Next, make sure the eject to card and put it in the cartridge.

Now you should be able to turn on your console and make use of the cartridge's functionality.

If you run into problems, make sure to check the [Troubleshooting](../troubleshooting) section.
