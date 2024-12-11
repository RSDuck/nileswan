BUILDDIR := build

.PHONY: all clean

all: ipl0.asc

$(BUILDDIR)/fix: fix.nim
	@echo "  NIM     $<"
	@$(MKDIR) -p $(@D)
	nim c --outdir:$(BUILDDIR) $<

ipl0.bin: ipl0.s swan.inc
	@echo "  NASM    $<"
	nasm $< -o $@

ipl0.asc: ipl0.bin $(BUILDDIR)/fix
	@echo "  FIX     $<"
	$(BUILDDIR)/fix fpga $@ $<

ipl0.ws: ipl0.bin $(BUILDDIR)/fix
	@echo "  FIX     $<"
	$(BUILDDIR)/fix fullrom $@ $<

clean:
	@echo "  CLEAN"
	rm -f ipl0.asc ipl0.bin ipl0.ws
	rm -rf build
