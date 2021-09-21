// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>
#include <lib/string.h>
#include <lib/stdlib.h>

#include <sys/bitmap.h>
#include <sys/condvar.h>
#include <sys/err.h>
#include <sys/list.h>
#include <sys/mmu.h>
#include <sys/pmm.h>
#include <sys/vmm.h>
#include <sys/mutex.h>
#include <sys/slab.h>

// Max # of objects in any 64KB page is 64KB / 8 = 8192 = 0x2000
// A single 64KB bitmap can handle 32GB of area.

#define SLAB_ENTRY			0xcafe

struct slab {
	struct list_head		free_head;
	struct list_head		part_head;
	struct mutex			lock;
	struct cond_var			wait;
	int				num_free;
	short				num_objs_per_entry;
	unsigned short			flags;
};

// Each entry handles 1 64KB page.
// sizeof on 64-bit: 32 bytes.
struct slab_entry {
	unsigned short			flags;
	unsigned short			slab_ix;
	short				next_free;
	short				num_free;
	struct list_head		entry;
	pfn_t				frame;	// For slabs_va_to_pa
};

#define NUM_SE_PER_PAGE			(PAGE_SIZE / sizeof(struct slab_entry))
#define NUM_SLAB_SIZES			13

struct slabs_manager {
	struct mutex			lock;
	vpn_t				base_pages[2];
	int				num_pages;

	struct bitmap			map;
	struct slab			slabs[NUM_SLAB_SIZES];
};

static const size_t g_slab_sizes[NUM_SLAB_SIZES] = {
	1ul << 3,	// 8
	1ul << 4,	// 16
	1ul << 5,	// 32
	1ul << 6,	// 64
	1ul << 7,	// 128
	1ul << 8,	// 256
	1ul << 9,	// 512
	1ul << 10,	// 1024
	1ul << 11,	// 2048
	1ul << 12,	// 4096
	1ul << 13,	// 8192
	1ul << 14,	// 16384
	1ul << 15,	// 32768
};
static struct slabs_manager g_sm;

// Called at IPL_THREAD
static
int slab_se_init(unsigned short slab_ix, struct slab_entry *se, vpn_t page,
		 pfn_t frame)
{
	int i;
	char *p;
	struct slab *slab;

	assert(slab_ix < NUM_SLAB_SIZES);
	if (slab_ix >= NUM_SLAB_SIZES)
		return ERR_PARAM;

	slab = &g_sm.slabs[slab_ix];

	assert(se);
	assert(page >= 0);
	assert(frame >= 0);

	if (se == NULL || page  < 0 || frame < 0)
		return ERR_PARAM;

	se->flags = SLAB_ENTRY;
	se->frame = frame;
	se->slab_ix = slab_ix;
	se->num_free = slab->num_objs_per_entry;
	assert(se->num_free >= 0);
	se->next_free = 0;

	p = (char *)vpn_to_va(page);

	// Initialize the linked list.
	for (i = 0; i < se->num_free - 1; ++i) {
		*(short *)p = i + 1;
		p += g_slab_sizes[slab_ix];
	}
	*(short *)p = -1;	// Terminate the linked list.

	mutex_lock(&slab->lock);
	slab->num_free += se->num_free;
	list_add_tail(&slab->free_head, &se->entry);
	slab->flags &= ~1;
	cond_var_signal(&slab->wait);
	mutex_unlock(&slab->lock);

	return ERR_SUCCESS;
}

// # of pages in 32GB is 524288 = 0x80000. Thus we need 0x80000 slab_entries
// They consume 0x80000*24 = 0xc00000 bytes of memory, or 192 pages.
// So the first 192 pages of the 32GB area are kept reserved for allocating
// slab_entries.

// Called at IPL_THREAD
int slabs_init(va_t *sys_end)
{
	int i, err;
	size_t size;
	struct slab *slab;
	static char bits[PAGE_SIZE]
		__attribute__((aligned(PAGE_SIZE))) = {0};

	mutex_init(&g_sm.lock);
	g_sm.base_pages[0] = va_to_vpn(SLABS_BASE);
	g_sm.num_pages = SLABS_SIZE >> PAGE_SIZE_BITS;
	err = bitmap_init(&g_sm.map, (void *)bits, g_sm.num_pages);
	if (err)
		return err;

	for (i = 0; i < NUM_SLAB_SIZES; ++i) {
		slab = &g_sm.slabs[i];
		mutex_init(&slab->lock);
		cond_var_init(&slab->wait);
		list_init(&slab->free_head);
		list_init(&slab->part_head);
		slab->num_objs_per_entry =
			divmod(PAGE_SIZE, g_slab_sizes[i], NULL);
	}

	// total # of slab_entries == num_pages.

	size = g_sm.num_pages * sizeof(struct slab_entry);
	size = align_up(size, PAGE_SIZE_BITS);
	g_sm.base_pages[1] = g_sm.base_pages[0] + (size >> PAGE_SIZE_BITS);
	return ERR_SUCCESS;
	(void)sys_end;
}

