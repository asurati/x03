// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/string.h>

#include <sys/bitmap.h>
#include <sys/err.h>
#include <sys/mmu.h>
#include <sys/mutex.h>

struct virt_mem_manager {
	struct mutex			lock;
	struct bitmap			map;
	vpn_t				base_page;
	int				num_pages;
};

static struct virt_mem_manager g_vmm;

// VMM_SIZE is 512MB. That is 1ul << (29 - 16) = 8192 pages.
// 8192 bits is 1024 bytes. So a bitmap of 1024 bytes is needed.
// The start of this VA space is at VMM_BASE. This space excludes the kernel,
// and contains only dynamically allocated regions.
int vmm_init(va_t *sys_end)
{
	int err;
	static uint64_t bits[1024 >> 3] = {0};	// aligned at 64-bits.

	mutex_init(&g_vmm.lock);

	g_vmm.base_page = va_to_vpn(VMM_BASE);
	g_vmm.num_pages = VMM_SIZE >> PAGE_SIZE_BITS;

	err = bitmap_init(&g_vmm.map, (void *)bits, g_vmm.num_pages);
	return err;
	(void)sys_end;
}

int vmm_alloc(enum align_bits align, int num_pages, vpn_t *out)
{
	int page_align, page, err;

	if (out == NULL || num_pages <= 0)
		return ERR_PARAM;

	page_align = align - ALIGN_PAGE;

	mutex_lock(&g_vmm.lock);
	err = page = bitmap_find_off(&g_vmm.map, page_align, 0, num_pages);
	if (page < 0)
		goto exit;

	err = bitmap_on(&g_vmm.map, page, num_pages);
	if (!err)
		*out = g_vmm.base_page + page;
exit:
	mutex_unlock(&g_vmm.lock);
	return err;
}

int vmm_free(pfn_t page, int num_pages)
{
	int err;

	if (page < 0 || num_pages <= 0)
		return ERR_PARAM;

	page -= g_vmm.base_page;

	// Should be allocated.
	mutex_lock(&g_vmm.lock);
	err = bitmap_is_on(&g_vmm.map, page, num_pages);
	if (!err)
		err = bitmap_off(&g_vmm.map, page, num_pages);
	mutex_unlock(&g_vmm.lock);
	return err;
}
