# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2021 Amol Surati

# cd /path/to/outdir
# make -f path/to/Makefile [all|c|r|rd]
# Later, make

# Accept only 'all', 'c', 'r', 'rd' as MAKECMDGOALS
T = $(filter-out all c r rd, $(MAKECMDGOALS))
ifneq ($(T),)
$(error Unsupported goal(s))
endif

all: FORCE

ifeq ($(MAKELEVEL),0)
MAKEFLAGS += -rR
MAKEFLAGS += --no-print-directory

THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
SRC_PATH := $(realpath $(dir $(THIS_MAKEFILE)))
OBJ_PATH := $(CURDIR)

# SRCPATH and OBJPATH must be different
ifneq ($(OBJ_PATH),$(SRC_PATH))
MAKEFLAGS += --include-dir=$(SRC_PATH)
else
$(error In-tree builds not supported)
endif

VPATH := $(SRC_PATH)
export VPATH SRC_PATH

c r rd all:
	$(MAKE) -C $(OBJ_PATH) -f $(THIS_MAKEFILE) $(MAKECMDGOALS)
endif # ifeq ($(MAKELEVEL),0)







# From here on, $(CURDIR) doesn't change.
# $(CURDIR) is the location where the output files will be stored.

ifeq ($(MAKELEVEL),1)
INC := -I$(SRC_PATH)/inc
LDFLAGS += -nostdlib --whole-archive
CPPFLAGS += -P -MMD -MP -x assembler-with-cpp $(INC) -march=armv6zk	\
	    -mno-unaligned-access -mfloat-abi=soft
CFLAGS += -c -g -MMD -MP -O3 -std=c99 $(INC) -pedantic -pedantic-errors	\
	-ffreestanding -fno-common -Wall -Wextra -Werror -Wshadow	\
       	-Wpedantic -Wfatal-errors -fno-exceptions -fno-unwind-tables	\
	-fno-asynchronous-unwind-tables -fsigned-char -fno-builtin	\
	-mno-unaligned-access -mfloat-abi=soft -march=armv6zk

CROSS := arm-none-eabi
CC := $(CROSS)-gcc
LD := $(CROSS)-ld
AR := $(CROSS)-ar
OC := $(CROSS)-objcopy
CPP := $(CROSS)-cpp
RM := rm -f
export CC CPP LD AR RM CFLAGS LDFLAGS CPPFLAGS

LIBGCC := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)
SYS := sys.elf
SBN := sys.bin
SGZ := sys.bin.gz
IMG := sys.img

# Building and Cleaning commands
BUILD := -f $(SRC_PATH)/mk/build.mk SUB
CLEAN := -f $(SRC_PATH)/mk/clean.mk SUB
export BUILD CLEAN

# Do not add any directories containing no makefiles.
BUILD_DIRS := sys lib dev
BUILD_OBJS := $(addsuffix /ar.a,$(BUILD_DIRS))

BUILD_DIRS := $(sort $(BUILD_DIRS))
CLEAN_DIRS := $(addprefix _clean_,$(BUILD_DIRS))

all: OUT_MK $(BUILD_DIRS) $(IMG)

$(BUILD_DIRS): FORCE
	$(MAKE) $(BUILD)=$@

# UBoot commands to boot on the RPi HW:
# loady 200000 (upload $(IMG))
# loady 300000 (upload dtb)
# bootm 200000 - 300000

EP = $(shell xxd -l 4 -s 0x18 -e $(SYS) | cut -c11-18)

$(IMG): $(SGZ)
	mkimage -A arm -O linux -T kernel -C gzip -n $@ -a $(EP)	\
		-e $(EP) -d $< $@

$(SGZ): $(SBN)
	gzip -c $< > $@

$(SBN): $(SYS)
	$(OC) -O binary $< $@

$(SYS): sys/sys.ld.ld $(BUILD_OBJS)
	$(LD) $(LDFLAGS) -T$< $(BUILD_OBJS) --no-whole-archive $(LIBGCC) -o $@

c: $(CLEAN_DIRS)
	$(RM) $(SBN) $(SYS) $(SGZ) $(IMG)

$(CLEAN_DIRS): FORCE
	$(MAKE) $(CLEAN)=$(patsubst _clean_%,%,$@)

OUT_MK: FORCE
	if [ ! -f Makefile ]; then \
		echo "include $(SRC_PATH)/Makefile" > Makefile; \
	fi

QFLAGS := -M raspi1ap -cpu arm1176 -d int,mmu,unimp -dtb bcm2708-rpi-b.dtb
QFLAGS += -serial mon:stdio -nographic -nodefaults -kernel $(SYS)
RUN := $(HOME)/tools/qemu/bin/qemu-system-arm $(QFLAGS)
RUND := $(RUN) -s -S

r: FORCE
	$(RUN)

rd: FORCE
	$(RUND)

endif # ifeq ($(MAKELEVEL),1)

FORCE:

.PHONY: FORCE