static
int slabs_ptr_to_se(void *ptr, struct slab_entry **out_se, int *out_obj)
{
	va_t va;
	int obj, err, page_ix, se_page_ix, ix;
	unsigned short slab_ix;
	vpn_t page;
	char *p;
	struct slab *slab;
	struct slab_entry *se;

	if (ptr == NULL)
		return ERR_PARAM;

	va = (va_t)ptr;
	page = va_to_vpn(va);

	// The allocation must be within the data region.
	if (page < g_sm.base_pages[1] || page >= g_sm.base_pages[0] +
	    g_sm.num_pages)
		return ERR_PARAM;

	page_ix = page - g_sm.base_pages[0];
	se_page_ix = page_ix / NUM_SE_PER_PAGE;
	se = (struct slab_entry *)vpn_to_va(g_sm.base_pages[0] + se_page_ix);
	ix = page_ix - se_page_ix * NUM_SE_PER_PAGE;
	se += ix;

	// The page and the corresponding se_page should be allocated.
	mutex_lock(&g_sm.lock);
	err = bitmap_is_on(&g_sm.map, page_ix, 1);
	if (err)
		goto err0;
	err = bitmap_is_on(&g_sm.map, se_page_ix, 1);
	if (err)
		goto err0;

	err = ERR_NOT_FOUND;
	if (se->flags != SLAB_ENTRY)
		goto err0;
	mutex_unlock(&g_sm.lock);

	slab_ix = se->slab_ix;
	slab = &g_sm.slabs[slab_ix];

	err = ERR_PARAM;
	mutex_lock(&slab->lock);
	// Check alignment.
	p = (char *)vpn_to_va(page);
	obj = divmod((char *)ptr - p, g_slab_sizes[slab_ix], NULL);
	if (ptr != p + obj * g_slab_sizes[slab_ix])
		goto err1;
	*out_se = se;
	if (out_obj)
		*out_obj = obj;
	// Returns with slab_lock held.
	return ERR_SUCCESS;
err1:
	mutex_unlock(&slab->lock);
	return err;
err0:
	mutex_unlock(&g_sm.lock);
	return err;

}

// Called at IPL_THREAD
static
int slabs_free(void *ptr)
{
	int obj, err;
	unsigned short slab_ix;
	struct slab *slab;
	struct slab_entry *se;

	err = slabs_ptr_to_se(ptr, &se, &obj);
	if (err)
		return err;

	// slabs_lock is held already.
	slab_ix = se->slab_ix;
	slab = &g_sm.slabs[slab_ix];

	memset(ptr, 0xff, g_slab_sizes[slab_ix]);

	*(short *)ptr = se->next_free;
	se->next_free = obj;
	++se->num_free;
	++slab->num_free;

	if (se->num_free == 1) {
		list_add_tail(&slab->part_head, &se->entry);
	} else if (se->num_free == slab->num_objs_per_entry) {
		list_del_entry(&se->entry);
		list_add_tail(&slab->free_head, &se->entry);
	}
	mutex_unlock(&slab->lock);
	return ERR_SUCCESS;
}

int slabs_va_to_pa(void *ptr, pa_t *out)
{
	int err;
	va_t va;
	pa_t pa;
	struct slab_entry *se;
	struct slab *slab;

	if (ptr == NULL || out == NULL)
		return ERR_PARAM;

	err = slabs_ptr_to_se(ptr, &se, NULL);
	if (err)
		return err;

	slab = &g_sm.slabs[se->slab_ix];

	va = (va_t)ptr;
	pa = pfn_to_pa(se->frame);
	pa |= va & (PAGE_SIZE - 1);
	mutex_unlock(&slab->lock);
	*out = pa;
	return ERR_SUCCESS;
}

static
int slab_se_alloc(int page_ix, struct slab_entry **out)
{
	int se_page_ix, err, ix;
	vpn_t se_page;
	struct slab_entry *se;
	pfn_t se_frame;

	// page_ix == slab_entry index.
	se_page_ix = page_ix / NUM_SE_PER_PAGE;
	se_page = g_sm.base_pages[0] + se_page_ix;

	se = (struct slab_entry *)vpn_to_va(se_page);

	mutex_lock(&g_sm.lock);
	err = bitmap_is_off(&g_sm.map, se_page_ix, 1);
	// The only (true) error we expect is ERR_UNEXP, which is sent when the
	// se is found to be already allocated. Note we do not free the se's
	// yet.
	if (err && err != ERR_UNEXP)
		goto err0;

	// If it is not allocated, allocate.
	if (!err) {
		err = pmm_alloc(ALIGN_PAGE, 1, &se_frame);
		if (err)
			goto err0;
		err = mmu_map_page(0, se_page, se_frame, ALIGN_PAGE, PROT_RW);
		if (err)
			goto err1;
		err = bitmap_on(&g_sm.map, se_page_ix, 1);
		if (err)
			goto err2;
		memset(se, 0, PAGE_SIZE);
	}

	err = ERR_SUCCESS;
	// slab_entry index within the se_page == page_ix % NUM_SE_PER_PAGE;
	ix = page_ix - se_page_ix * NUM_SE_PER_PAGE;
	se += ix;
	*out = se;
	goto err0;
err2:
	mmu_unmap_page(0, se_page);
err1:
	pmm_free(se_frame, 1);
err0:
	mutex_unlock(&g_sm.lock);
	return err;
}

