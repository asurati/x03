// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdlib.h>

#include <sys/bits.h>
#include <sys/err.h>
#include <sys/mmu.h>

#include "rng.h"

#define RNG_CTRL			(void *)(IO_BASE + 0x104000)
#define RNG_STATUS			(void *)(IO_BASE + 0x104004)
#define RNG_DATA			(void *)(IO_BASE + 0x104008)

#define RNG_CTRL_ENABLE_POS		0
#define RNG_CTRL_ENABLE_BITS		1

#define RNG_STATUS_NUM_WORDS_POS	24
#define RNG_STATUS_NUM_WORDS_BITS	8

int rng_read(unsigned int *buf, int num)
{
	int num_avail, i, j;
	unsigned int val;

	if (buf == NULL || num <= 0) return ERR_PARAM;

	for (i = 0; num;) {
		val = readl(RNG_STATUS);
		num_avail = bits_get(val, RNG_STATUS_NUM_WORDS);
		if (num_avail > num)
			num_avail = num;
		for (j = 0; j < num_avail; ++j)
			buf[i + j] = readl(RNG_DATA);
		i += num_avail;
		num -= num_avail;
	}
	return ERR_SUCCESS;
}

int rng_init()
{
	unsigned int val;

	val = bits_on(RNG_CTRL_ENABLE);
	writel(RNG_CTRL, val);
	return ERR_SUCCESS;
}
