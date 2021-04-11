// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_CPU_H
#define SYS_CPU_H

#include <sys/bits.h>
#include <sys/thread.h>

#define CTRL_M_POS			0
#define CTRL_A_POS			1
#define CTRL_C_POS			2
#define CTRL_Z_POS			11
#define CTRL_I_POS			12
#define CTRL_V_POS			13
#define CTRL_XP_POS			23
#define CTRL_FA_POS			29
#define CTRL_M_BITS			1
#define CTRL_A_BITS			1
#define CTRL_C_BITS			1
#define CTRL_Z_BITS			1
#define CTRL_I_BITS			1
#define CTRL_V_BITS			1
#define CTRL_XP_BITS			1
#define CTRL_FA_BITS			1

#define PSR_I_POS			7
#define PSR_I_BITS			1

#define PDE_TYPE_POS			0
#define PDE_TYPE_BITS			2

#define PDE_TYPE_PTE			2

#define PTE_S_XN_POS			4
#define PTE_S_AP0_POS			10
#define PTE_S_AP1_POS			11
#define PTE_S_TEX_POS			12
#define PTE_S_APX_POS			15
#define PTE_S_SHR_POS			16
#define PTE_S_NG_POS			17
#define PTE_SS_POS			18
#define PTE_S_NS_POS			19
#define PTE_S_BASE_POS			20
#define PTE_SS_BASE_POS			24
#define PTE_S_XN_BITS			1
#define PTE_S_AP0_BITS			1
#define PTE_S_AP1_BITS			1
#define PTE_S_TEX_BITS			3
#define PTE_S_APX_BITS			1
#define PTE_S_SHR_BITS			1
#define PTE_S_NG_BITS			1
#define PTE_SS_BITS			1
#define PTE_S_NS_BITS			1
#define PTE_S_BASE_BITS			12
#define PTE_SS_BASE_BITS		8

#define PTE_B_POS			2
#define PTE_C_POS			3
#define PTE_B_BITS			1
#define PTE_C_BITS			1

#define TTBCR_N_POS			0
#define TTBCR_PD0_POS			4
#define TTBCR_PD1_POS			5
#define TTBCR_N_BITS			3
#define TTBCR_PD0_BITS			1
#define TTBCR_PD1_BITS			1

#define TTBR_C_POS			0
#define TTBR_SHR_POS			1
#define TTBR_RGN0_POS			3
#define TTBR_RGN1_POS			4
#define TTBR_BASE_POS			14
#define TTBR_C_BITS			1
#define TTBR_SHR_BITS			1
#define TTBR_RGN0_BITS			1
#define TTBR_RGN1_BITS			1
#define TTBR_BASE_BITS			18

static inline
void dsb()
{
	__asm volatile ("mcr p15, 0, %0, c7, c10, 4" :: "r"(0) : "memory");
}

static inline
void dmb()
{
	__asm volatile ("mcr p15, 0, %0, c7, c10, 5" :: "r"(0) : "memory");
}

static inline
void wfi()
{
	__asm volatile ("mcr p15, 0, %0, c7, c0, 4" :: "r"(0) : "memory");
}

static inline
void isb()
{
	__asm volatile ("mcr p15, 0, %0, c7, c5, 4" :: "r"(0) : "memory");
	__asm volatile ("mcr p15, 0, %0, c7, c5, 6" :: "r"(0) : "memory");
}

static inline
void tlbi_all()
{
	__asm volatile ("mcr p15, 0, %0, c8, c5, 0" :: "r"(0) : "memory");
}

// Clean and Invalidate DC range.
// MCRR p15,0,<End Address>,<Start Address>,c14
// 32-byte alignment
static inline
void dc_civac(volatile void *va, size_t size)
{
	unsigned long start, end;

	start = (unsigned long)va;
	end = start + size;
	start = align_down(start, 32);
	end = align_up(end, 32);
	__asm volatile ("mcrr p15, 0, %0, %1, c14"
			:: "r"(end), "r"(start) : "memory");
}

// Clean only.
static inline
void dc_cvac(volatile void *va, size_t size)
{
	unsigned long start, end;

	start = (unsigned long)va;
	end = start + size;
	start = align_down(start, 32);
	end = align_up(end, 32);
	__asm volatile ("mcrr p15, 0, %0, %1, c12"
			:: "r"(end), "r"(start) : "memory");
}

// Invalidate only.
static inline
void dc_ivac(volatile void *va, size_t size)
{
	unsigned long start, end;

	start = (unsigned long)va;
	end = start + size;
	start = align_down(start, 32);
	end = align_up(end, 32);
	__asm volatile ("mcrr p15, 0, %0, %1, c6"
			:: "r"(end), "r"(start) : "memory");
}

static inline
unsigned long mrc_ctrl()
{
	unsigned long val;
	__asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r"(val));
	return val;
}

static inline
void mcr_ctrl(unsigned long val)
{
	__asm volatile ("mcr p15, 0, %0, c1, c0, 0" :: "r"(val) : "memory");
}

static inline
void mcr_ttbr0(unsigned long val)
{
	__asm volatile ("mcr p15, 0, %0, c2, c0, 0" :: "r"(val) : "memory");
}

static inline
unsigned long mrc_ttbr0()
{
	unsigned long val;
	__asm volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r"(val));
	return val;
}

static inline
unsigned long mrc_ttbcr()
{
	unsigned long val;
	__asm volatile ("mrc p15, 0, %0, c2, c0, 2" : "=r"(val));
	return val;
}

static inline
void mcr_ttbcr(unsigned long val)
{
	__asm volatile ("mcr p15, 0, %0, c2, c0, 2" :: "r"(val) : "memory");
}

static inline
void mcr_dacr(unsigned long val)
{
	__asm volatile ("mcr p15, 0, %0, c3, c0, 0" :: "r"(val) : "memory");
}

static inline
unsigned long mrc_dacr()
{
	unsigned long val;
	__asm volatile ("mrc p15, 0, %0, c3, c0, 0" : "=r"(val));
	return val;
}

static inline
void mcr_vbar(unsigned long val)
{
	__asm volatile ("mcr p15, 0, %0, c12, c0, 0" :: "r"(val) : "memory");
}

static inline
void cpu_disable_irq()
{
	__asm volatile ("cpsid i" ::: "cc", "memory");
}

static inline
void cpu_enable_irq()
{
	__asm volatile ("cpsie i" ::: "cc", "memory");
}

static inline
unsigned long mrs_cpsr()
{
	unsigned long cpsr;
	__asm volatile ("mrs %0, cpsr\n" : "=r"(cpsr));
	return cpsr;
}

static inline
void barrier()
{
	__asm volatile ("" ::: "memory");
}

enum irq {
	IRQ_TIMER,
	IRQ_UART,
	IRQ_MAILBOX,
	IRQ_VC_3D,
	NUM_IRQS
};

typedef void (fn_irq_handler)(void *data);
enum ipl {
	IPL_HARD,
	IPL_SOFT,
	IPL_THREAD
};

int	cpu_init();
int	cpu_raise_ipl(enum ipl ipl);
int	cpu_lower_ipl(enum ipl ipl);
void	cpu_register_irqh(enum irq irq, fn_irq_handler *fn, void *data);
void	cpu_register_soft_irqh(enum irq irq, fn_irq_handler *fn, void *data);
void	cpu_raise_soft_irq(enum irq);

static inline
void cpu_disable_preemption()
{
	cpu_raise_ipl(IPL_SOFT);
	barrier();
}

static inline
void cpu_enable_preemption()
{
	barrier();
	cpu_lower_ipl(IPL_THREAD);
}
#endif
