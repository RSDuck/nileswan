# Power draw

As the default mean of powering a Wonderswan is a single AA battery keeping an eye on the power consumption of the cartridge is very important. This is especially true for the original Wonderswan model, where the boost converter is said to be less capable than the ones in the later two models.

## Power draw of a cartridge

Some figures on the power draw of a cartridge:

| Setup | Battery draw (in mA at 1.5 V) |
|-------|-------------------------------|
|Chocobo Dungeon (real cartridge), title screen|36|
|Crazy Climber (real cartridge), intro animation|42|
|Crazy Climber (externally powered FPGA board), intro animation|36|

All measurements were made on a monochrome Wonderswan.

From this limited data we can conclude that a ROM only cartridge such as Crazy Climber draws a current of about 6 mA.

## microSD cards

Even if we try to hold the power consumption of the cartridge as low as possible, if we want to use microSD cards as a storage medium, we need to deal with their power consumption. Fortunately once the ROM is loaded into PSRAM this is not an concern anymore, assuming the microSD card has a low enough sleep current.

To keep concurrent power draw low, it is probably the smartest to not select the SD card and PSRAM at the same time. Putting the processor to sleep and buffering into the FPGA's integrated SRAM might be the way to go.

Based on various sources, at a relatively low frequency of a few MHz, a microSD card can still be expected to draw on average between 20-30 mA at 3.3 V, with very current spikes from time to time.

## Monochrome Wonderswan: stress tests

An additional load was added to the 3.3 V line of a monochrome Wonderswan. Initially white LEDs in series with 75 Ohm resistors were used. After I ran out of those, white LEDs in series with 180 Ohm resistors were used. After running dry of LEDs, 180 Ohm resistors were put directly between 3.3 V and GND.

For the tests the intro of Gunpey was running.

| Voltage (in V) | Additional current (in mA) |
|----------------|-----------------|
|3.256|0|
|3.242|27|
|3.226|42.8|
|3.2| 56-57|
|3.16|72|
|3.11|84|
|2.99|94|

Concluding from this, even on a monochrome Wonderswan it should be possible to use a microSD card, assuming there are no other additional consumers of power such as backlighted displays.
