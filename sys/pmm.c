// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>
#include <lib/string.h>

#include <sys/bitmap.h>
#include <sys/err.h>
#include <sys/mmu.h>
#include <sys/mutex.h>

// Manages only RAM for now.
struct phy_mem_manager {
	struct mutex			lock;
	struct bitmap			map;
	pfn_t				base_frame;
	int				num_frames;
};

static struct phy_mem_manager g_pmm;

int pmm_get_num_frames()
{
	return g_pmm.num_frames;
}

int pmm_init(va_t *sys_end)
{
	int err;
	va_t se;
	size_t size;

	if (sys_end == NULL)
		return ERR_PARAM;

	g_pmm.num_frames = RAM_SIZE >> PAGE_SIZE_BITS;
	g_pmm.base_frame = pa_to_pfn(RAM_BASE);

	mutex_init(&g_pmm.lock);

	se = *sys_end;
	se = align_up(se, 3);	// Align on 8byte boundary.

	err = bitmap_init(&g_pmm.map, (void *)se, g_pmm.num_frames);
	if (err)
		return err;

	// Bitmap works with 64-bit units. Reserve a few extra bits to align.
	size = align_up(g_pmm.num_frames, 6);
	se += size >> 3;
	*sys_end = se;
	return ERR_SUCCESS;
}

int pmm_post_init(va_t sys_end)
{
	pa_t pa[2];
	pfn_t frame[2];
	int nframes, err;

	// Mark kernel frames as busy.
	pa[0] = va_to_pa(SYS_BASE);
	pa[1] = va_to_pa(sys_end);
	frame[0] = pa_to_pfn(align_down(pa[0], PAGE_SIZE_BITS));
	frame[1] = pa_to_pfn(align_down(pa[1], PAGE_SIZE_BITS));
	nframes = frame[1] - frame[0];
	frame[0] -= g_pmm.base_frame;

	err = bitmap_on(&g_pmm.map, frame[0], nframes);
	if (err)
		return err;
	return ERR_SUCCESS;
}

int pmm_alloc(enum align_bits align, int num_frames, pfn_t *out)
{
	int frame_align, frame, err;

	if (out == NULL || num_frames <= 0)
		return ERR_PARAM;

	frame_align = align - ALIGN_PAGE;

	mutex_lock(&g_pmm.lock);
	err = frame = bitmap_find_off(&g_pmm.map, frame_align, 0, num_frames);
	if (frame < 0)
		goto exit;

	err = bitmap_on(&g_pmm.map, frame, num_frames);
	if (!err)
		*out = g_pmm.base_frame + frame;
exit:
	mutex_unlock(&g_pmm.lock);
	return err;
}

int pmm_free(pfn_t frame, int num_frames)
{
	int err;

	if (frame < 0 || num_frames <= 0)
		return ERR_PARAM;

	frame -= g_pmm.base_frame;

	// Should be allocated.
	mutex_lock(&g_pmm.lock);
	err = bitmap_is_on(&g_pmm.map, frame, num_frames);
	if (!err)
		err = bitmap_off(&g_pmm.map, frame, num_frames);
	mutex_unlock(&g_pmm.lock);
	return err;
}
