// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_CPU_H
#define SYS_CPU_H

#include <stdint.h>

#include <sys/bits.h>
#include <sys/list.h>
#include <sys/mmu.h>

typedef void fn_irqh();

enum irq {
	IRQ_ARM_MAILBOX,	// Bank 0, IRQ 1
	IRQ_TIMER3,		// Bank 1, IRQ 3
	IRQ_VC_3D,		// Bank 1, IRQ 10
	NUM_IRQS,
};

enum ipl {
	IPL_HARD,
	IPL_SCHED,
	IPL_THREAD,
};

#define PSR_I_POS			7
#define PSR_I_BITS			1

#define PDE_T_POS			0
#define PDE_NLTA_POS			10
#define PDE_T_BITS			1
#define PDE_NLTA_BITS			22

#define PTE_SN_POS			1
#define PTE_SN_B_POS			2
#define PTE_SN_C_POS			3
#define PTE_SN_XN_POS			4
#define PTE_SN_AP_POS			10
#define PTE_SN_TEX_POS			12
#define PTE_SN_APX_POS			15
#define PTE_SN_S_POS			16
#define PTE_SN_NG_POS			17
#define PTE_SSN_POS			18
#define PTE_SN_BASE_POS			20
#define PTE_SSN_BASE_POS		24
#define PTE_SN_BITS			1
#define PTE_SN_B_BITS			1
#define PTE_SN_C_BITS			1
#define PTE_SN_XN_BITS			1
#define PTE_SN_AP_BITS			2
#define PTE_SN_TEX_BITS			3
#define PTE_SN_APX_BITS			1
#define PTE_SN_S_BITS			1
#define PTE_SN_NG_BITS			1
#define PTE_SSN_BITS			1
#define PTE_SN_BASE_BITS		12
#define PTE_SSN_BASE_BITS		8

#define PTE_LP_POS			0
#define PTE_B_POS			2
#define PTE_C_POS			3
#define PTE_AP_POS			4
#define PTE_APX_POS			9
#define PTE_S_POS			10
#define PTE_NG_POS			11
#define PTE_TEX_POS			12
#define PTE_XN_POS			15
#define PTE_BASE_POS			16
#define PTE_LP_BITS			1
#define PTE_B_BITS			1
#define PTE_C_BITS			1
#define PTE_AP_BITS			2
#define PTE_APX_BITS			1
#define PTE_S_BITS			1
#define PTE_NG_BITS			1
#define PTE_TEX_BITS			3
#define PTE_XN_BITS			1
#define PTE_BASE_BITS			16

#define TTBR_C_POS			0
#define TTBR_S_POS			1
#define TTBR_RGN_POS			3
#define TTBR_BASE_POS			14
#define TTBR_C_BITS			1
#define TTBR_S_BITS			1
#define TTBR_RGN_BITS			2
#define TTBR_BASE_BITS			18

#define CR_M_POS			0
#define CR_A_POS			1
#define CR_C_POS			2
#define CR_W_POS			3
#define CR_Z_POS			11
#define CR_I_POS			12
#define CR_XP_POS			23
#define CR_M_BITS			1
#define CR_A_BITS			1
#define CR_C_BITS			1
#define CR_W_BITS			1
#define CR_Z_BITS			1
#define CR_I_BITS			1
#define CR_XP_BITS			1

#define AUX_CR_CZ_POS			6
#define AUX_CR_CZ_BITS			1

typedef uint32_t			reg_t;

struct thread;
struct cpu {
	struct list_head		ready_queue;
	struct list_head		entry;
	struct thread			*curr_thread;
	struct thread			*idle_thread;
	enum ipl			curr_ipl;
	reg_t				hw_id;
	char				index;
	char				online;
};

static inline
void tlbi_va(va_t va)
{
	va = align_down(va, PAGE_SIZE);
	__asm volatile ("mcr	p15, 0, %0, c8, c5, 1" :: "r"(va) : "memory");
}

static inline
void tlbi_all()
{
	__asm volatile ("mcr	p15, 0, %0, c8, c5, 0" :: "r"(0) : "memory");
	__asm volatile ("mcr	p15, 0, %0, c8, c6, 0" :: "r"(0) : "memory");
}

static inline
void ic_iall()
{
	__asm volatile ("mcr	p15, 0, %0, c7, c5, 0" :: "r"(0) : "memory");
}

static inline
void dc_iall()
{
	__asm volatile ("mcr	p15, 0, %0, c7, c6, 0" :: "r"(0) : "memory");
}

