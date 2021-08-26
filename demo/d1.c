// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <dev/v3d.h>

// ori host_int, 1, 1;
// pe;
// ;
// ;

int d1_run()
{
	int err;
	static const uint32_t code[] __attribute__((aligned(8))) = {
		0x159c1fc0, 0xd00209a7,
		0x009e7000, 0x300009e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
	};
	err = v3d_run_prog(va_to_pa((va_t)code), 0, 0);
	return err;
}
