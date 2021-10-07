// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>
#include <lib/string.h>	// For memset

#include <sys/err.h>
#include <sys/pmm.h>	// For pmm_get_num_frames, pmm_alloc
#include <sys/mutex.h>	// For mutex
#include <sys/mmu.h>	// For va_to_pa

// PD0 for system
static pde_t g_pd0_hw[PD0_SIZE >> 2] __attribute__((aligned(PD0_SIZE)));

static struct mutex g_mmu_lock;
static struct list_head g_pd1_head;

void	mmu_real_switch(reg_t ttbr0);

static
uint32_t mmu_get_sn_flags(int flags, char is_ssn, pa_t pa)
{
	uint32_t val;

	val = 0;
	val |= bits_on(PTE_SN);
	if (is_ssn)
		val |= bits_on(PTE_SSN);

	if (flags & ATTR_IO) {
		// TEX,C,B = 000,0,1 Device, Shared.
		// XN
		flags &= ~PROT_X;
		val |= bits_on(PTE_SN_B);
	} else {
		// Normal, cacheable, Non-shared
		val |= bits_on(PTE_SN_B);
		val |= bits_on(PTE_SN_C);
		val |= bits_set(PTE_SN_TEX, 5);
	}

	if (flags & PROT_W) {
		flags &= ~PROT_X;
		val |= bits_set(PTE_SN_AP, 1);
	} else if (flags & PROT_R) {
		val |= bits_on(PTE_SN_APX);
		val |= bits_set(PTE_SN_AP, 1);
	}

	if (!(flags & PROT_X))
		val |= bits_on(PTE_SN_XN);

	if (!is_ssn)
		val |= bits_push(PTE_SN_BASE, pa);
	else
		val |= bits_push(PTE_SSN_BASE, pa);
	return val;
}

static
uint32_t mmu_get_flags(int flags, char is_sn, char is_ssn, pa_t pa)
{
	uint32_t val;

	if (is_sn)
		return mmu_get_sn_flags(flags, is_ssn, pa);

	val = 0;
	val |= bits_on(PTE_LP);

	if (flags & ATTR_IO) {
		// TEX,C,B = 000,0,1 Device, Shared.
		// XN
		flags &= ~PROT_X;
		val |= bits_on(PTE_B);
	} else {
		// Normal, cacheable, Non-Shared.
		val |= bits_on(PTE_B);
		val |= bits_on(PTE_C);
		val |= bits_set(PTE_TEX, 5);
	}

	if (flags & PROT_W) {
		flags &= ~PROT_X;
		val |= bits_set(PTE_AP, 1);
	} else if (flags & PROT_R) {
		val |= bits_on(PTE_APX);
		val |= bits_set(PTE_AP, 1);
	}

	if (!(flags & PROT_X))
		val |= bits_on(PTE_XN);
	val |= bits_push(PTE_BASE, pa);
	return val;
}

