// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>
#include <lib/stdlib.h>

#include <sys/err.h>
#include <sys/cpu.h>
#include <sys/vmm.h>
#include <sys/pmm.h>

struct intc_regs {
	uint32_t			irq0_pending;
	uint32_t			irq1_pending;
	uint32_t			irq2_pending;
	uint32_t			fiq_ctrl;
	uint32_t			irq1_enable;
	uint32_t			irq2_enable;
	uint32_t			irq0_enable;
	uint32_t			irq1_disable;
	uint32_t			irq2_disable;
	uint32_t			irq0_disable;
};

static volatile struct intc_regs *g_intc_regs;

struct irq_info {
	int				reg_enable;
	int				reg_disable;
	int				reg_pending;
	uint32_t			mask;
};

static struct irq_info g_irqs[NUM_IRQS] = {
	[IRQ_TIMER3] = {
		offsetof(struct intc_regs, irq1_enable) / 4,
		offsetof(struct intc_regs, irq1_disable) / 4,
		offsetof(struct intc_regs, irq1_pending) / 4,
		1ul << 3
	},

	[IRQ_VC_3D] = {
		offsetof(struct intc_regs, irq1_enable) / 4,
		offsetof(struct intc_regs, irq1_disable) / 4,
		offsetof(struct intc_regs, irq1_pending) / 4,
		1ul << 10
	},

	[IRQ_ARM_MAILBOX] = {
		offsetof(struct intc_regs, irq0_enable) / 4,
		offsetof(struct intc_regs, irq0_disable) / 4,
		offsetof(struct intc_regs, irq0_pending) / 4,
		1ul << 1
	}
};

int intc_init()
{
	int err;
	pa_t pa;
	va_t va;
	vpn_t page;
	pfn_t frame;

	pa = INTC_BASE;

	err = vmm_alloc(ALIGN_PAGE, 1, &page);
	if (err)
		goto err0;
	frame = pa_to_pfn(pa);
	err = mmu_map_page(0, page, frame, ALIGN_PAGE, PROT_RW | ATTR_IO);
	if (err)
		goto err1;
	va = vpn_to_va(page);
	va += pa & (PAGE_SIZE - 1);
	g_intc_regs = (volatile struct intc_regs *)va;

	// Disable FIQ generation.
	g_intc_regs->fiq_ctrl = 0;

	// Disable all IRQs
	g_intc_regs->irq0_disable = -1;
	g_intc_regs->irq1_disable = -1;
	g_intc_regs->irq2_disable = -1;
	return ERR_SUCCESS;
err1:
	vmm_free(page, 1);
err0:
	return err;
}

static
int intc_enable_disable_irq(enum irq irq, char is_enable)
{
	struct irq_info *ii;
	volatile uint32_t *regs;

	ii = &g_irqs[irq];
	regs = (volatile uint32_t *)g_intc_regs;
	if (is_enable)
		regs[ii->reg_enable] |= ii->mask;
	else
		regs[ii->reg_disable] |= ii->mask;
	return ERR_SUCCESS;
}

int intc_enable_irq(enum irq irq)
{
	return intc_enable_disable_irq(irq, 1);
}

int intc_disable_irq(enum irq irq)
{
	return intc_enable_disable_irq(irq, 0);
}

uint32_t intc_get_pending()
{
	int i;
	uint32_t mask;
	struct irq_info *ii;
	volatile uint32_t *regs;

	regs = (volatile uint32_t *)g_intc_regs;
	mask = 0;

	for (i = 0; i < NUM_IRQS; ++i) {
		ii = &g_irqs[i];
		if (regs[ii->reg_pending] & ii->mask)
			mask |= 1ul << i;
	}
	return mask;
}
