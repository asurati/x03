// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <stddef.h>

#include <lib/assert.h>

#include <sys/bits.h>
#include <sys/err.h>
#include <sys/vmm.h>

// IPL_THREAD
int dev_map_io(pa_t pa, size_t size, va_t *out)
{
	int err, num_pages, i;
	va_t va;
	vpn_t pages, page;
	pfn_t frame;

	size = align_up(size, PAGE_SIZE_BITS);
	num_pages = size >> PAGE_SIZE_BITS;

	err = vmm_alloc(ALIGN_PAGE, num_pages, &pages);
	if (err)
		goto err0;

	page = pages;
	frame = pa_to_pfn(pa);
	for (i = 0; i < num_pages; ++i, ++page, ++frame) {
		err = mmu_map_page(0, page, frame, ALIGN_PAGE,
				   PROT_RW | ATTR_IO);
		if (err)
			goto err1;
	}
	va = vpn_to_va(pages);
	va |= pa & (PAGE_SIZE - 1);
	*out = va;
	// TODO invalidate any cache entries for the VA.
	return ERR_SUCCESS;
err1:
	for (--i, --page; i >= 0; --i, --page)
		mmu_unmap_page(0, page);
	vmm_free(pages, num_pages);
err0:
	return err;
}