static
void mmu_init_map_sys(va_t *sys_end)
{
	int ix, i, j, k, num_pages, flags, pd1_ix;
	va_t se, va;
	pa_t pa, pd1_pa;
	uint32_t val, memsz, prop;
	pde_t *pd1_hw;
	struct elf32_hdr *eh;
	struct elf32_phdr *ph;
	// PD1 for the system area.
	static pde_t pd1_hw_arr[32][PD1_SIZE >> 2]
		__attribute__((aligned(PD1_SIZE))) = {0};

	// VA_BASE is a 512MB region. Thus it needs a total of 512 PD0
	// entries, and so 512 possible PD1s.
	pd1_ix = 0;
	se = *sys_end;
	eh = (struct elf32_hdr *)SYS_BASE;
	ph = (struct elf32_phdr *)((char *)eh + eh->phoff);

	for (i = 0; i < eh->phnum; ++i, ++ph) {
		if (ph->type != PT_LOAD || ph->memsz == 0)
			continue;

		memsz = ph->memsz;
		if (i == eh->phnum - 1)
			memsz = se - ph->vaddr;

		memsz = align_up(memsz, PAGE_SIZE_BITS);
		num_pages = memsz >> PAGE_SIZE_BITS;
		va = (va_t)ph->vaddr;
		pa = va_to_pa(va);
		flags = ph->flags;

		prop = mmu_get_flags(flags, 0, 0, 0);
		for (j = 0; j < num_pages; ++j) {
			ix = bits_get(va, PD0);
			val = g_pd0_hw[ix];
			if (val == 0) {
				// Support 32 PD1s max, atm.
				assert(pd1_ix < 32);
				pd1_hw = pd1_hw_arr[pd1_ix];
				pd1_pa = va_to_pa((va_t)pd1_hw);
				val |= bits_on(PDE_T);
				val |= bits_push(PDE_NLTA, pd1_pa);
				g_pd0_hw[ix] = val;
				++pd1_ix;
			} else {
				pd1_pa = bits_pull(val, PDE_NLTA);
				pd1_hw = (pde_t *)pa_to_va(pd1_pa);
			}

			ix = bits_get(va, PD1) & (~0xf);
			val = prop;
			val |= bits_push(PTE_BASE, pa);
			for (k = 0; k < 16; ++k, ++ix)
				pd1_hw[ix] = val;
			va += PAGE_SIZE;
			pa += PAGE_SIZE;
		}
	}
}

static
void mmu_init_map_ram_map()
{
	int num_frames, i, ix, j;
	uint32_t val, prop;
	pa_t pa;
	va_t va;

	num_frames = pmm_get_num_frames();
	pa = RAM_BASE + pfn_to_pa(num_frames);
	pa = align_up(pa, ALIGN_16MB);
	num_frames = (pa - RAM_BASE) >> ALIGN_16MB;

	// The RAM_MAP is a 512MB region. The # of frames that we can support,
	// each of size 16MB, is thus 512GB/16MB = 32
	assert(num_frames <= 32);

	va = RAM_MAP_BASE;	// assumed aligned on 16MB boundary.
	pa = RAM_BASE;
	ix = bits_get(va, PD0);
	prop = mmu_get_flags(PROT_RW, 1, 1, 0);
	for (i = 0; i < num_frames; ++i) {
		val = prop;
		val |= bits_push(PTE_SSN_BASE, pa);
		for (j = 0; j < 16; ++j, ++ix)
			g_pd0_hw[ix] = val;
		pa += 1ul << ALIGN_16MB;
	}
}

int mmu_init(va_t *sys_end)
{
	int ix, flags;
	uint32_t val;
	pa_t pa;

	list_init(&g_pd1_head);
	mutex_init(&g_mmu_lock);

	// Mark 1MB containing the mmu_real_switch as RX.
	flags = PROT_R | PROT_X;
	pa = va_to_pa((va_t)mmu_real_switch);
	val = mmu_get_flags(flags, 1, 0, pa);
	ix = bits_get(pa, PD0);
	g_pd0_hw[ix] = val;

	// VA_BASE, RAM_MAP_BASE, SLABS_BASE and VMM_BASE are all sized
	// 512MB, thus totalling 2GB of system address space.

	// RAM_MAP is mapped as supersections, so it doesn't need PD1.
	mmu_init_map_ram_map();
	mmu_init_map_sys(sys_end);
	return ERR_SUCCESS;
}

int mmu_post_init(va_t sys_end)
{
	reg_t ttbr;
	uint64_t val;
	enum ipl ipl;
	reg_t irq_mask;
	typedef void amrs_fn(reg_t ttbr);
	amrs_fn *fn;

	// Calculate TTBR
	val = 0;
	val |= bits_on(TTBR_C);		// Inner Cacheable
	val |= bits_set(TTBR_RGN, 1);	// Outer Cacheable, WB
	val |= bits_push(TTBR_BASE, va_to_pa((va_t)g_pd0_hw));
	ttbr = val;

	fn = (amrs_fn *)va_to_pa((va_t)mmu_real_switch);

	// Block all interrupts while we switch the mmu.
	ipl = cpu_raise_ipl(IPL_HARD, &irq_mask);
	assert(ipl == IPL_THREAD);

	// Run the switching code at its real (physical) address, and then
	// jump back to the virtual mode.
	fn(ttbr);
	cpu_lower_ipl(ipl, irq_mask);

	return ERR_SUCCESS;
	(void)sys_end;
}

