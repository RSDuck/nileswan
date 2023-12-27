all: ipl1.ws ipl1.bin

ipl1.bin : ipl1.s
	nasm $< -o $@

ipl1.ws: ipl1.s
	nasm $< -dEMU -o $@

program: ipl1.bin
	iceprog -o 128k $<
