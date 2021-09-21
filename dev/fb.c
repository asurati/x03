// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <sys/err.h>
#include <sys/cpu.h>
#include <sys/vmm.h>

#include <dev/mbox.h>

static pa_t g_fb_base;
static size_t g_fb_size;
static volatile uint32_t *g_fb;

pa_t fb_get_pa()
{
	return g_fb_base;
}

volatile uint32_t *fb_get()
{
	return g_fb;
}

static
int fb_map()
{
	int num_pages, i, err;
	vpn_t pages, page;
	pfn_t frame;
	size_t size;

	size = align_up(g_fb_size, PAGE_SIZE_BITS);
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
	g_fb = (volatile uint32_t *)vpn_to_va(pages);
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
