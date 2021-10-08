// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>

#include <sys/err.h>
#include <sys/vmm.h>

#include <dev/con.h>

volatile uint32_t *g_txp_regs;

// IPL_THREAD
int txp_init()
{
	int err;
	pa_t pa;
	va_t va;
	vpn_t page;
	pfn_t frame;

	err = vmm_alloc(ALIGN_PAGE, 1, &page);
	if (err)
		goto err0;

	pa = TXP_BASE;
	frame = pa_to_pfn(pa);
	err = mmu_map_page(0, page, frame, ALIGN_PAGE, PROT_RW | ATTR_IO);
	if (err)
		goto err1;
	va = vpn_to_va(page);
	va |= pa & (PAGE_SIZE - 1);
	g_txp_regs = (volatile uint32_t *)va;
	return ERR_SUCCESS;
err1:
	vmm_free(page, 2);
err0:
	return err;
}