//////////////////////////////////////////////////////////////////////////////
static
int mmu_alloc_pd(va_t *out_va, pa_t *out_pa)
{
	size_t i;
	int err;
	pfn_t frame;
	va_t va;
	pa_t pa;
	struct list_head *e;

	if (list_is_empty(&g_pd1_head)) {
		err = pmm_alloc(ALIGN_PAGE, 1, &frame);
		if (err)
			return err;
		pa = pfn_to_pa(frame);
		va = ram_map_pa_to_va(pa);

		e = (struct list_head *)va;
		for (i = 0; i < PAGE_SIZE / PD1_SIZE; ++i) {
			list_add_tail(&g_pd1_head, e);
			e = (struct list_head *)((char *)e + PD1_SIZE);
		}
	}
	e = list_del_head(&g_pd1_head);
	va = (va_t)e;
	pa = ram_map_va_to_pa(va);
	*out_va = va;
	*out_pa = pa;
	return ERR_SUCCESS;
}

static
int mmu_read_pde(pde_t pde_hw, int pde_level, pa_t *out_pa,
		 char *out_is_pte, char *out_is_ssn)
{
	char is_pte, t, sn, ssn;

	assert(pde_level == 0 || pde_level == 1);

	if (pde_level == 0) {
		t = bits_get(pde_hw, PDE_T);
		sn = bits_get(pde_hw, PTE_SN);
		ssn = bits_get(pde_hw, PTE_SSN);

		if (t && sn)
			return ERR_UNEXP;

		if (!t && !sn)
			return ERR_INVALID;

		is_pte = sn;
		if (out_is_pte)
			*out_is_pte = is_pte;

		if (out_pa) {
			if (!is_pte)
				*out_pa = bits_pull(pde_hw, PDE_NLTA);
			else if (ssn == 0)
				*out_pa = bits_pull(pde_hw, PTE_SN_BASE);
			else
				*out_pa = bits_pull(pde_hw, PTE_SSN_BASE);
		}

		if (is_pte && out_is_ssn)
			*out_is_ssn = ssn;
		return ERR_SUCCESS;
	}

	t = pde_hw & 3;
	if (t == 0)
		return ERR_INVALID;
	if (t != 1)
		return ERR_UNEXP;

	if (out_is_pte)
		*out_is_pte = 1;
	if (out_pa)
		*out_pa = bits_pull(pde_hw, PTE_BASE);
	return ERR_SUCCESS;
}

static
int mmu_write_pde(pde_t *pde_hw, int pde_level, pa_t pa)
{
	uint64_t val;

	assert(pde_hw);
	assert(pde_level == 0);

	val = 0;
	val |= bits_on(PDE_T);
	val |= bits_push(PDE_NLTA, pa);
	*pde_hw = val;
	dc_cvac(pde_hw, sizeof(*pde_hw));
	return ERR_SUCCESS;
}

static
int mmu_write_sn_pte(pde_t *pde_hw, char is_ssn, pa_t pa, int flags)
{
	int i;
	uint32_t val;

	val = mmu_get_flags(flags, 1, is_ssn, pa);
	pde_hw[0] = val;
	for (i = 1; is_ssn && i < 16; ++i)
		pde_hw[i] = val;
	if (is_ssn)
		dc_cvac(pde_hw, 16 * sizeof(*pde_hw));
	else
		dc_cvac(pde_hw, sizeof(*pde_hw));
	return ERR_SUCCESS;
}

