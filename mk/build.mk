# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2021 Amol Surati

# SUB is the sub-directory to process.
SRC := $(SUB)
OBJ := $(SUB)
$(shell mkdir -p $(OBJ))

OBJS :=
SUBDIRS :=

# Default target for this makefile
allbuild: FORCE

include $(SRC)/Makefile
include mk/inc.mk

# OBJS must not be empty. If it is, remove $(SRC)/Makefile, and the reference
# to the SUB sub-directory from the main Makefile.
ifeq ($(OBJS),)
$(error Invalid build configuration)
#allbuild: $(SUBDIRS)
else
allbuild: $(OBJ)/ar.a
endif

$(OBJ)/ar.a: $(OBJS) $(SUBDIRS)
	if [ -f $@ ]; then $(RM) $@; fi
	$(AR) -rcsTP $@ $(SUBDIR_AR_OBJS) $(AR_OBJS)

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
