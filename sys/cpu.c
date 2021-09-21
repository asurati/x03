// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>
#include <lib/string.h>

#include <sys/cpu.h>			// struct cpu
#include <sys/err.h>
#include <sys/thread.h>

static struct thread g_idle_thread;
static struct cpu g_cpu;
static uint32_t g_cpu_sw_irq_mask;

struct irq_info {
	fn_irqh *hw;
	fn_irqh *sw;
};

static struct irq_info g_irq_info[NUM_IRQS];

void cpu_hw_irq_handler()
{
	int i;
	enum ipl ipl;
	reg_t irq_mask;
	uint32_t mask;
	uint32_t	intc_get_pending();

	ipl = cpu_raise_ipl(IPL_HARD, &irq_mask);
	mask = intc_get_pending();

	for (i = 0; mask && i < NUM_IRQS; ++i) {
		if ((mask & (1 << i)) == 0)
			continue;
		g_irq_info[i].hw();
		mask >>= 1;
	}
	cpu_lower_ipl(ipl, irq_mask);
}

static
void cpu_sw_irq_handlers(uint32_t mask)
{
	int i;
	for (i = 0; mask && i < NUM_IRQS; ++i) {
		if ((mask & (1 << i)) == 0)
			continue;
		g_irq_info[i].sw();
		mask >>= 1;
	}
}

void cpu_register_irqh(enum irq irq, fn_irqh *hw, fn_irqh *sw)
{
	g_irq_info[irq].hw = hw;
	g_irq_info[irq].sw = sw;
}

// Called at IPL_HARD only
void cpu_raise_sw_irq(enum irq irq)
{
	g_cpu_sw_irq_mask |= 1ul << irq;
}

static inline
enum ipl cpu_get_curr_ipl()
{
	struct cpu *cpu;

	cpu = cpu_get();
	return cpu->curr_ipl;
}

static inline
void cpu_set_curr_ipl(enum ipl ipl)
{
	struct cpu *cpu;

	cpu = cpu_get();
	cpu->curr_ipl = ipl;
}

static inline
reg_t mrs_cpsr()
{
	reg_t val;
	__asm volatile ("mrs	%0, cpsr" : "=r"(val));
	return val;
}

static inline
void cpsie_i()
{
	__asm volatile ("cpsie	i" ::: "memory");
}

static inline
void cpsid_i()
{
	__asm volatile ("cpsid	i" ::: "memory");
}

static inline
reg_t cpu_disable_irqs()
{
	reg_t prev_mask;

	prev_mask = mrs_cpsr();
	cpsid_i();
	return prev_mask;
}

static inline
reg_t cpu_enable_irqs()
{
	reg_t prev_mask;

	prev_mask = mrs_cpsr();
	cpsie_i();
	return prev_mask;
}

static inline
reg_t cpu_get_irqs()
{
	return mrs_cpsr() & bits_on(PSR_I);
}

static inline
void cpu_set_irqs(reg_t prev_mask)
{
	if (bits_get(prev_mask, PSR_I))
		cpsid_i();
	else
		cpsie_i();
}

enum ipl cpu_raise_ipl(enum ipl new_ipl, reg_t *irq_mask)
{
	enum ipl curr_ipl;

	*irq_mask = cpu_get_irqs();
	curr_ipl = cpu_get_curr_ipl();
	assert(new_ipl <= curr_ipl);

	if (new_ipl == curr_ipl)
		return curr_ipl;

	// If moving into the HARD IPLs.
	if (new_ipl == IPL_HARD)
		cpu_disable_irqs();

	cpu_set_curr_ipl(new_ipl);
	return curr_ipl;
}

enum ipl cpu_lower_ipl(enum ipl new_ipl, reg_t irq_mask)
{
	enum ipl curr_ipl;
	uint32_t mask;

	curr_ipl = cpu_get_curr_ipl();
	assert(new_ipl >= curr_ipl);

	if (new_ipl == curr_ipl) {
		cpu_set_irqs(irq_mask);
		return curr_ipl;
	}

	// IPL_HARD to IPL_SCHED: set IPL and set irq_mask
	// IPL_HARD TO IPL_THREAD:
	// set IPL to IPL_SCHED, enable IRQs, run soft handlers, disable IRQs,
	// set IPL to IPL_THREAD, set irq_mask
	// IPL_SCHED to IPL_THREAD:
	// run soft handlers, disable IRQs, check, enable loop. then set
	// IPL and set irq_mask
	if (curr_ipl == IPL_HARD && new_ipl == IPL_SCHED) {
		cpu_set_curr_ipl(new_ipl);
		cpu_set_irqs(irq_mask);
		return curr_ipl;
	}

	cpu_set_curr_ipl(IPL_SCHED);

	// Run soft handlers.
	while (1) {
		cpu_disable_irqs();
		if (g_cpu_sw_irq_mask == 0)
			break;
		mask = g_cpu_sw_irq_mask;
		g_cpu_sw_irq_mask = 0;
		cpu_enable_irqs();
		cpu_sw_irq_handlers(mask);
	}
	cpu_set_curr_ipl(new_ipl);
	cpu_set_irqs(irq_mask);
	return curr_ipl;
}

// This call runs under the mmu maps supplied by the loader. Hence, malloc,
// mmu_map, etc. are not available to this function and its callees.
void cpu_init()
{
	struct cpu *cpu;
	void excptn_vector();

	cpu = &g_cpu;
	cpu->index = 0;
	cpu->hw_id = 0;
	cpu->curr_ipl = IPL_HARD;
	cpu->idle_thread = &g_idle_thread;
	cpu->curr_thread = cpu->idle_thread;
	list_init(&cpu->ready_queue);
	cpu_set(cpu);
	mcr_vbar((reg_t)excptn_vector);
	cpu->online = 1;
}
