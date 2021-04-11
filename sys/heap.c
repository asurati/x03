// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <sys/err.h>
#include <sys/pmm.h>
#include <sys/slabs.h>

#define HEAP_SLAB_ENTRY_IX		1

#define NUM_HEAP_SIZES			19
static const size_t heap_sizes[NUM_HEAP_SIZES] = {
	2 << 3,				// 16
	3 << 3,				// 24 	sizeof(slab_entry)
	4 << 3,				// 32
	5 << 3,				// 40
	6 << 3,				// 48
	7 << 3,				// 56
	8 << 3,				// 64

	10 << 3,			// 80
	12 << 3,			// 96
	14 << 3,			// 112
	16 << 3,			// 128

	20 << 3,			// 160
	24 << 3,			// 192
	28 << 3,			// 224
	32 << 3,			// 256

	64 << 3,			// 512
	128 << 3,			// 1024
	256 << 3,			// 2048
	512 << 3			// 4096
};

static struct slab g_heaps[NUM_HEAP_SIZES];

static int heap_size_to_ix(size_t size)
{
	int i;

	for (i = 0; i < NUM_HEAP_SIZES; ++i)
		if (heap_sizes[i] >= size)
			break;
	return i;
}

int heap_init(pa_t *krnl_end)
{
	int err;
	struct slab *se_slab;
	pa_t ke;
	pfn_t frame;

	err = slabs_init(g_heaps, heap_sizes, NUM_HEAP_SIZES);
	if (err) return err;

	se_slab = &g_heaps[HEAP_SLAB_ENTRY_IX];

	ke = *krnl_end;
	ke = align_up(ke, PAGE_SIZE);

	frame = pa_to_pfn(ke);
	err = slabs_add_frame(se_slab, se_slab, frame);
	if (err) return err;

	ke += PAGE_SIZE;
	*krnl_end = ke;
	return ERR_SUCCESS;
}

static
int heap_alloc(size_t size, void **out)
{
	int ix, num_free, err;
	struct slab *se_slab, *slab;
	pfn_t frame;

	ix = heap_size_to_ix(size);
	if (ix == NUM_HEAP_SIZES)
		return ERR_UNSUP;

	// Does the slab_entry slab have sufficient free blocks?
	se_slab = &g_heaps[HEAP_SLAB_ENTRY_IX];
	num_free = slabs_num_free(se_slab);
	if (num_free <= 4) {
		err = pmm_alloc(ALIGN_PAGE, 1, &frame);
		if (err) return err;

		err = slabs_add_frame(se_slab, se_slab, frame);
		if (err) return err;
	}

	slab = &g_heaps[ix];
	num_free = slabs_num_free(slab);
	if (num_free == 0) {
		err = pmm_alloc(ALIGN_PAGE, 1, &frame);
		if (err) return err;

		err = slabs_add_frame(slab, se_slab, frame);
		if (err) return err;
	}
	return slabs_alloc(slab, out);
}

void *malloc(size_t size)
{
	int err;
	void *out;

	out = NULL;
	err = heap_alloc(size, &out);
	if (err) return NULL;
	return out;
}

void free(void *p)
{
	if (p == NULL) return;
	slabs_free(p);
}
