// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>

#include <sys/err.h>
#include <sys/vmm.h>

struct tmr_regs {
	uint32_t			ctrl_status;
	uint32_t			ctr_lo;
	uint32_t			ctr_hi;
	uint32_t			cmp[4];
};

static volatile struct tmr_regs *g_tmr_regs;

uint32_t tmr_get_ctr()
{
	return g_tmr_regs->ctr_lo;
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
	g_tmr_regs = (volatile struct tmr_regs *)va;
	return ERR_SUCCESS;
err1:
	vmm_free(page, 1);
err0:
	return err;
}
