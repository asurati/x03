// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <sys/err.h>
#include <sys/mmu.h>

#include <dev/v3d.h>

extern int	d1();

int demo_run()
{
	int err;

	err = d1();
	if (err) return err;

	return ERR_SUCCESS;
}
