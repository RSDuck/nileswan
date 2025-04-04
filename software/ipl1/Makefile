# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Adrian "asie" Siekierka, 2023

WONDERFUL_TOOLCHAIN ?= /opt/wonderful
TARGET = wswan/bootfriend
include $(WONDERFUL_TOOLCHAIN)/target/$(TARGET)/makedefs.mk

# Source code paths
# -----------------

PROGRAM ?= boot

ifeq ($(PROGRAM),safe)
SOURCEDIRS	:= src/safe src/shared
else
SOURCEDIRS	:= src/boot src/shared
endif

# Defines passed to all files
# ---------------------------

DEFINES		:= -DPROGRAM_$(PROGRAM)

# Libraries
# ---------

LIBS		:= -lnilefs -lnile -lwsx -lws
LIBDIRS		:= $(WF_TARGET_DIR) \
		   ../libnile/dist/ipl1

# Build artifacts
# ---------------

BUILDDIR	:= build/$(PROGRAM)
ELF		:= build/$(PROGRAM).elf
MAP		:= build/$(PROGRAM).map
EXE		:= $(PROGRAM).bin
BFB		:= $(PROGRAM).bfb

# Verbose flag
# ------------

ifeq ($(V),1)
_V		:=
else
_V		:= @
endif

# Source files
# ------------

INCLUDEDIRS	:= $(SOURCEDIRS) $(BUILDDIR)

SOURCES_WFPROCESS	:= $(shell find -L assets -name "*.lua")
INCLUDEDIRS		+= $(addprefix $(BUILDDIR)/,$(ASSETDIRS))

SOURCES_S	:= $(shell find -L $(SOURCEDIRS) -name "*.s")
SOURCES_C	:= $(shell find -L $(SOURCEDIRS) -name "*.c")

# Compiler and linker flags
# -------------------------

WARNFLAGS	:= -Wall

INCLUDEFLAGS	:= $(foreach path,$(INCLUDEDIRS),-I$(path)) \
		   $(foreach path,$(LIBDIRS),-isystem $(path)/include)

LIBDIRSFLAGS	:= $(foreach path,$(LIBDIRS),-L$(path)/lib)

ASFLAGS		+= -x assembler-with-cpp $(DEFINES) $(WF_ARCH_CFLAGS) \
		   $(INCLUDEFLAGS) -ffunction-sections -fdata-sections

CFLAGS		+= -std=gnu11 $(WARNFLAGS) $(DEFINES) $(WF_ARCH_CFLAGS) \
		   $(INCLUDEFLAGS) -ffunction-sections -fdata-sections -Os

LDFLAGS		:= -Tlink.ld -Wl,--gc-sections $(LIBDIRSFLAGS) \
		   $(WF_ARCH_LDFLAGS) $(LIBS)

# Intermediate build files
# ------------------------

OBJS_ASSETS	:= $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_WFPROCESS)))

OBJS_SOURCES	:= $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_S))) \
		   $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_C)))

OBJS		:= $(OBJS_ASSETS) $(OBJS_SOURCES)

DEPS		:= $(OBJS:.o=.d)

# Targets
# -------

.PHONY: all clean

all: $(EXE) $(BFB) compile_commands.json

$(EXE): $(ELF)
	@echo "  OBJCOPY $@"
	$(_V)$(OBJCOPY) -O binary $< $@

$(BFB): $(EXE) src/bootfriend_stub.asm
	@echo "  NASM    $@"
	nasm -D$(PROGRAM) -o $(BFB) src/bootfriend_stub.asm

$(ELF): $(OBJS) link.ld
	@echo "  LD      $@"
	$(_V)$(CC) -o $(ELF) $(OBJS) $(LDFLAGS) -Wl,-Map,$(MAP)

clean:
	@echo "  CLEAN"
	$(_V)$(RM) $(EXE) $(ELF) $(BFB) $(BUILDDIR) compile_commands.json

compile_commands.json: $(OBJS) | Makefile
	@echo "  MERGE   compile_commands.json"
	$(_V)$(WF)/bin/wf-compile-commands-merge $@ $(patsubst %.o,%.cc.json,$^)

# Rules
# -----

$(BUILDDIR)/%.s.o : %.s | $(OBJS_ASSETS)
	@echo "  AS      $<"
	@$(MKDIR) -p $(@D)
	$(_V)$(CC) $(ASFLAGS) -MMD -MP -MJ $(patsubst %.o,%.cc.json,$@) -c -o $@ $<

$(BUILDDIR)/%.c.o : %.c | $(OBJS_ASSETS)
	@echo "  CC      $<"
	@$(MKDIR) -p $(@D)
	$(_V)$(CC) $(CFLAGS) -MMD -MP -MJ $(patsubst %.o,%.cc.json,$@) -c -o $@ $<

$(BUILDDIR)/%.lua.o : %.lua
	@echo "  PROCESS $<"
	@$(MKDIR) -p $(@D)
	$(_V)$(WF)/bin/wf-process -o $(BUILDDIR)/$*.c -t $(TARGET) --depfile $(BUILDDIR)/$*.lua.d --depfile-target $(BUILDDIR)/$*.lua.o $<
	$(_V)$(CC) $(CFLAGS) -MMD -MP -c -o $(BUILDDIR)/$*.lua.o $(BUILDDIR)/$*.c

# Include dependency files if they exist
# --------------------------------------

-include $(DEPS)
