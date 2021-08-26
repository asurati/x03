// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <dev/con.h>

int demo_run()
{
	int err;
	int	d1_run();
	int	d2_run();

	err = d1_run();
	con_out("d1: err %x", err);
	if (err)
		return err;

	err = d2_run();
	con_out("d2: err %x", err);
	if (err)
		return err;
	return err;
}
