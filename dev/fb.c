// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdio.h>

#include <sys/err.h>
#include <sys/cpu.h>
#include <sys/vmm.h>

#include <dev/con.h>
#include <dev/mbox.h>

static pa_t g_fb_base;
static size_t g_fb_size;
static volatile uint32_t *g_fb;

pa_t fb_get_pa()
{
	return g_fb_base;
}

// Run length encoding.
void rle_dump(volatile uint32_t *fb, int width, int height)
{
	int i, count[8], l;
	uint32_t val[8], t;
	static char buf[1024];
	static const char *fmt =
		"{0x%x,%d}," "{0x%x,%d}," "{0x%x,%d}," "{0x%x,%d},"
		"{0x%x,%d}," "{0x%x,%d}," "{0x%x,%d}," "{0x%x,%d},";

	l = 0;
	val[l] = fb[0];
	count[l] = 1;
	for (i = 1; i < width * height; ++i) {
		t = fb[i];
		if (t == val[l]) {
			++count[l];
			continue;
		}
		++l;
		if (l == 8) {
			snprintf(buf, 1024, fmt,
				val[0], count[0], val[1], count[1],
				val[2], count[2], val[3], count[3],
				val[4], count[4], val[5], count[5],
				val[6], count[6], val[7], count[7]);
			con_out(buf);
			l = 0;
		}
		val[l] = t;
		count[l] = 1;
	}

	for (i = 0; i <= l; ++i)
		con_out("{0x%x,%d},", val[i], count[i]);
}

int fb_dump()
{
	rle_dump(g_fb, 640, 480);
	return ERR_SUCCESS;
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
