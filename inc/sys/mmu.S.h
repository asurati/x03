// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_MMU_S_H
#define SYS_MMU_S_H

#define ASM_VA_BASE			0x80000000
#define ASM_RAM_BASE			0
#define ASM_LDR_BASE			(ASM_RAM_BASE + 0x10000)
// #define ASM_LDR_BASE			ASM_RAM_BASE + 0x8000

#define PAGE_SIZE_BITS			16

// Skip the first 4MB of VA and PA space, as it stores the pkg.bin
#define ASM_SYS_BASE			(ASM_VA_BASE + (1 << 22))
#endif
