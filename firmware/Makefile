# See
# https://github.com/libopencm3/libopencm3-template/blob/master/my-project/Makefile
# https://github.com/libopencm3/libopencm3-template/blob/master/LICENSE

PROJECT = nileswan_firmware
BUILD_DIR = build

CFILES = src/main.c

DEVICE=stm32l052K8
#OOCD_FILE = board/stm32f4discovery.cfg

# You shouldn't have to edit anything below here.
VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
OPENCM3_DIR=libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk

program: $(PROJECT).bin
	iceprog -i 4 -o 192k $(PROJECT).bin
