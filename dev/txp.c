// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>

#include <sys/err.h>
#include <sys/vmm.h>

#include <dev/dev.h>
#include <dev/con.h>

volatile uint32_t *g_txp_regs;

// IPL_THREAD
int txp_init()
{
	int err;
	va_t va;

	err = dev_map_io(TXP_BASE, 0x20, &va);
	if (err)
		return err;
	g_txp_regs = (volatile uint32_t *)va;
	return ERR_SUCCESS;
}