// Called at IPL_THREAD
static
int slab_add_page(unsigned short slab_ix)
{
	vpn_t page;
	pfn_t frame;
	int err, page_ix;
	struct slab_entry *se;

	assert(slab_ix < NUM_SLAB_SIZES);

	if (slab_ix >= NUM_SLAB_SIZES)
		return ERR_PARAM;

	// Allocate a page for the data.
	page_ix = g_sm.base_pages[1] - g_sm.base_pages[0];
	mutex_lock(&g_sm.lock);
	err = page_ix = bitmap_find_off(&g_sm.map, 0, page_ix, 1);
	if (page_ix < 0) {
		mutex_unlock(&g_sm.lock);
		return err;
	}
	err = bitmap_on(&g_sm.map, page_ix, 1);
	mutex_unlock(&g_sm.lock);
	if (err)
		return err;

	page = g_sm.base_pages[0] + page_ix;

	// The page_ix is reserved. Allocate the corresponding slab_entry.
	se = NULL;
	err = slab_se_alloc(page_ix, &se);
	if (err)
		goto err0;
	assert(se);

	err = pmm_alloc(ALIGN_PAGE, 1, &frame);
	if (err)
		goto err1;
	err = mmu_map_page(0, page, frame, ALIGN_PAGE, PROT_RW);
	if (err)
		goto err2;

	err = slab_se_init(slab_ix, se, page, frame);
	if (err)
		goto err3;
	return err;
err3:
	mmu_unmap_page(0, page);
err2:
	pmm_free(frame, 1);
err1:
	// Free the slab_entry
err0:
	mutex_lock(&g_sm.lock);
	bitmap_off(&g_sm.map, page_ix, 1);
	mutex_unlock(&g_sm.lock);
	return err;
}

// Called at IPL_THREAD
static
int slab_alloc_locked(unsigned short slab_ix, void **out)
{
	char *p;
	vpn_t page;
	size_t off;
	short *data;
	struct slab_entry *se;
	struct list_head *head;
	struct slab *slab;
	unsigned short slab_entry_ix;

	slab = &g_sm.slabs[slab_ix];

	head = &slab->part_head;
	if (list_is_empty(head))
		head = &slab->free_head;

	assert(!list_is_empty(head));
	if (list_is_empty(head))
		assert(0);	// Should not occur.

	se = list_entry(head->next, struct slab_entry, entry);
	slab_entry_ix =
		se - (struct slab_entry *)vpn_to_va(g_sm.base_pages[0]);
	page = g_sm.base_pages[0] + slab_entry_ix;

	off = se->next_free * g_slab_sizes[slab_ix];
	p = (char *)vpn_to_va(page);
	data = (short *)(p + off);
	se->next_free = *data;
	--se->num_free;
	--slab->num_free;

	if (se->num_free == 0) {
		list_del_entry(&se->entry);
	} else if (se->num_free == slab->num_objs_per_entry - 1) {
		list_del_entry(&se->entry);
		list_add_tail(&slab->part_head, &se->entry);
	}
	memset(data, 0xff, g_slab_sizes[slab_ix]);
	*out = data;
	return ERR_SUCCESS;
}

// Called at IPL_THREAD
static
int slab_alloc(unsigned short slab_ix, void **out)
{
	int err;
	struct slab *slab;

	assert(slab_ix < NUM_SLAB_SIZES);
	assert(out);

	if (slab_ix >= NUM_SLAB_SIZES || out == NULL)
		return ERR_PARAM;

	slab = &g_sm.slabs[slab_ix];
	mutex_lock(&slab->lock);
	while (1) {
		// Enough units available for us to use.
		if (slab->num_free) {
			assert((slab->flags & 1) == 0);
			err = slab_alloc_locked(slab_ix, out);
			assert(!err);
			mutex_unlock(&slab->lock);
			break;
		}

		// Not enough units available.
		if ((slab->flags & 1) == 0) {
			// No one else working on filling the slab. Fill it.
			slab->flags |= 1;
			mutex_unlock(&slab->lock);

			// Will reset the flag.
			err = slab_add_page(slab_ix);
			if (err)
				return err;

			mutex_lock(&slab->lock);
		} else {
			// Someone else is already working on filling the slab.
			// Wait.
			cond_var_wait(&slab->wait, &slab->lock);
		}
	}
	return err;
}

void *malloc(size_t size)
{
	int i, err;
	void *out;

	for (i = 0; i < NUM_SLAB_SIZES; ++i)
		if (size <= g_slab_sizes[i])
			break;

	assert(i < NUM_SLAB_SIZES);
	if (i == NUM_SLAB_SIZES)
		return NULL;

	out = NULL;
	err = slab_alloc(i, &out);
	if (err) {
		assert(0);
		return NULL;
	}
	return out;
	(void)slabs_free;
}

void free(void *p)
{
	int err;

	if (p == NULL)
		return;

	err = slabs_free(p);
	assert(!err);
}