static inline
void icdc_iall()
{
	__asm volatile ("mcr	p15, 0, %0, c7, c7, 0" :: "r"(0) : "memory");
}

static inline
void _dc_civac(va_t va)
{
	__asm volatile ("mcr	p15, 0, %0, c7, c14, 1" :: "r"(va));
}

static inline
void dc_civac(void *p, size_t size)
{
	va_t start, end;

	start = (va_t)p;
	end = start + size;
	start = align_down(start, 32);
	end = align_up(end, 32);

	for (; start < end; start += 32)
		_dc_civac(start);
}

static inline
void _dc_cvac(va_t va)
{
	__asm volatile ("mcr	p15, 0, %0, c7, c10, 1" :: "r"(va));
}

static inline
void dc_cvac(void *p, size_t size)
{
	va_t start, end;

	start = (va_t)p;
	end = start + size;
	start = align_down(start, 32);
	end = align_up(end, 32);

	for (; start < end; start += 32)
		_dc_cvac(start);
}

static inline
void mcr_dacr(reg_t val)
{
	__asm volatile ("mcr	p15, 0, %0, c3, c0, 0" :: "r"(val) : "memory");
}

static inline
reg_t mrc_aux_cr()
{
	reg_t val;
	__asm volatile ("mrc	p15, 0, %0, c1, c0, 1" : "=r"(val));
	return val;
}

static inline
void mcr_aux_cr(reg_t val)
{
	__asm volatile ("mcr	p15, 0, %0, c1, c0, 1" :: "r"(val) : "memory");
}

static inline
reg_t mrc_cr()
{
	reg_t val;
	__asm volatile ("mrc	p15, 0, %0, c1, c0, 0" : "=r"(val));
	return val;
}

static inline
void mcr_cr(reg_t val)
{
	__asm volatile ("mcr	p15, 0, %0, c1, c0, 0" :: "r"(val) : "memory");
}

static inline
void mcr_ttbr0(reg_t val)
{
	__asm volatile ("mcr	p15, 0, %0, c2, c0, 0" :: "r"(val) : "memory");
}

static inline
void mcr_vbar(reg_t val)
{
	__asm volatile ("mcr	p15, 0, %0, c12, c0, 0" :: "r"(val));
}

static inline
void dmb()
{
	__asm volatile ("mcr	p15, 0, %0, c7, c10, 5" :: "r"(0) : "memory");
}

static inline
void dsb()
{
	__asm volatile ("mcr	p15, 0, %0, c7, c10, 4" :: "r"(0) : "memory");
}

static inline
void isb()
{
	__asm volatile ("mcr	p15, 0, %0, c7, c5, 4" :: "r"(0) : "memory");
	__asm volatile ("mcr	p15, 0, %0, c7, c5, 6" :: "r"(0) : "memory");
}

static inline
void cpu_set(const void *cpu)
{
	// Privileged Thread and Process ID register.
	__asm volatile ("mcr	p15, 0, %0, c13, c0, 4" :: "r"(cpu));
}

static inline
void *cpu_get()
{
	// Privileged Thread and Process ID register.
	void *cpu;
	__asm volatile ("mrc	p15, 0, %0, c13, c0, 4" : "=r"(cpu));
	return cpu;
}

static inline
void cpu_yield()
{
	__asm volatile ("wfi");
}

static inline
struct thread *cpu_get_idle_thread()
{
	struct cpu *cpu;
	cpu = cpu_get();
	return cpu->idle_thread;
}

static inline
struct list_head *cpu_get_ready_queue()
{
	struct cpu *cpu;
	cpu = cpu_get();
	return &cpu->ready_queue;
}

static inline
void cpu_set_curr_thread(struct thread *thread)
{
	struct cpu *cpu;
	cpu = cpu_get();
	cpu->curr_thread = thread;
}

static inline
struct thread *cpu_get_curr_thread()
{
	struct cpu *cpu;
	cpu = cpu_get();
	return cpu->curr_thread;
}

static inline
int cpu_get_index()
{
	struct cpu *cpu;
	cpu = cpu_get();
	return cpu->index;
}

static inline
int cpu_enable_irq(enum irq irq)
{
	int	intc_enable_irq(enum irq irq);
	return intc_enable_irq(irq);
}

enum ipl	cpu_raise_ipl(enum ipl ipl, reg_t *irq_mask);
enum ipl	cpu_lower_ipl(enum ipl ipl, reg_t irq_mask);
void		cpu_register_irqh(enum irq irq, fn_irqh *hw, fn_irqh *sw);
void		cpu_raise_sw_irq(enum irq irq);
#endif
