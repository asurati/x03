// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_BITMAP_H
#define SYS_BITMAP_H

#include <stdint.h>

struct bitmap {
	uint64_t			*buf;
	int				num_bits;
};

int	bitmap_init(struct bitmap *map, void *buf, int num_bits);
int	bitmap_on(struct bitmap *map, int start, int num_bits);
int	bitmap_off(struct bitmap *map, int start, int num_bits);
int	bitmap_is_on(const struct bitmap *map, int start, int num_bits);
int	bitmap_is_off(const struct bitmap *map, int start, int num_bits);
int	bitmap_find_off(const struct bitmap *map, int align, int start,
			int num_bits);
#endif
