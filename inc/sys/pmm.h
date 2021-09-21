// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_PMM_H
#define SYS_PMM_H

#include <sys/mmu.h>

int	pmm_alloc(enum align_bits align, int num_frames, pfn_t *out);
int	pmm_free(pfn_t frame, int num_frames);
int	pmm_get_ctx(pfn_t frame, void **ctx);
int	pmm_set_ctx(pfn_t frame, void *ctx);
int	pmm_get_num_frames();
#endif
