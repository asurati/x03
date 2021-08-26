// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <stddef.h>

#include <lib/string.h>

#include <sys/cpu.h>
#include <sys/mmu.h>

static pde_t g_pd0_hw[PD0_SIZE >> 2]
__attribute__((aligned(PD0_SIZE))) __attribute__((section(".data"))) = {0};

void set_cache_size()
{
	reg_t val;

	val = mrc_aux_cr();
	val |= bits_on(AUX_CR_CZ);
	mcr_aux_cr(val);
}

void invalidate()
{
	// Invalidate TLBs.
	tlbi_all();

	// Invalidate Caches
	icdc_iall();
	dsb();
	isb();
}

void mmu_setup()
{
	int ix, i;
	uint32_t val, prop;

	prop = 0;
	prop |= bits_on(PTE_SN);
	prop |= bits_on(PTE_SSN);

	// TEX,C,B = 101,1,1. Outer: WBWA, Inner: WBNWA
	// See Table 6-2 inside DDI0301H_arm1176jzfs_r0p7_trm.pdf for
	// point (c).
	// "The processor does not support Inner Allocate on Write."
	prop |= bits_on(PTE_SN_B);
	prop |= bits_on(PTE_SN_C);
	prop |= bits_set(PTE_SN_TEX, 5);

	// AP_RW_NONE
	prop |= bits_set(PTE_SN_AP, 1);

	// Mark 16MB containing the pkg.bin as RWX.
	// UBoot is asked to load us at RAM_BASE + 0x10000.
	// RAM_BASE assumed to be aligned on 16MB.
	ix = bits_get(RAM_BASE, PD0);
	val = prop;
	val |= bits_push(PTE_SSN_BASE, RAM_BASE);	// Identity Map.
	for (i = 0; i < 16; ++i, ++ix)
		g_pd0_hw[ix] = val;

	// Mark 16MB containing the system as RWX. The SYS_BASE is +4MB from
	// the VA_BASE.
	ix = bits_get(VA_BASE, PD0);
	val = prop;
	val |= bits_push(PTE_SSN_BASE, va_to_pa(VA_BASE));
	for (i = 0; i < 16; ++i, ++ix)
		g_pd0_hw[ix] = val;
}

void mmu_enable()
{
	uint32_t val;

	// Domain0 is Client.
	mcr_dacr(1);

	val = 0;
	val |= bits_on(TTBR_C);		// Inner Cacheable
	val |= bits_set(TTBR_RGN, 1);	// Outer Cacheable, WBWA
	val |= bits_push(TTBR_BASE, (pa_t)g_pd0_hw);
	mcr_ttbr0(val);

	val = mrc_cr();
	val |= bits_on(CR_M);
	val |= bits_on(CR_A);
	val |= bits_on(CR_C);
	val |= bits_on(CR_W);
	val |= bits_on(CR_Z);
	val |= bits_on(CR_I);
	val |= bits_on(CR_XP);
	mcr_cr(val);
}

uint32_t load_sys()
{
	int i;
	char *dst;
	const char *p, *src;
	extern char _end[];
	const struct elf32_hdr *eh;
	const struct elf32_phdr *ph;

	p = _end;
	eh = (const struct elf32_hdr *)p;
	ph = (const struct elf32_phdr *)(p + eh->phoff);
	for (i = 0; i < eh->phnum; ++i, ++ph) {
		// skip if not of type PT_LOAD.
		if (ph->type != PT_LOAD)
			continue;

		// skip if empty.
		if (ph->memsz == 0)
			continue;

		src = p + ph->offset;
		dst = (char *)va_to_pa(ph->vaddr);

		// Copy data from file.
		memcpy(dst, src, ph->filesz);

		// Zero any remaining in-mem length.
		dst += ph->filesz;
		memset(dst, 0, ph->memsz - ph->filesz);
	}
	return eh->entry;
}
