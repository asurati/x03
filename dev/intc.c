// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdlib.h>

#include <sys/bits.h>
#include <sys/cpu.h>
#include <sys/err.h>
#include <sys/mmu.h>

// IRQ# == irq belonging to bank #.
#define IRQ0_PENDING			(void *)(IO_BASE + 0xb200)
#define IRQ1_PENDING			(void *)(IO_BASE + 0xb204)
#define IRQ2_PENDING			(void *)(IO_BASE + 0xb208)
#define FIQ_CTRL			(void *)(IO_BASE + 0xb20c)
#define IRQ1_ENABLE			(void *)(IO_BASE + 0xb210)
#define IRQ2_ENABLE			(void *)(IO_BASE + 0xb214)
#define IRQ0_ENABLE			(void *)(IO_BASE + 0xb218)
#define IRQ1_DISABLE			(void *)(IO_BASE + 0xb21c)
#define IRQ2_DISABLE			(void *)(IO_BASE + 0xb220)
#define IRQ0_DISABLE			(void *)(IO_BASE + 0xb224)

// Bank0 interrupts we need.
#define IRQ0_ARM_MAILBOX_POS		1
#define IRQ0_ARM_MAILBOX_BITS		1

// Bank1 interrupts we need.
#define IRQ1_TIMER3_POS			3
#define IRQ1_VC_3D_POS			10
#define IRQ1_TIMER3_BITS		1
#define IRQ1_VC_3D_BITS			1

// Bank2 interrupts we need.
#define IRQ2_VC_UART_POS		25
#define IRQ2_VC_UART_BITS		1

struct irq_info {
	void				*const enable_addr;
	void				*const disable_addr;
	unsigned long			mask;
};

// In the order defined by enum irq.
static const struct irq_info g_irq_info[NUM_IRQS] = {
	{IRQ1_ENABLE, IRQ1_DISABLE, bits_on(IRQ1_TIMER3)},
	{IRQ2_ENABLE, IRQ2_DISABLE, bits_on(IRQ2_VC_UART)},
	{IRQ0_ENABLE, IRQ0_DISABLE, bits_on(IRQ0_ARM_MAILBOX)},
	{IRQ1_ENABLE, IRQ1_DISABLE, bits_on(IRQ1_VC_3D)},
};

static
void intc_enable_disable_irq(enum irq irq, int is_enable)
{
	void *addr;
	unsigned long val;

	if (is_enable)
		addr = g_irq_info[irq].enable_addr;
	else
		addr = g_irq_info[irq].disable_addr;

	val = readl(addr);
	val |= g_irq_info[irq].mask;
	writel(addr, val);
}

void intc_disable_irq(enum irq irq)
{
	intc_enable_disable_irq(irq, 0);
}

void intc_enable_irq(enum irq irq)
{
	intc_enable_disable_irq(irq, 1);
}

int intc_init()
{
	// Disable FIQ.
	writel(FIQ_CTRL, 0);

	// Disable the timer, uart, v3d, and mailbox interrupts.
	intc_disable_irq(IRQ_TIMER);
	intc_disable_irq(IRQ_MAILBOX);
	intc_disable_irq(IRQ_UART);
	intc_disable_irq(IRQ_VC_3D);
	return ERR_SUCCESS;
}