static
int mmu_write_pte(pde_t *pde_hw, int pte_level, char is_ssn, pa_t pa,
		  int flags)
{
	int i;
	uint32_t val;

	assert(pde_hw);
	assert(pte_level == 0 || pte_level == 1);

	if (pte_level == 0)
		return mmu_write_sn_pte(pde_hw, is_ssn, pa, flags);

	val = mmu_get_flags(flags, 0, 0, pa);
	for (i = 0; i < 16; ++i)
		pde_hw[i] = val;
	dc_cvac(pde_hw, 16 * sizeof(*pde_hw));
	return ERR_SUCCESS;
}

static
int mmu_map(pde_t *pd0_hw, va_t va, pa_t pa, int flags, int pte_level)
{
	int ix[2], i, err;
	char is_pte, is_ssn;
	pde_t *pd_hw, *pde_hw;
	va_t pdn_hw_va;
	pa_t pdn_hw_pa;

	assert(pte_level >= 0 && pte_level < 3);
	if (pte_level < 0 || pte_level > 2)
		return ERR_PARAM;

	ix[0] = bits_get(va, PD0);
	ix[1] = bits_get(va, PD1);

	is_ssn = 0;
	if (pte_level == 0)
		is_ssn = 1;
	else
		--pte_level;

	pd_hw = pd0_hw;
	for (i = 0; i < pte_level + 1; ++i) {
		pde_hw = &pd_hw[ix[i]];
		err = mmu_read_pde(*pde_hw, i, &pdn_hw_pa, &is_pte, NULL);

		// Are we at the page table (vs being at a page directory)?
		if (i == pte_level) {
			// At the last level, the pte should be invalid.
			if (err != ERR_INVALID)
				return ERR_UNEXP;
			err = mmu_write_pte(pde_hw, i, is_ssn, pa, flags);
			if (!err)
				tlbi_va(va);
			return err;
		}

		// Not yet at the page table.
		if (!err) {
			// Successfully read the next-level PD pa.
			pdn_hw_va = ram_map_pa_to_va(pdn_hw_pa);
			pd_hw = (pde_t *)pdn_hw_va;
			continue;
		}

		// The pde should be invalid; if not, return the error.
		if (err != ERR_INVALID)
			return err;

		err = mmu_alloc_pd(&pdn_hw_va, &pdn_hw_pa);
		if (err)
			return err;

		pd_hw = (pde_t *)pdn_hw_va;
		assert(i == 0);
		memset(pd_hw, 0, PD1_SIZE);

		err = mmu_write_pde(pde_hw, i, pdn_hw_pa);
		if (err)
			return err;
	}
	return ERR_UNEXP;
}

int mmu_map_page(int pid, vpn_t page, pfn_t frame, enum align_bits align,
		 int flags)
{
	int err, pte_level;
	va_t va;
	pa_t pa;
	static const enum align_bits align_arr[3] = {
		ALIGN_16MB, ALIGN_1MB, ALIGN_PAGE
	};

	assert(pid == 0);

	if (pid)
		return ERR_PARAM;

	assert(align == ALIGN_16MB || align == ALIGN_1MB ||
	       align == ALIGN_PAGE);
	if (align != ALIGN_16MB && align != ALIGN_1MB && align != ALIGN_PAGE)
		return ERR_PARAM;

	// input pte_level:
	// 0 == 16MB, 1 == 1MB, and 2 == PAGE
	if (align == ALIGN_16MB)
		pte_level = 0;
	else if (align == ALIGN_1MB)
		pte_level = 1;
	else
		pte_level = 2;

	va = vpn_to_va(page);
	pa = pfn_to_pa(frame);
	if (!is_aligned(va, align_arr[pte_level]) ||
	    !is_aligned(pa, align_arr[pte_level]))
		return ERR_PARAM;

	mutex_lock(&g_mmu_lock);
	err = mmu_map(g_pd0_hw, va, pa, flags, pte_level);
	mutex_unlock(&g_mmu_lock);
	return err;
}

int mmu_unmap_page(int pid, vpn_t page)
{
	assert(pid == 0);
	assert(0);
	(void)page;
	return ERR_SUCCESS;
}
