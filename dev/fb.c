// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>
#include <lib/stdlib.h>
#include <lib/string.h>

#include <sys/err.h>
#include <sys/cpu.h>
#include <sys/mmu.h>
#include <sys/pmm.h>
#include <sys/slab.h>
#include <sys/vmm.h>

#include <dev/con.h>
#include <dev/mbox.h>
#include <dev/v3d.h>

static pa_t g_fb_base;
static size_t g_fb_size;
volatile uint32_t *g_fb;

static
int fb_map()
{
	int num_pages, i, err;
	va_t va;
	vpn_t pages, page;
	pfn_t frame;
	size_t size;

	size = align_up(g_fb_size, PAGE_SIZE);
	num_pages = size >> PAGE_SIZE_BITS;

	err = vmm_alloc(ALIGN_PAGE, num_pages, &pages);
	if (err)
		goto err0;

	page = pages;
	frame = pa_to_pfn(g_fb_base);
	for (i = 0; i < num_pages; ++i, ++page, ++frame) {
		err = mmu_map_page(0, page, frame, ALIGN_PAGE, PROT_RW);
		if (err)
			goto err1;
	}
	va = vpn_to_va(pages);
	va += g_fb_base & (PAGE_SIZE - 1);
	g_fb = (volatile uint32_t *)va;
	return ERR_SUCCESS;
err1:
	for (--i, --page; i >= 0; --i, --page)
		mmu_unmap_page(0, page);
	vmm_free(pages, num_pages);
err0:
	return err;
}

int fb_init()
{
	int err;

	err = mbox_alloc_fb(&g_fb_base, &g_fb_size);
	if (err)
		goto err0;

	err = fb_map();
	if (err)
		goto err1;
	return ERR_SUCCESS;
err1:
	mbox_free_fb(g_fb_base);
err0:
	return err;
}
