// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>

#include <sys/err.h>
#include <sys/vmm.h>

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
	pa_t pa;
	va_t va;
	vpn_t page;
	pfn_t frame;

	pa = TIMER_BASE;
	frame = pa_to_pfn(pa);
	err = vmm_alloc(ALIGN_PAGE, 1, &page);
	if (err)
		goto err0;
	err = mmu_map_page(0, page, frame, ALIGN_PAGE, PROT_RW | ATTR_IO);
	if (err)
		goto err1;
	va = vpn_to_va(page);
	va += pa & (PAGE_SIZE - 1);
	g_tmr_regs = (volatile uint32_t *)va;
	return ERR_SUCCESS;
err1:
	vmm_free(page, 1);
err0:
	return err;
}
