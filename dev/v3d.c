// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>
#include <lib/stdlib.h>

#include <sys/err.h>
#include <sys/cpu.h>
#include <sys/vmm.h>

#include <dev/con.h>
#include <dev/mbox.h>
#include <dev/v3d.h>

static volatile uint32_t *g_v3d_regs;

// IPL_HARD
static
void v3d_hw_irqh()
{
	uint32_t intctl, dbqitc;

	intctl = g_v3d_regs[V3D_INTCTL];
	dbqitc = g_v3d_regs[V3D_DBQITC];
	con_out("v3dirqh: intctl %x, dbqitc %x", intctl, dbqitc);

	// Deassert the signals
	if (intctl)
		g_v3d_regs[V3D_INTCTL] = intctl;
	if (dbqitc)
		g_v3d_regs[V3D_DBQITC] = dbqitc;
}

// IPL_THREAD
int v3d_run_prog(ba_t code_ba, ba_t unif_ba, size_t unif_size)
{
	int srqcs;
	int pdone, cdone;

	if (unif_size) {
		g_v3d_regs[V3D_SRQUA] = unif_ba;
		g_v3d_regs[V3D_SRQUL] = unif_size;
	}
	srqcs = g_v3d_regs[V3D_SRQCS];
	pdone = bits_get(srqcs, V3D_SRQCS_NUM_DONE);
	g_v3d_regs[V3D_SRQPC] = code_ba;

	for (;;) {
		srqcs = g_v3d_regs[V3D_SRQCS];
		cdone = bits_get(srqcs, V3D_SRQCS_NUM_DONE);
		if (cdone != pdone)
			break;
	}
	return ERR_SUCCESS;
}

void v3d_run_renderer(ba_t cr, size_t size)
{
	g_v3d_regs[V3D_CT1CS] = 1ul << 15;
	g_v3d_regs[V3D_CT1CA] = cr;
	g_v3d_regs[V3D_CT1EA] = cr + size;

	for (;;) {
		if (g_v3d_regs[V3D_CT1CS] & (1ul << 5))
			continue;
		break;
	}
#if 0
	con_out("errstat %x", g_v3d_regs[V3D_ERRSTAT]);
	con_out("dbge %x", g_v3d_regs[V3D_DBGE]);
	con_out("ct1cs %x", g_v3d_regs[V3D_CT1CS]);
	con_out("ct1ca %x", g_v3d_regs[V3D_CT1CA]);
	con_out("ct1ea %x", g_v3d_regs[V3D_CT1EA]);
	con_out("ct1lc %x", g_v3d_regs[V3D_CT1LC]);
	con_out("ct1pc %x", g_v3d_regs[V3D_CT1PC]);
	con_out("pcs %x", g_v3d_regs[V3D_PCS]);
	con_out("bfc %x", g_v3d_regs[V3D_BFC]);
	con_out("rfc %x", g_v3d_regs[V3D_RFC]);
	con_out("bpca %x", g_v3d_regs[V3D_BPCA]);
	con_out("bpcs %x", g_v3d_regs[V3D_BPCS]);
#endif
}

void v3d_run_binner(ba_t cr, size_t size)
{
	g_v3d_regs[V3D_CT0CS] = 1ul << 15;
	g_v3d_regs[V3D_CT0CA] = cr;
	g_v3d_regs[V3D_CT0EA] = cr + size;

	for (;;) {
		if (g_v3d_regs[V3D_CT0CS] & (1ul << 5))
			continue;
		break;
	}
	con_out("errstat %x", g_v3d_regs[V3D_ERRSTAT]);
	con_out("dbge %x", g_v3d_regs[V3D_DBGE]);
	con_out("ct0cs %x", g_v3d_regs[V3D_CT0CS]);
	con_out("ct0ca %x", g_v3d_regs[V3D_CT0CA]);
	con_out("ct0ea %x", g_v3d_regs[V3D_CT0EA]);
	con_out("ct0lc %x", g_v3d_regs[V3D_CT0LC]);
	con_out("ct0pc %x", g_v3d_regs[V3D_CT0PC]);
	con_out("pcs %x", g_v3d_regs[V3D_PCS]);
	con_out("bfc %x", g_v3d_regs[V3D_BFC]);
	con_out("rfc %x", g_v3d_regs[V3D_RFC]);
	con_out("bpca %x", g_v3d_regs[V3D_BPCA]);
	con_out("bpcs %x", g_v3d_regs[V3D_BPCS]);
}

// IPL_THREAD
int v3d_init()
{
	int err;
	pa_t pa;
	va_t va;
	vpn_t page;
	pfn_t frame;

	err = mbox_set_dom_state(11, 1);
	if (err)
		goto err0;

	pa = V3D_BASE;
	frame = pa_to_pfn(pa);
	err = vmm_alloc(ALIGN_PAGE, 1, &page);
	if (err)
		goto err0;
	err = mmu_map_page(0, page, frame, ALIGN_PAGE, PROT_RW | ATTR_IO);
	if (err)
		goto err1;
	va = vpn_to_va(page);
	va += pa & (PAGE_SIZE - 1);
	g_v3d_regs = (volatile uint32_t *)va;

	err = ERR_INVALID;
	if (g_v3d_regs[V3D_IDENT0] != 0x2443356)
		goto err2;

	cpu_register_irqh(IRQ_VC_3D, v3d_hw_irqh, NULL);

	// Allow QPU to interrupt the host.
	g_v3d_regs[V3D_DBCFG] |= bits_on(V3D_DBCFG_QITENA);

	// Clear any interrupts.
	g_v3d_regs[V3D_DBQITC] |= 0xffff;	// 16 QPUS.

	// Allow all QPUs to generate interrupts.
	g_v3d_regs[V3D_DBQITE] |= 0xffff;	// 16 QPUS.

	// Enable binning/rendering done interrupts.
	g_v3d_regs[V3D_INTENA] |= 0x3;

	// From QPU's PoV, VPM is a 64x16x4 = 4KB memory region.
	// Enable VPM for user programs.
	//g_v3d_regs[V3D_VPMBASE] = 16;	// In the unit of 256 bytes.

	cpu_enable_irq(IRQ_VC_3D);
	return ERR_SUCCESS;
err2:
	mmu_unmap_page(0, page);
err1:
	vmm_free(page, 1);
err0:
	return err;
}
