# inspired by https://github.com/damdoy/ice40_ultraplus_examples
# and https://github.com/pwmarcz/fpga-chip8/blob/master/Makefile

build_folder := build

all: build

###############################################
# IPL0
###############################################

$(build_folder)/fix: ipl0/fix.nim
	nim c --outdir:$(build_folder) $<

$(build_folder)/ipl0.bin: ipl0/ipl0.s
	nasm $< -o $@

$(build_folder)/ipl0.asc: $(build_folder)/ipl0.bin $(build_folder)/fix
	$(build_folder)/fix fpga $@ $<

ipl0.ws: $(build_folder)/ipl0.bin $(build_folder)/fix
	$(build_folder)/fix fullrom $@ $<

###############################################
#  Build
###############################################

project := nileswan
pcf_file := io.pcf
pcf_path := rtl/$(pcf_file)
pnr_flags := --hx1k --package vq100 --freq 25
rtl_source_paths := $(wildcard rtl/*.sv)

bram_seed := 123456

$(build_folder)/bram_init.asc:
	icebram -g -s $(bram_seed) 16 256 >$@

$(build_folder)/$(project).json: $(rtl_source_paths) $(build_folder)/bram_init.asc
	yosys -p "synth_ice40 -top $(project) -json $@" rtl/$(project).sv

$(build_folder)/$(project).asc: $(build_folder)/$(project).json $(pcf_path)
	nextpnr-ice40 $(pnr_flags) --json $< --pcf $(pcf_path) --asc $@

$(build_folder)/$(project).inserted.asc: $(build_folder)/$(project).asc $(build_folder)/bram_init.asc $(build_folder)/ipl0.asc
	icebram -v $(build_folder)/bram_init.asc $(build_folder)/ipl0.asc <$< >$@

$(project).bin: $(build_folder)/$(project).inserted.asc
	"icepack.exe" -s $< $@

###############################################
#  Simulation
###############################################

ICE40_LIBS := $(shell yosys-config --datdir/ice40/cells_sim.v)

bench_sources := $(wildcard rtl/benches/*.sv)
bench_out_files := $(foreach source,$(notdir $(bench_sources)),$(source))
bench_fst_files := $(bench_out_files:.sv=.fst)


// https://github.com/no2fpga/no2build/blob/master/core-rules.mk
$(build_folder)/%.tb: rtl/benches/%.sv $(ICE40_LIBS) $(rtl_source_paths) $(build_folder)/ipl0.asc
	iverilog -g 2012 -o $@ -I rtl/benches -I rtl -l $(ICE40_LIBS) $<

%.fst: $(build_folder)/%.tb
	vvp $< -fst

simulation: $(bench_fst_files)

###############################################
#  Meta
###############################################

build: $(project).bin ipl0.ws

clean:
	rm -r -f build
	rm -f nileswan.bin
	rm -f ipl0.ws

	rm -f *.vcd

###############################################
# Utility
###############################################

program: build
	iceprog nileswan.bin

$(shell mkdir -p $(build_folder))
