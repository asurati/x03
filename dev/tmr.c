// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>

#include <sys/err.h>
#include <sys/vmm.h>

#include <dev/dev.h>

#define TMR_CS				(0 >> 2)
#define TMR_CLO				(0x4 >> 2)
#define TMR_CHI				(0x8 >> 2)

static volatile uint32_t *g_tmr_regs;

uint32_t tmr_get_ctr()
{
	return g_tmr_regs[TMR_CLO];
}

// IPL_THREAD
int tmr_init()
{
	int err;
	va_t va;

	err = dev_map_io(TIMER_BASE, 0x1000, &va);
	if (err)
		return err;
	g_tmr_regs = (volatile uint32_t *)va;
	return ERR_SUCCESS;
}
