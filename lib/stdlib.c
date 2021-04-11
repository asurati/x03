// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

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
