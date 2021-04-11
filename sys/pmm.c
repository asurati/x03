// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>
#include <lib/string.h>

#include <sys/bitmap.h>
#include <sys/endian.h>
#include <sys/err.h>
#include <sys/mmu.h>
#include <sys/mutex.h>

#include <dev/dtb.h>

struct frame {
	void				*ctx;
};

// Manages only RAM for now.
struct phy_mem_manager {
	struct mutex			lock;
	struct bitmap			map;
	struct frame			*frames;
	pfn_t				base_frame;
	int				num_frames;
};

static struct phy_mem_manager g_pmm;
static int g_pmm_in_init;

static
int pmm_parse_dtb(const struct dtb *dtb)
{
	int err, node;
	uint64_t base, end, size;
	static uint32_t reg[8];

	if (dtb == NULL) return ERR_PARAM;

	// Assuming size and address cells to be 1.
	node = dtb_node(dtb, "/memory@");
	if (node < 0) return node;

	err = dtbn_prop_read(dtb, node, "reg", reg, sizeof(reg));
	if (err) return err;

	base = be32_to_cpu(reg[0]);
	size = be32_to_cpu(reg[1]);
	end = base + size;

	assert(base == RAM_BASE);

	base = align_up(base, PAGE_SIZE);
	end = align_down(end, PAGE_SIZE);
	size = end - base;
	g_pmm.base_frame = pa_to_pfn(base);
	g_pmm.num_frames = size >> PAGE_SIZE_BITS;

	if (g_pmm.base_frame < 0 || g_pmm.num_frames <= 0)
		return ERR_BAD_FILE;

	return ERR_SUCCESS;
}

int pmm_init(const struct dtb *dtb, pa_t *krnl_end)
{
	int err;
	pa_t ke;
	size_t size;

	if (dtb == NULL || krnl_end == NULL)
		return ERR_PARAM;

	g_pmm.frames = NULL;

	err = pmm_parse_dtb(dtb);
	if (err) return err;

	ke = *krnl_end;
	ke = align_up(ke, 8);	// Align on 8byte boundary.

	err = bitmap_init(&g_pmm.map, (void *)ke, g_pmm.num_frames);
	if (err) return err;

	// Bitmap works with 64-bit units.
	// Reserve a few extra bits to align.

	size = align_up(g_pmm.num_frames, 64);
	ke += size >> 3;

	// Frame array.
	g_pmm.frames = (void *)ke;
	size = sizeof(g_pmm.frames[0]) * g_pmm.num_frames;
	ke += size;
	memset(g_pmm.frames, 0, size);

	g_pmm_in_init = 1;
	*krnl_end = ke;

	mutex_init(&g_pmm.lock);
	return ERR_SUCCESS;
}

int pmm_post_init(const struct dtb *dtb, pa_t krnl_end)
{
	pfn_t frame[2];
	int i, npages, err;
	uint64_t t;
	static uint64_t res_addr[32];
	static uint64_t res_size[32];

	// Mark reserved mem as busy.
	err = dtb_mem_res_read(dtb, res_addr, res_size, 32);
	if (err) return err;

	for (i = 0; i < 32; ++i) {
		if (res_size[i] == 0)
			break;

		t = res_addr[i];
		frame[0] = pa_to_pfn(align_down(t, PAGE_SIZE));
		frame[0] -= g_pmm.base_frame;

		t += res_size[i];
		frame[1] = pa_to_pfn(align_up(t, PAGE_SIZE));
		frame[1] -= g_pmm.base_frame;

		// Skip the entry if it doesn't lie inside our range.
		if (frame[0] < 0 || frame[1] < 0 ||
		    frame[0] >= g_pmm.num_frames ||
		    frame[1] >= g_pmm.num_frames)
			continue;

		npages = frame[1] - frame[0];
		err = bitmap_on(&g_pmm.map, frame[0], npages);
		if (err) return err;
	}

	// Mark kernel frames as busy.
	frame[0] = pa_to_pfn(align_down(VA_BASE, PAGE_SIZE));
	frame[1] = pa_to_pfn(align_up(krnl_end, PAGE_SIZE));
	npages = frame[1] - frame[0];

	frame[0] -= g_pmm.base_frame;
	err = bitmap_on(&g_pmm.map, frame[0], npages);
	if (err) return err;

	g_pmm_in_init = 0;
	return ERR_SUCCESS;
}

int pmm_alloc(enum align_bits align, int num_frames, pfn_t *out)
{
	int frame_align, frame, err;

	if (out == NULL || num_frames <= 0) return ERR_PARAM;

	mutex_lock(&g_pmm.lock);
	frame_align = align - ALIGN_PAGE;
	err = frame = bitmap_find_off(&g_pmm.map, frame_align, num_frames);
	if (err < 0) goto err0;

	err = bitmap_on(&g_pmm.map, frame, num_frames);
	if (err) goto err0;

	*out = g_pmm.base_frame + frame;
err0:
	mutex_unlock(&g_pmm.lock);
	return err;
}

int pmm_free(pfn_t frame, int num_frames)
{
	int err;

	if (frame < 0 || num_frames <= 0) return ERR_PARAM;

	frame -= g_pmm.base_frame;

	// Should be allocated.
	mutex_lock(&g_pmm.lock);
	err = bitmap_is_on(&g_pmm.map, frame, num_frames);
	if (err) goto err0;

	err = bitmap_off(&g_pmm.map, frame, num_frames);
err0:
	mutex_unlock(&g_pmm.lock);
	return err;
}

int pmm_get_ctx(pfn_t frame, void **out)
{
	int err;

	if (frame < 0 || out == NULL)
		return ERR_PARAM;

	frame -= g_pmm.base_frame;

	// Should be allocated.
	mutex_lock(&g_pmm.lock);
	err = bitmap_is_on(&g_pmm.map, frame, 1);
	if (err) goto err0;
	*out = g_pmm.frames[frame].ctx;
err0:
	mutex_unlock(&g_pmm.lock);
	return err;
}

int pmm_set_ctx(pfn_t frame, void *ctx)
{
	int err;

	if (frame < 0) return ERR_PARAM;

	frame -= g_pmm.base_frame;

	// During init, slabs set ctx before the frame is marked ON.
	mutex_lock(&g_pmm.lock);
	if (!g_pmm_in_init) {
		// Should be allocated.
		err = bitmap_is_on(&g_pmm.map, frame, 1);
		if (err) goto err0;
	}
	err = ERR_SUCCESS;
	g_pmm.frames[frame].ctx = ctx;
err0:
	mutex_unlock(&g_pmm.lock);
	return err;
}
