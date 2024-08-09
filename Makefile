all: stage0 programmer bitstream ipl1

stage0:
	cd firmware/stage0 && make

programmer:
	cd programmer && make

bitstream:
	cd rtl && make

ipl1: libnile-ipl1
	cd ipl1 && make

libnile-ipl1:
	cd libnile && make TARGET=ipl1 install

clean:
	cd firmware/stage0 && make clean
	cd programmer && make clean
	cd rtl && make clean
