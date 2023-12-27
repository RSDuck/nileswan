all: stage0 programmer bitstream

stage0:
	cd firmware/stage0 && make

programmer:
	cd programmer && make

bitstream:
	cd rtl && make

clean:
	cd firmware/stage0 && make clean
	cd programmer && make clean
	cd rtl && make clean
