// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <dev/con.h>

int demo_run()
{
	int err;
	int	d1_run();
	int	d2_run();
	int	d3_run();
	int	d4_run();
	int	d5_run();

#if 0
	err = d1_run();
	con_out("d1: err %x", err);
	if (err)
		return err;

	err = d2_run();
	con_out("d2: err %x", err);
	if (err)
		return err;

	err = d3_run();
	con_out("d3: err %x", err);
	if (err)
		return err;

	err = d4_run();
	con_out("d4: err %x", err);
	if (err)
		return err;
#endif
	err = d5_run();
	con_out("d5: err %x", err);
	if (err)
		return err;
	return err;
}
