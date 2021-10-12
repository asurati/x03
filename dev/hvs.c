// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>

#include <sys/err.h>
#include <sys/vmm.h>

#include <dev/dev.h>
#include <dev/con.h>

volatile uint32_t *g_hvs_regs;

// IPL_THREAD
int hvs_init()
{
	int err;
	va_t va;

	err = dev_map_io(HVS_BASE, 0x6000, &va);
	if (err)
		return err;
	g_hvs_regs = (volatile uint32_t *)va;
	return ERR_SUCCESS;
}
