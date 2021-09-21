// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>
#include <lib/stdlib.h>

#include <sys/err.h>
#include <sys/cpu.h>
#include <sys/vmm.h>
#include <sys/pmm.h>

#define INTC_IRQ0_PENDING		(0 >> 2)
#define INTC_IRQ1_PENDING		(0x4 >> 2)
#define INTC_IRQ2_PENDING		(0x8 >> 2)
#define INTC_FIQ_CTRL			(0xc >> 2)
#define INTC_IRQ1_ENABLE		(0x10 >> 2)
#define INTC_IRQ2_ENABLE		(0x14 >> 2)
#define INTC_IRQ0_ENABLE		(0x18 >> 2)
#define INTC_IRQ1_DISABLE		(0x1c >> 2)
#define INTC_IRQ2_DISABLE		(0x20 >> 2)
#define INTC_IRQ0_DISABLE		(0x24 >> 2)

static volatile uint32_t *g_intc_regs;

struct irq_info {
	int				reg_enable;
	int				reg_disable;
	int				reg_pending;
	uint32_t			mask;
};

static struct irq_info g_irqs[NUM_IRQS] = {
	[IRQ_TIMER3] = {
		INTC_IRQ1_ENABLE,
		INTC_IRQ1_DISABLE,
		INTC_IRQ1_PENDING,
		1ul << 3
	},

	[IRQ_VC_3D] = {
		INTC_IRQ1_ENABLE,
		INTC_IRQ1_DISABLE,
		INTC_IRQ1_PENDING,
		1ul << 10
	},

	[IRQ_ARM_MAILBOX] = {
		INTC_IRQ0_ENABLE,
		INTC_IRQ0_DISABLE,
		INTC_IRQ0_PENDING,
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
	g_intc_regs = (volatile uint32_t *)va;

	// Disable FIQ generation.
	g_intc_regs[INTC_FIQ_CTRL] = 0;

	// Disable all IRQs
	g_intc_regs[INTC_IRQ0_DISABLE] = -1;
	g_intc_regs[INTC_IRQ1_DISABLE] = -1;
	g_intc_regs[INTC_IRQ2_DISABLE] = -1;
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

	ii = &g_irqs[irq];
	if (is_enable)
		g_intc_regs[ii->reg_enable] |= ii->mask;
	else
		g_intc_regs[ii->reg_disable] |= ii->mask;
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

	mask = 0;

	for (i = 0; i < NUM_IRQS; ++i) {
		ii = &g_irqs[i];
		if (g_intc_regs[ii->reg_pending] & ii->mask)
			mask |= 1ul << i;
	}
	return mask;
}
