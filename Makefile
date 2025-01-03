include config.mk

DISTDIR  ?= out/dist
MFGDIR   ?= out/manufacturing
MANIFEST ?= manifest/full_update.txt
UPDATEWS := $(DISTDIR)/fwupdate.ws
FLASHBIN := $(MFGDIR)/spi.bin

.PHONY: all clean help program-fpga program libnile libnile-ipl1 ipl0 ipl1 recovery updater fpga

all: ipl0 ipl1 recovery fpga $(UPDATEWS) $(FLASHBIN)

help:
	@echo "nileswan build system"
	@echo ""
	@echo "all              Build all components"
	@echo "                 User distributables are stored in $(DISTDIR)"
	@echo "                 Manufacturing files are stored in $(MFGDIR)"
	@echo "program-fpga     Build and program initial FPGA bitstream"
	@echo "program          Build and program complete SPI flash contents"

libnile:
	cd software/libnile && make TARGET=wswan/medium && make -j1 TARGET=wswan/medium install

libnile-ipl1:
	cd software/libnile && make TARGET=ipl1 && make -j1 TARGET=ipl1 install

ipl0:
	cd software/ipl0 && make

ipl1: libnile-ipl1
	cd software/ipl1 && make

recovery: libnile
	cd software/recovery && make

updater: libnile
	cd software/updater && make

$(FLASHBIN): fpga ipl1 recovery $(MANIFEST) software/updater/manifest_to_bin.py
	@mkdir -p $(@D)
	python3 software/updater/manifest_to_bin.py $(MANIFEST) $@

$(UPDATEWS): fpga ipl1 recovery updater $(MANIFEST) software/updater/manifest_to_rom.py
	@mkdir -p $(@D)
	python3 software/updater/manifest_to_rom.py software/updater/updater_base.ws $(MANIFEST) $@

fpga: ipl0
	cd fpga && make

program-fpga: fpga
	cd fpga && make program

program: $(FLASHBIN)
	iceprog $<

clean:
	rm -rf out
	cd software/libnile && rm -rf build
	cd software/ipl0 && make clean
	cd software/ipl1 && make clean
	cd software/recovery && make clean
	cd software/updater && make clean
	cd fpga && make clean
