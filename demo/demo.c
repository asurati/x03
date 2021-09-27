// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <dev/con.h>

#define NUM_DEMOS			7

int demo_run()
{
	typedef int (*fn_demo_run)();
	int err, i;
	int	d1_run();
	int	d2_run();
	int	d3_run();
	int	d4_run();
	int	d50_run();
	int	d51_run();
	int	d52_run();

	static const fn_demo_run fns[] = {
		d1_run, d2_run, d3_run, d4_run, d50_run, d51_run, d52_run,
	};

	static const char *fn_names[] = {
		"d1", "d2", "d3", "d4", "d50", "d51", "d52",
	};

	for (i = 0; i < NUM_DEMOS; ++i) {
		err = fns[i]();
		con_out("%s: err %x", fn_names[i], err);
		if (err)
			break;
	}
	return err;
}
