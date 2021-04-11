// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_SLABS_H
#define SYS_SLABS_H

#include <stdint.h>

#include <sys/list.h>
#include <sys/mmu.h>
#include <sys/mutex.h>

struct slab {
	struct list_head		free_head;
	struct list_head		part_head;
	struct mutex			lock;
	size_t				obj_size;
	int				num_objs_per_entry;
	int				num_free;
};

// Each entry handles 1 64KB frame.
struct slab_entry {
	struct list_head		entry;
	pfn_t				frame;
	struct slab			*slab;
	int				next_free;
	int				num_free;
};

int	slabs_alloc(struct slab *slab, void **out);
int	slabs_free(void *p);
int	slabs_init(struct slab *slabs, const size_t *obj_sizes, int len);
int	slabs_add_frame(struct slab *slab, struct slab *se_slab, pfn_t frame);
int	slabs_num_free(struct slab *slab);
#endif
