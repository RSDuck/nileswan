.PHONY: all clean program-fpga libnile libnile-ipl1 ipl0 ipl1 fpga

all: ipl0 ipl1 fpga

libnile:
	cd software/libnile && make TARGET=wswan/medium

libnile-ipl1:
	cd software/libnile && make TARGET=ipl1

ipl0:
	cd software/ipl0 && make

ipl1: libnile-ipl1
	cd software/ipl1 && make

fpga: ipl0
	cd fpga && make

program-fpga: fpga
	cd fpga && make program

clean:
	cd software/libnile && rm -rf build
	cd software/ipl0 && make clean
	cd software/ipl1 && make clean
	cd fpga && make clean
