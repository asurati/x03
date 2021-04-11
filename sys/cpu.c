// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>

#include <sys/cpu.h>
#include <sys/mmu.h>
#include <sys/err.h>

struct cpu {
	int				cpl;
};

struct cpu_irqh {
	fn_irq_handler			*fn;
	void				*data;
};

static struct cpu g_cpu;
static struct cpu_irqh g_cpu_irqs[NUM_IRQS];
static struct cpu_irqh g_cpu_soft_irqs[NUM_IRQS];
static unsigned long g_cpu_soft_irq_mask;
extern void excptn_vector();

int cpu_init()
{
	g_cpu.cpl = IPL_THREAD;

	// Set the NS exception base address.
	mcr_vbar((pa_t)excptn_vector);

	return ERR_SUCCESS;
}

void cpu_irq_handler_dummy()
{
	assert(0);
}

void cpu_soft_irq_handler()
{
	int i;
	unsigned long mask;
	fn_irq_handler *fn;
	void *data;

	for (;;) {
		cpu_disable_irq();
		mask = *(volatile unsigned long *)&g_cpu_soft_irq_mask;
		g_cpu_soft_irq_mask = 0;
		cpu_enable_irq();

		// To implement thread scheduling based on ticks, allow the
		// timer irqh to raise a scheduler softirq. The loop below,
		// instead of calling the softirq like it calls other softirqs,
		// will mark that a scheduler softirq needs to be called.

		// When this outer loop sees that the mask is 0 and a
		// scheduler softirq is pending, call into the softirq
		// routine. That routine may decide to switch to another
		// thread.

		// When this thread comes back again up on the cpu, the
		// processing resumes by returning from the scheduler
		// softirq, and reiterates this outerloop as before. The
		// reiteration is needed since there might be pending softirqs
		// that need to be processed; mutex/event rely on lowering
		// of the IPL, but this code is part of the IPL-lowering code
		// itself.

		// if (mask == 0 && scheduler_softirq_pending) {
		//   call scheduler_softirq;
		//   continue;
		// } else if (mask == 0) {
		//   break;
		// }

		if (mask == 0) break;

		for (i = 0; i < NUM_IRQS && mask; ++i, mask >>= 1) {
			if (!(mask & 1)) continue;
			fn = g_cpu_soft_irqs[i].fn;
			data = g_cpu_soft_irqs[i].data;
			if (fn) fn(data);
		}
	}
}

// Should be called from IPL_HARD only.
void cpu_raise_soft_irq(enum irq irq)
{
	g_cpu_soft_irq_mask |= 1ul << irq;
}

int cpu_raise_ipl(enum ipl ipl)
{
	int prev_ipl;
	struct cpu *cpu;

	cpu = &g_cpu;
	prev_ipl = cpu->cpl;

	assert(ipl < prev_ipl);

	if (ipl == IPL_SOFT) {
		assert(cpu->cpl == IPL_THREAD);
		cpu->cpl = IPL_SOFT;
		return prev_ipl;
	}

	assert(ipl == IPL_HARD);
	assert(cpu->cpl == IPL_SOFT || cpu->cpl == IPL_THREAD);

	// Raising to IPL_HARD should block all interrupts.
	cpu_disable_irq();
	cpu->cpl = IPL_HARD;
	return prev_ipl;
}

int cpu_lower_ipl(enum ipl ipl)
{
	int prev_ipl;
	struct cpu *cpu;

	cpu = &g_cpu;
	prev_ipl = cpu->cpl;

	assert(ipl > prev_ipl);

	// If lowering to IPL_SOFT, we must be at IPL_HARD. That is, the irqs
	// are in disabled state. Update (atomically) the cpl, and enable irqs.

	if (ipl == IPL_SOFT) {
		assert(cpu->cpl == IPL_HARD);
		cpu->cpl = IPL_SOFT;
		cpu_enable_irq();
		return prev_ipl;
	}

	// Lowering to IPL_THREAD.
	assert(ipl == IPL_THREAD);

	// cpl is either IPL_HARD or IPL_SOFT. In any case, we need to run
	// soft irqs.

	cpu->cpl = IPL_SOFT;
	if (prev_ipl == IPL_HARD)
		cpu_enable_irq();
	cpu_soft_irq_handler();
	cpu->cpl = IPL_THREAD;
	return prev_ipl;
}

void cpu_irq_handler()
{
	int i, prev_ipl;
	fn_irq_handler *fn;
	void *data;
	struct cpu *cpu;

	// Manually raise the IPL; irqs are already disabled.
	cpu = &g_cpu;
	prev_ipl = cpu->cpl;

	// IRQs aren't enabled at IPL_HARD.
	assert(cpu->cpl == IPL_SOFT || cpu->cpl == IPL_THREAD);

	cpu->cpl = IPL_HARD;
	for (i = 0; i < NUM_IRQS; ++i) {
		fn = g_cpu_irqs[i].fn;
		data = g_cpu_irqs[i].data;
		if (fn) fn(data);
	}

	// If the prev_ipl was IPL_SOFT, return. The rfi will enable the
	// interrupts back again, and the IPL_SOFT processing that was
	// interrupted will resume.

	if (prev_ipl == IPL_SOFT) {
		cpu->cpl = IPL_SOFT;
		return;
	}

	// If the prev_ipl was IPL_THREAD, we need to run IPL_SOFT, and then
	// return to IPL_THREAD.

	assert(prev_ipl == IPL_THREAD);
	cpu->cpl = IPL_SOFT;
	cpu_enable_irq();
	cpu_soft_irq_handler();
	cpu_disable_irq();
	cpu->cpl = IPL_THREAD;
}

void cpu_register_irqh(enum irq irq, fn_irq_handler *fn, void *data)
{
	g_cpu_irqs[irq].fn = fn;
	g_cpu_irqs[irq].data = data;
}

void cpu_register_soft_irqh(enum irq irq, fn_irq_handler *fn, void *data)
{
	g_cpu_soft_irqs[irq].fn = fn;
	g_cpu_soft_irqs[irq].data = data;
}
