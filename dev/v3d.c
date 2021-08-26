// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>
#include <lib/stdlib.h>

#include <sys/err.h>
#include <sys/vmm.h>
#include <sys/spinlock.h>

#include <dev/con.h>
#include <dev/ioq.h>
#include <dev/mbox.h>
#include <dev/v3d.h>

struct v3d_run_prog {
	uint32_t			srqcs;
	ba_t				code_ba;
	ba_t				unif_ba;
	size_t				unif_size;
};

static volatile uint32_t *g_v3d_regs;
static struct ioq g_v3d_ioq;

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

	if (intctl || dbqitc)
		cpu_raise_sw_irq(IRQ_VC_3D);
}

// IPL_SCHED
static
void v3d_sw_irqh()
{
	ioq_complete_ior(&g_v3d_ioq);
}

// IPL_THREAD, or IPL_SCHED
static
int v3d_ioq_req(struct ior *ior)
{
	struct v3d_run_prog *rp;

	rp = ior_param(ior);
	if (rp->unif_size) {
		g_v3d_regs[V3D_SRQUA] = rp->unif_ba;
		g_v3d_regs[V3D_SRQUL] = rp->unif_size;
	}
	rp->srqcs = g_v3d_regs[V3D_SRQCS];
	g_v3d_regs[V3D_SRQPC] = rp->code_ba;
	return ERR_SUCCESS;
}

// IPL_SCHED
static
int v3d_ioq_res(struct ior *ior)
{
	int pdone, cdone;
	uint32_t srqcs;
	struct v3d_run_prog *rp;

	rp = ior_param(ior);
	srqcs = g_v3d_regs[V3D_SRQCS];
	pdone = bits_get(rp->srqcs, V3D_SRQCS_NUM_DONE);
	cdone = bits_get(srqcs, V3D_SRQCS_NUM_DONE);
	if (cdone > pdone || (pdone == 0xff && cdone == 0))
		return ERR_SUCCESS;
	return ERR_FAILED;
}

// IPL_THREAD
int v3d_run_prog(pa_t code_pa, pa_t unif_pa, size_t unif_size)
{
	int err;
	struct ior ior;
	struct v3d_run_prog *rp;

	rp = malloc(sizeof(*rp));
	if (rp == NULL)
		return ERR_NO_MEM;
	rp->code_ba = pa_to_ba(code_pa);
	rp->unif_ba = pa_to_ba(unif_pa);
	rp->unif_size = unif_size;

	ior_init(&ior, &g_v3d_ioq, 0, rp, 0);
	err = ioq_queue_ior(&ior);
	if (!err)
		err = ior_wait(&ior);
	free(rp);
	return err;
}

// IPL_THREAD
int v3d_init()
{
	int err;
	pa_t pa;
	va_t va;
	vpn_t page;
	pfn_t frame;

	ioq_init(&g_v3d_ioq, v3d_ioq_req, v3d_ioq_res);

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
		
	cpu_register_irqh(IRQ_VC_3D, v3d_hw_irqh, v3d_sw_irqh);

	// Allow QPU to interrupt the host.
	g_v3d_regs[V3D_DBCFG] |= bits_on(V3D_DBCFG_QITENA);

	// Clear any interrupts.
	g_v3d_regs[V3D_DBQITC] |= 0xffff;	// 16 QPUS.

	// Allow all QPUs to generate interrupts.
	g_v3d_regs[V3D_DBQITE] |= 0xffff;	// 16 QPUS.

	// Disable GPU pipeline interrupts.
	g_v3d_regs[V3D_INTDIS] |= 0xf;

	// From QPU's PoV, VPM is a 64x16x4 = 4KB memory region.
	// Enable VPM for user programs.
	g_v3d_regs[V3D_VPMBASE] = 16;	// In the unit of 256 bytes.

	cpu_enable_irq(IRQ_VC_3D);
	return ERR_SUCCESS;
err2:
	mmu_unmap_page(0, page);
err1:
	vmm_free(page, 1);
err0:
	return err;
}
