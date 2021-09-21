// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_VMM_H
#define SYS_VMM_H

#include <sys/mmu.h>

int	vmm_alloc(enum align_bits align, int num_pages, vpn_t *out);
int	vmm_free(vpn_t page, int num_pages);
#endif
