# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2021 Amol Surati

ARCH := arm

F := -march=armv6zk+nofp -mno-unaligned-access -mfloat-abi=soft
F += -mtune=arm1176jzf-s

CROSS := arm-none-eabi

CFLAGS += $(F)
CPPFLAGS += $(F)

LDFLAGS += -m armelf

QFLAGS := -M raspi1ap
#QFLAGS += -trace enable=bcm*
QFLAGS += -serial mon:stdio -nographic -nodefaults -d mmu,int -kernel $(PKG)
RUN := $(HOME)/tools/qemu/bin/qemu-system-arm $(QFLAGS)
DRUN := $(RUN) -s -S
