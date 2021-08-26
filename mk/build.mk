# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2021 Amol Surati

# SUB is the sub-directory to process.
SRC := $(SUB)
OBJ := $(SUB)
$(shell mkdir -p $(OBJ))

OBJS :=
SUBDIRS :=

# Default target for this makefile
all: FORCE

include $(SRC)/Makefile
include mk/inc.mk

# OBJS must not be empty. If it is, remove $(SRC)/Makefile, and the reference
# to the SUB sub-directory from the main Makefile.
ifeq ($(OBJS),)
# ifeq ($(SUBDIRS),)
$(error Invalid build configuration)
# endif
# all: $(SUBDIRS)
else
all: $(OBJ)/ar.a
endif

$(OBJ)/ar.a: $(OBJS) $(SUBDIRS)
	$(AR) -rcuTP $@ $(SUBDIR_AR_OBJS) $(AR_OBJS)

$(SUBDIRS): FORCE
	$(MAKE) $(BUILD)=$@

# make can find the prereqs because of VPATH := $(SRC_PATH)
$(OBJ)/%.S.o: $(SRC)/%.S
	$(CC) $(CFLAGS) $< -o $@

$(OBJ)/%.c.o: $(SRC)/%.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJ)/%.ld.ld: $(SRC)/%.ld
	$(CPP) $(CPPFLAGS) -MT $@ $< -o $@

FORCE:

-include $(DEPS)

.PHONY: FORCE
