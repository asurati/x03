# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2021 Amol Surati

# SUB is the directory to process.
SRC := $(SUB)
OBJ := $(SUB)

# default target for this makefile.
all: FORCE

OBJS :=
SUBDIRS :=
CLEAN_FILES :=

include $(SRC)/Makefile
include mk/inc.mk

CLEAN_FILES += $(OBJS) $(DEPS) $(OBJ)/ar.a

all: $(SUBDIRS)
	$(RM) $(CLEAN_FILES)

$(SUBDIRS): FORCE
	$(MAKE) $(CLEAN)=$@

FORCE:

.PHONY: FORCE
