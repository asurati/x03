// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_MMU_H
#define SYS_MMU_H

// sys.ld needs VA_BASE.
#define VA_BASE				0x100000

#if !defined(__ASSEMBLER__)
#include <sys/bits.h>
#include <dev/dtb.h>

#define PAGE_SIZE_BITS			16

#define RAM_BASE			0ul
#define IO_BASE				0x20000000ul

// ILP32.
typedef unsigned long			pa_t;
typedef unsigned long			pde_t;
typedef long				pfn_t;

#define PAGE_SIZE			(1ul << PAGE_SIZE_BITS)

#define PD0_BITS			12
#define PD0_POS				20
#define PD0_SIZE			(1ul << (PD0_BITS + 2))

static inline
pa_t pa_to_bus(pa_t a)
{
	return a | 0x40000000;
}

static inline
pfn_t pa_to_pfn(pa_t a)
{
	return (pfn_t)(a >> PAGE_SIZE_BITS);
}

static inline
pa_t pfn_to_pa(pfn_t n)
{
	return ((pa_t)n) << PAGE_SIZE_BITS;
}

enum align_bits {
	ALIGN_PAGE			= PAGE_SIZE_BITS,
};

static inline
unsigned long readl(const void *addr)
{
	return *(volatile const unsigned long *)addr;
}

static inline
void writel(void *addr, unsigned long val)
{
	*(volatile unsigned long *)addr = val;
}

int	mmu_init(const struct dtb *dtb);
#endif	// __ASSEMBLER__
#endif
