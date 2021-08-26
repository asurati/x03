// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <stdint.h>

static unsigned int g_seed;
void srand(unsigned int seed)
{
	g_seed = seed;
}

int rand()
{
	g_seed = 1103515245 * g_seed + 12345;
	return g_seed;
}

uint64_t divmod(uint64_t num, uint64_t den, uint64_t *mod)
{
	uint64_t q;

	if (den == 0)
		return -1;
	q = 0;
	while (num >= den) {
		++q;
		num -= den;
	}
	if (mod)
		*mod = num;
	return q;
}
