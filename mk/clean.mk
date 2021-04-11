# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2021 Amol Surati

# SUB is the sub-directory to process.
SRC := $(SUB)
OBJ := $(SUB)

# default target for this makefile.
allclean: FORCE

OBJS :=
SUBDIRS :=
CLEAN_FILES :=

include $(SRC)/Makefile
include mk/inc.mk

CLEAN_FILES += $(OBJS) $(DEPS) $(OBJ)/ar.a

allclean: $(SUBDIRS)
	$(RM) $(CLEAN_FILES)

$(SUBDIRS): FORCE
	$(MAKE) $(CLEAN)=$@

FORCE:

.PHONY: FORCE
