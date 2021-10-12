// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef DEV_DEV_H
#define DEV_DEV_H

#include <stddef.h>

#include <sys/mmu.h>

int	dev_map_io(pa_t pa, size_t size, va_t *out);
#endif
