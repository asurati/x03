# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2021 Amol Surati

# cd /path/to/outdir
# make -f path/to/Makefile [all|c|r|rd]
# Later, make

# Accept only 'all', 'c', 'r', 'rd', 'd' as MAKECMDGOALS
T = $(filter-out all c r rd d,$(MAKECMDGOALS))
ifneq ($(T),)
$(error Unsupported goal(s))
endif

all:

ifeq ($(MAKELEVEL),0)
MAKEFLAGS += -rR
MAKEFLAGS += --no-print-directory

THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
SRC_PATH := $(realpath $(dir $(THIS_MAKEFILE)))

# CURDIR is the path where object files are built.
# SRCPATH and CURDIR must be different.
ifneq ($(CURDIR),$(SRC_PATH))
MAKEFLAGS += --include-dir=$(SRC_PATH)
else
$(error In-tree builds not supported)
endif

VPATH := $(SRC_PATH)
export VPATH SRC_PATH

c r rd d all: FORCE
	$(MAKE) -f $(THIS_MAKEFILE) $(MAKECMDGOALS)
endif # ifeq ($(MAKELEVEL),0)







# From here on, $(CURDIR) doesn't change.
# $(CURDIR) is the location where the output files will be stored.

ifeq ($(MAKELEVEL),1)

PKG := pkg.bin
MACH := rpi1b

LDFLAGS :=
CPPFLAGS :=
CFLAGS :=

include mk/$(MACH).mk

ifeq ($(ARCH),)
$(error ARCH not set)
endif

ifeq ($(CROSS),)
$(error CROSS not set)
endif

INC := -I $(SRC_PATH)/inc

CC := $(CROSS)-gcc
LD := $(CROSS)-ld
AR := $(CROSS)-ar
OC := $(CROSS)-objcopy
OD := $(CROSS)-objdump
CPP := $(CROSS)-cpp
RM := rm -f

LDFLAGS += -nostdlib --whole-archive
CPPFLAGS += -P -MMD -MP -x assembler-with-cpp $(INC)
CFLAGS += -c -g -MMD -MP -O3 -std=c99 $(INC) -pedantic -pedantic-errors	\
	-ffreestanding -fno-common -Wall -Wextra -Werror -Wshadow	\
       	-Wpedantic -Wfatal-errors -fno-exceptions -fno-unwind-tables	\
	-fno-asynchronous-unwind-tables -fsigned-char -fno-builtin

export CC CPP LD AR RM CFLAGS CPPFLAGS

# These two for booting with uboot, mostly on hw devices.
PKZ := pkg.gz
PKI := pkg.img

SYS := sys.elf
LDR := ldr.elf
LDB := ldr.bin

# Building and Cleaning commands
BUILD := -f $(SRC_PATH)/mk/build.mk SUB
CLEAN := -f $(SRC_PATH)/mk/clean.mk SUB
export BUILD CLEAN

# Do not add any directories containing no makefiles.

LDR_DIRS := ldr
SYS_DIRS := sys lib dev

BUILD_DIRS := $(LDR_DIRS) $(SYS_DIRS)

SYS_DIRS := $(sort $(SYS_DIRS))
LDR_DIRS := $(sort $(LDR_DIRS))
BUILD_DIRS := $(sort $(BUILD_DIRS))
CLEAN_DIRS := $(addprefix _clean_,$(BUILD_DIRS))

LDR_OBJS := $(addsuffix /ar.a,$(LDR_DIRS))
SYS_OBJS := $(addsuffix /ar.a,$(SYS_DIRS))




EP = $(shell xxd -l 4 -s 0x18 -g 4 -e $(LDR) | cut -c11-18)
all: OUT_MK $(BUILD_DIRS) $(PKI)

$(BUILD_DIRS): FORCE
	$(MAKE) $(BUILD)=$@

$(PKI): $(PKZ)
	mkimage -A arm -O linux -T kernel -C gzip -n $@ -a $(EP)	\
		-e $(EP) -d $< $@

$(PKZ): $(PKG)
	gzip -c $< > $@

$(PKG): $(LDB) $(SYS)
	cat $^ > $@

$(LDB): $(LDR)
	$(OC) -O binary $< $@

$(LDR): ldr/ldr.ld.ld $(LDR_OBJS)
	$(LD) $(LDFLAGS) -T $< $(LDR_OBJS) -o $@

$(SYS): sys/sys.ld.ld $(SYS_OBJS)
	$(LD) $(LDFLAGS) -T $< $(SYS_OBJS) $(LDABIFLAGS) -o $@

c: $(CLEAN_DIRS)
	$(RM) $(PKG) $(SYS) $(LDR) $(LDB) $(PKI) $(PKZ)

$(CLEAN_DIRS): FORCE
	$(MAKE) $(CLEAN)=$(patsubst _clean_%,%,$@)

OUT_MK: FORCE
	if [ ! -f Makefile ]; then \
		echo "include $(SRC_PATH)/Makefile" > Makefile; \
	fi

r: FORCE
	$(RUN)

rd: FORCE
	$(DRUN)

d: FORCE
	$(OD) -d sys.elf > ds.txt
	$(OD) -d ldr.elf > dl.txt
endif # ifeq ($(MAKELEVEL),1)

FORCE:

.PHONY: FORCE
