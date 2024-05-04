# μController firmware

Firmware for the STM32L052K(6/8)Ux MCU. It is connected on the same SPI bus as the SPI flash and is responsible for USB communication, time keeping RTC and EEPROM emulation. For this reason it is constantly powered by the backup battery.