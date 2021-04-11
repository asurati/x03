# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2021 Amol Surati

# If OBJS is empty, do not create the ar.a at this level.
ifneq ($(OBJS),)
OBJS := $(sort $(OBJS))
OBJS := $(addprefix $(OBJ)/,$(OBJS))
# Actual objects, not containing .ld
# Do not add .ld objects into the archives
AR_OBJS := $(filter-out %.ld,$(OBJS))
DEPS := $(addsuffix .d,$(basename $(OBJS)))
endif

ifneq ($(SUBDIRS),)
SUBDIRS := $(sort $(SUBDIRS))
SUBDIRS := $(addprefix $(OBJ)/,$(SUBDIRS))
SUBDIR_AR_OBJS := $(addsuffix /ar.a,$(SUBDIRS))
endif
