# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Adrian "asie" Siekierka, 2023

WONDERFUL_TOOLCHAIN ?= /opt/wonderful
TARGET = wswan/bootfriend
include $(WONDERFUL_TOOLCHAIN)/target/$(TARGET)/makedefs.mk

# Metadata
# --------

NAME_BOOT	:= ipl1
NAME_SELFTEST	:= ipl1_selftest

# Source code paths
# -----------------

INCLUDEDIRS	:= src/boot src/selftest src/shared

# Defines passed to all files
# ---------------------------

DEFINES		:= -DNILESWAN_IPL1

# Libraries
# ---------

LIBS		:= -lwsx -lws
LIBDIRS		:= $(WF_TARGET_DIR)

# Build artifacts
# ---------------

BUILDDIR	:= build
ELF_BOOT	:= build/$(NAME_BOOT).elf
MAP_BOOT	:= build/$(NAME_BOOT).map
EXE_BOOT	:= $(NAME_BOOT).bin
ELF_SELFTEST	:= build/$(NAME_SELFTEST).elf
MAP_SELFTEST	:= build/$(NAME_SELFTEST).map
EXE_SELFTEST	:= $(NAME_SELFTEST).bin

# Verbose flag
# ------------

ifeq ($(V),1)
_V		:=
else
_V		:= @
endif

# Source files
# ------------

SOURCES_WFPROCESS	:= $(shell find -L assets -name "*.lua")
INCLUDEDIRS		+= $(addprefix $(BUILDDIR)/,$(ASSETDIRS))

SOURCES_SHARED		:= \
	$(shell find -L src/shared -name "*.s") \
	$(shell find -L src/shared -name "*.c")

SOURCES_BOOT		:= \
	$(shell find -L src/boot -name "*.s") \
	$(shell find -L src/boot -name "*.c")
	
SOURCES_SELFTEST	:= \
	$(shell find -L src/selftest -name "*.s") \
	$(shell find -L src/selftest -name "*.c")

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

OBJS_ASSETS	:= $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_CBIN))) \
		   $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_WFPROCESS)))

OBJS_SHARED	:= $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_SHARED)))
OBJS_BOOT	:= $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_BOOT)))
OBJS_SELFTEST	:= $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_SELFTEST)))
OBJS		:= $(OBJS_ASSETS) $(OBJS_SHARED) $(OBJS_BOOT) $(OBJS_SELFTEST)

DEPS		:= $(OBJS:.o=.d)

# Targets
# -------

.PHONY: all clean

all: $(EXE_BOOT) $(EXE_SELFTEST) compile_commands.json

program: $(EXE_BOOT) $(EXE_SELFTEST)
	iceprog -i 32 -o 128k $(EXE_BOOT)
	iceprog -i 32 -o 160k $(EXE_SELFTEST)

$(EXE_BOOT): $(ELF_BOOT)
	@echo "  OBJCOPY $@"
	$(_V)$(OBJCOPY) -O binary $< $@

$(EXE_SELFTEST): $(ELF_SELFTEST)
	@echo "  OBJCOPY $@"
	$(_V)$(OBJCOPY) -O binary $< $@

$(ELF_BOOT): $(OBJS_ASSETS) $(OBJS_SHARED) $(OBJS_BOOT) link.ld
	@echo "  LD      $@"
	$(_V)$(CC) -o $(ELF_BOOT) $(OBJS_ASSETS) $(OBJS_SHARED) $(OBJS_BOOT) $(LDFLAGS) -Wl,-Map,$(MAP_BOOT)

$(ELF_SELFTEST): $(OBJS_ASSETS) $(OBJS_SHARED) $(OBJS_SELFTEST) link.ld
	@echo "  LD      $@"
	$(_V)$(CC) -o $(ELF_SELFTEST) $(OBJS_ASSETS) $(OBJS_SHARED) $(OBJS_SELFTEST) $(LDFLAGS) -Wl,-Map,$(MAP_SELFTEST) 

clean:
	@echo "  CLEAN"
	$(_V)$(RM) $(EXE_BOOT) $(ELF_BOOT) $(EXE_SELFTEST) $(ELF_SELFTEST) $(BUILDDIR) compile_commands.json

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
