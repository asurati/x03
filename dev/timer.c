// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdlib.h>

#include <sys/bits.h>
#include <sys/err.h>
#include <sys/mmu.h>

#include "intc.h"
#include "timer.h"

#define TMR_CTRL_STATUS			(void *)(IO_BASE + 0x3000)
#define TMR_CNTR_LOW			(void *)(IO_BASE + 0x3004)
#define TMR_CNTR_HIGH			(void *)(IO_BASE + 0x3008)
#define TMR_CMP3			(void *)(IO_BASE + 0x3018)

#define TMR_CS_M3_POS			3
#define TMR_CS_M3_BITS			1

// Should be called at IPL_HARD only.
static
void timer_irqh(void *p)
{
	unsigned long val;

	val = readl(TMR_CTRL_STATUS);
	if (bits_get(val, TMR_CS_M3) == 0) return;

	// Deassert the signal.
	writel(TMR_CTRL_STATUS, val);

	// Raise the soft irq.
	cpu_raise_soft_irq(IRQ_TIMER);

	// Disable further firing of the timer.
	intc_disable_irq(IRQ_TIMER);

	(void)p;
}

// Should be called at IPL_SOFT only.
static
void timer_soft_irqh(void *p)
{
	(void)p;
}

int timer_init()
{
	// The intc ensures that the timer irq starts as disabled.

	cpu_register_irqh(IRQ_TIMER, timer_irqh, NULL);
	cpu_register_soft_irqh(IRQ_TIMER, timer_soft_irqh, NULL);
	return ERR_SUCCESS;
}

void timer_set(int off)
{
	unsigned long val;

	val = readl(TMR_CNTR_LOW);
	val += off;
	writel(TMR_CMP3, val);
	intc_enable_irq(IRQ_TIMER);
}
