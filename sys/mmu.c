// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>

#include <sys/bits.h>
#include <sys/cpu.h>
#include <sys/endian.h>
#include <sys/err.h>
#include <sys/mmu.h>

#include <dev/dtb.h>

#define _16MB				(1ul << 24)

// Identity map the RAM and the IO space. Enable DCache, etc.
int mmu_init(const struct dtb *dtb)
{
	pa_t pa;
	int node, err, num_ss, ix, i, j;
	unsigned long val, t, mem_type;
	uint64_t base, size, end;
	static uint32_t reg[8];
	static pde_t g_pd0_hw[PD0_SIZE >> 2]
		__attribute__((aligned(PD0_SIZE))) = {0};

	if (dtb == NULL) return ERR_PARAM;

	// Assuming size and address cells to be 1.
	node = dtb_node(dtb, "/memory@");
	if (node < 0) return node;

	err = dtbn_prop_read(dtb, node, "reg", reg, sizeof(reg));
	if (err) return err;

	val = 0;
	val |= bits_set(PDE_TYPE, PDE_TYPE_PTE);	// SuperSection.
	val |= bits_on(PTE_SS);

	// non-secure, non-shared, global.
	val |= bits_on(PTE_S_NS);

	// Supervisor: rw, User: noaccess.
	val |= bits_on(PTE_S_AP0);

	// RAM as Normal memory
	// Outer: WB,WA
	// Inner: WB,No-WA (The cpu doesn't support WA @ l1).

	mem_type = 0;
	mem_type |= bits_set(PTE_S_TEX, 5);
	mem_type |= bits_on(PTE_C);
	mem_type |= bits_on(PTE_B);

	base = be32_to_cpu(reg[0]);
	size = be32_to_cpu(reg[1]);
	end = base + size;

	assert(base == RAM_BASE);

	base = align_down(base, _16MB);
	end = align_up(end, _16MB);
	size = end - base;

	pa = (pa_t)base;
	num_ss = size >> 24;
	ix = bits_get(pa, PD0);

	for (i = 0; i < num_ss; ++i, pa += _16MB) {
		t = val | bits_push(PTE_SS_BASE, pa) | mem_type;
		for (j = 0; j < 16; ++j)
			g_pd0_hw[ix++] = t;
	}

	pa = IO_BASE;
	val |= bits_push(PTE_SS_BASE, pa);

	// Device memory should be marked as shared.
	// Marking it as non-shared on RPi doesn't allow MMU to be turned
	// on - the CPU hangs after mcr_ctrl.

	val |= bits_on(PTE_B);
	ix = bits_get(pa, PD0);
	for (i = 0; i < 16; ++i)
		g_pd0_hw[ix++] = val;

	// Set TTBR.
	val = 0;
	val |= bits_on(TTBR_C);		// PT Walks: inner-cacheable.
	val |= bits_on(TTBR_RGN0);	// PT Walks: outer WB,WA.
	val |= bits_push(TTBR_BASE, (pa_t)g_pd0_hw);
	mcr_ttbr0(val);

	// Disable TTBR1.
	val = mrc_ttbcr();
	val |= bits_on(TTBCR_PD1);
	mcr_ttbcr(val);

	// Setup domain0 as client.
	val = mrc_dacr();
	val |= 1;
	mcr_dacr(val);

	tlbi_all();	// Invalidate all TLBs.
	dsb();		// Wait for invalidation/updates.
	isb();		// Do not sample any PT state beyond this point.

	val = mrc_ctrl();
	val |= bits_on(CTRL_M);
	val |= bits_on(CTRL_A);
	val |= bits_on(CTRL_C);
	val |= bits_on(CTRL_Z);
	val |= bits_on(CTRL_I);
	val |= bits_on(CTRL_XP);
	mcr_ctrl(val);
	dsb();
	isb();
	return ERR_SUCCESS;
}
