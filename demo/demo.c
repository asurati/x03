// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <sys/err.h>

#include <dev/con.h>

typedef int (*fn_demo_run)();
int demo0_run()
{
	int err, i;
	int	d1_run();
	int	d2_run();
	int	d3_run();
	int	d4_run();
	int	d50_run();
	int	d51_run();
	int	d52_run();
	int	d53_run();
	int	d54_run();
	int	d55_run();

	static const fn_demo_run fns[] = {
		d1_run, d2_run, d3_run, d4_run, d50_run, d51_run, d52_run,
		d53_run, d54_run, d55_run,
	};

	static const char *fn_names[] = {
		"d1", "d2", "d3", "d4", "d50", "d51", "d52", "d53", "d54",
		"d55"
	};

	for (i = 0; i < (int)(sizeof(fns)/sizeof(fns[0])); ++i) {
		err = fns[i]();
		con_out("%s: err %x", fn_names[i], err);
		if (err)
			break;
	}
	return err;
}

int demo_run(int phase)
{
	// Phase 0: The display pipeline is setup by the firmware
	// Phase 1 and beyond: The display pipeline is setup by us.
	switch (phase) {
	case 0: return demo0_run();
	default: return ERR_PARAM;
	}
}
