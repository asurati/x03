// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef DEV_V3D_H
#define DEV_V3D_H

#include <stddef.h>

#include <sys/mmu.h>

// Indices of the registers
#define V3D_IDENT0			(0x0 >> 2)
#define V3D_IDENT1			(0x4 >> 2)
#define V3D_IDENT2			(0x8 >> 2)
#define V3D_SCRATCH			(0x10 >> 2)
#define V3D_L2CACTL			(0x20 >> 2)
#define V3D_SLCACTL			(0x24 >> 2)
#define V3D_INTCTL			(0x30 >> 2)
#define V3D_INTENA			(0x34 >> 2)
#define V3D_INTDIS			(0x38 >> 2)

#define V3D_SRQPC			(0x430 >> 2)
#define V3D_SRQUA			(0x434 >> 2)
#define V3D_SRQUL			(0x438 >> 2)
#define V3D_SRQCS			(0x43c >> 2)

#define V3D_VPACNTL			(0x500 >> 2)
#define V3D_VPMBASE			(0x504 >> 2)

#define V3D_DBCFG			(0xe00 >> 2)
#define V3D_DBQITE			(0xe2c >> 2)
#define V3D_DBQITC			(0xe30 >> 2)

#define V3D_DBCFG_QITENA_POS		0
#define V3D_DBCFG_QITENA_BITS		1

#define V3D_SRQCS_NUM_DONE_POS		16
#define V3D_SRQCS_NUM_DONE_BITS		8

int	v3d_run_prog(pa_t code_pa, pa_t unif_pa, size_t unif_size);
#endif
