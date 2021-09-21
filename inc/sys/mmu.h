// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_MMU_H
#define SYS_MMU_H

#include <sys/mmu.S.h>

#include <stdint.h>

#include <sys/elf.h>

typedef uint32_t			pa_t;
typedef uint32_t			va_t;
typedef uint32_t			ba_t;
typedef uint32_t			pde_t;
typedef int32_t				vpn_t;
typedef int32_t				pfn_t;

#define VA_BASE				((va_t)ASM_VA_BASE)
#define RAM_MAP_BASE			(VA_BASE + (1ul << 29))
#define SLABS_BASE			(RAM_MAP_BASE + (1ul << 29))
#define VMM_BASE			(SLABS_BASE + (1ul << 29))

// See ASM_VA_BASE
#define VA_SIZE_BITS			31
#define VA_SIZE				(1ul << VA_SIZE_BITS)
#define SLABS_SIZE			(1ul << 29)
#define VMM_SIZE			(1ul << 29)

#define SYS_BASE			((va_t)ASM_SYS_BASE)
#define LDR_BASE			((pa_t)ASM_LDR_BASE)
#define RAM_BASE			((pa_t)ASM_RAM_BASE)

// Fix the RAM size to 128 MB for now.
#define RAM_SIZE			(1ul << 27)

#define PD0_BITS			12
#define PD1_BITS			8

#define PD1_POS				12
#define PD0_POS				(PD1_POS + PD1_BITS)	// 20

#define PD0_SIZE			(1ul << (PD0_BITS + 2))
#define PD1_SIZE			(1ul << (PD1_BITS + 2))

// 16MB, 1MB, 64KB

// Fields in page dir/table entries.
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

// Device base PAs.
#define IO_BASE				0x20000000ul
#define UART_BASE			(IO_BASE + 0x201000)
#define INTC_BASE			(IO_BASE + 0xb200)
#define MBOX_BASE			(IO_BASE + 0xb880)
#define V3D_BASE			(IO_BASE + 0xc00000)
#define TIMER_BASE			(IO_BASE + 0x3000)

#define PAGE_SIZE			(1ul << PAGE_SIZE_BITS)

enum align_bits {
	ALIGN_PAGE			= PAGE_SIZE_BITS,
	ALIGN_1MB			= 20,
	ALIGN_16MB			= 24,
};

#define PROT_R				PF_R
#define PROT_W				PF_W
#define PROT_X				PF_X
#define PROT_RW				(PROT_R | PROT_W)

#define ATTR_IO				(1 << 3)

// These two VA <-> PA functions valid only for the linear mapping within
// the kernel binary area.
// RAM_MAP, SLABS and VMM (will) have their own functions.
static inline
pa_t va_to_pa(va_t va)
{
	return va - VA_BASE + RAM_BASE;
}

static inline
va_t pa_to_va(pa_t pa)
{
	return pa - RAM_BASE + VA_BASE;
}

static inline
pfn_t pa_to_pfn(pa_t pa)
{
	return pa >> PAGE_SIZE_BITS;
}

static inline
pa_t pfn_to_pa(pfn_t frame)
{
	return ((pa_t)frame) << PAGE_SIZE_BITS;
}

static inline
vpn_t va_to_vpn(va_t va)
{
	return va >> PAGE_SIZE_BITS;
}

static inline
va_t vpn_to_va(vpn_t page)
{
	return ((va_t)page) << PAGE_SIZE_BITS;
}

static inline
pfn_t vpn_to_pfn(vpn_t page)
{
	return pa_to_pfn(va_to_pa(vpn_to_va(page)));
}

static inline
va_t ram_map_pa_to_va(pa_t pa)
{
	return pa - RAM_BASE + RAM_MAP_BASE;
}

static inline
pa_t ram_map_va_to_pa(va_t va)
{
	return va - RAM_MAP_BASE + RAM_BASE;
}

static inline
ba_t pa_to_ba(pa_t pa)
{
	return pa | 0x40000000ul;
}

static inline
ba_t va_to_ba(va_t va)
{
	return pa_to_ba(va_to_pa(va));
}

int mmu_map_page(int pid, vpn_t page, pfn_t frame, enum align_bits align,
		 int flags);

int mmu_unmap_page(int pid, vpn_t page);
#endif
