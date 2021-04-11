// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <stdint.h>

#include <lib/assert.h>
#include <lib/stdlib.h>

#include <sys/bits.h>
#include <sys/elf.h>
#include <sys/err.h>
#include <sys/list.h>
#include <sys/mmu.h>
#include <sys/pmm.h>
#include <sys/slabs.h>

static
int slab_entry_init(struct slab_entry *se, struct slab *slab, pfn_t frame,
		    int num_used)
{
	int i;
	char *p;
	struct list_head *head;

	se->slab = slab;

	se->frame = frame;
	se->num_free = slab->num_objs_per_entry - num_used;
	se->next_free = num_used;
	slab->num_free += se->num_free;

	p = (char *)(frame << PAGE_SIZE_BITS);
	p += num_used * slab->obj_size;

	// Initialize the linked list.
	for (i = num_used; i < se->num_free - 1; ++i) {
		*(short *)p = i + 1;
		p += slab->obj_size;
	}
	*(short *)p = -1;	// Terminate the linked list.

	if (num_used == 0)
		head = &slab->free_head;
	else if (num_used < se->num_free)
		head = &slab->part_head;
	else
		head = NULL;

	if (head)
		list_add_tail(head, &se->entry);

	return pmm_set_ctx(frame, se);
}

int slabs_alloc(struct slab *slab, void **out)
{
	char *frame;
	short *data;
	size_t off;
	struct slab_entry *se;
	struct list_head *head;

	if (slab == NULL || out == NULL)
		return ERR_PARAM;

	mutex_lock(&slab->lock);
	*out = NULL;
	head = &slab->part_head;
	if (list_is_empty(head))
		head = &slab->free_head;
	if (list_is_empty(head)) {
		mutex_unlock(&slab->lock);
		return ERR_NO_MEM;
	}

	se = list_entry(head->next, struct slab_entry, entry);

	off = se->next_free * slab->obj_size;
	frame = (char *)((pa_t)se->frame << PAGE_SIZE_BITS);
	data = (short *)(frame + off);

	se->next_free = *data;
	--se->num_free;
	--slab->num_free;

	if (se->num_free == 0) {
		list_del_entry(&se->entry);
	} else if (se->num_free == slab->num_objs_per_entry - 1) {
		list_del_entry(&se->entry);
		list_add_tail(&slab->part_head, &se->entry);
	}
	*out = data;
	mutex_unlock(&slab->lock);
	return ERR_SUCCESS;
}

int slabs_free(void *p)
{
	pa_t pa;
	int obj, err;
	pfn_t frame;
	char *se_frame;
	struct slab *slab;
	struct slab_entry *se;

	if (p == NULL)
		return ERR_PARAM;

	pa = (pa_t)p;
	frame = pa_to_pfn(pa);

	se = NULL;
	err = pmm_get_ctx(frame, (void **)&se);
	if (err) return err;

	if (se->frame != frame)
		return ERR_UNEXP;
	slab = se->slab;

	se_frame = (char *)(se->frame << PAGE_SIZE_BITS);

	// Check alignment.
	obj = ((char *)p - se_frame) / slab->obj_size;
	if (p != se_frame + obj * slab->obj_size)
		return ERR_PARAM;

	mutex_lock(&slab->lock);
	*(short *)p = se->next_free;
	se->next_free = obj;
	++se->num_free;
	++slab->num_free;

	// We do not go from full to free directly.
	if (se->num_free == 1) {
		list_add_tail(&slab->part_head, &se->entry);
	} else if (se->num_free == slab->num_objs_per_entry) {
		list_del_entry(&se->entry);
		list_add_tail(&slab->free_head, &se->entry);
	}
	mutex_unlock(&slab->lock);
	return ERR_SUCCESS;
}

int slabs_init(struct slab *slabs, const size_t *obj_sizes, int len)
{
	int i;
	struct slab *slab;

	if (slabs == NULL || len <= 0)
		return ERR_PARAM;

	// TODO check for duplicates, etc.
	for (i = 0; i < len; ++i) {
		slab = &slabs[i];

		slab->obj_size = obj_sizes[i];
		slab->num_objs_per_entry = PAGE_SIZE / obj_sizes[i];
		slab->num_free = 0;

		list_init(&slab->free_head);
		list_init(&slab->part_head);
		mutex_init(&slab->lock);
	}
	return ERR_SUCCESS;
}

int slabs_add_frame(struct slab *slab, struct slab *se_slab, pfn_t frame)
{
	pa_t pa;
	int err, num_used;
	struct slab_entry *se;

	if (slab == NULL || se_slab == NULL || frame < 0)
		return ERR_PARAM;

	num_used = 0;
	se = NULL;
	err = slabs_alloc(se_slab, (void **)&se);
	if (err == ERR_NO_MEM && slab == se_slab) {
		pa = (pa_t)frame << PAGE_SIZE_BITS;
		se = (struct slab_entry *)pa;
		num_used = 1;
	} else if (err) {
		return err;
	}

	mutex_lock(&slab->lock);
	err = slab_entry_init(se, slab, frame, num_used);
	mutex_unlock(&slab->lock);
	return err;
}

int slabs_num_free(struct slab *slab)
{
	int ret;
	if (slab == NULL) return ERR_PARAM;
	mutex_lock(&slab->lock);
	ret = slab->num_free;
	mutex_unlock(&slab->lock);
	return ret;
}
