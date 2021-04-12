// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdlib.h>

#include <sys/err.h>
#include <sys/mmu.h>

#include <dev/v3d.h>
#include <dev/rng.h>

extern int	d1();
extern int	d2();

int demo_run()
{
	int err;
	unsigned int seed;

	rng_read(&seed, 1);
	srand(seed);

	err = d1();
	if (err) return err;

	err = d2();
	if (err) return err;

	return ERR_SUCCESS;
}
