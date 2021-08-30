// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef DEV_V3D_H
#define DEV_V3D_H

#include <stddef.h>

#include <sys/bits.h>
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

// VDR: Read from system RAM into VPM.
// VPITCH = VPM Pitch
// MPITCH = Memory Pitch
// The Y resolution is 7 bits, perhaps to support 8KB of VPM memory.
#define VDR_ADDR_X_POS			0
#define VDR_ADDR_Y_POS			4
#define VDR_VERT_POS			11
#define VDR_VPITCH_POS			12
#define VDR_NROWS_POS			16
#define VDR_ROWLEN_POS			20
#define VDR_MPITCH_POS			24
#define VDR_ID_POS			31
#define VDR_ADDR_X_BITS			4
#define VDR_ADDR_Y_BITS			7
#define VDR_VERT_BITS			1
#define VDR_VPITCH_BITS			4
#define VDR_NROWS_BITS			4
#define VDR_ROWLEN_BITS			4
#define VDR_MPITCH_BITS			4
#define VDR_ID_BITS			1

// VDW: Write into system RAM from VPM
// This operation always writes one full 16-element vector.
// If UNITS != 1, the single vector is duplicated UNIT number of times.
// There is no auto VPM-address increment facility
#define VDW_ADDR_X_POS			3
#define VDW_ADDR_Y_POS			7
#define VDW_HORIZ_POS			14
#define VDW_DEPTH_POS			16	// In the units of width
#define VDW_UNITS_POS			23
#define VDW_ID_POS			30
#define VDW_ADDR_X_BITS			4
#define VDW_ADDR_Y_BITS			7
#define VDW_HORIZ_BITS			1
#define VDW_DEPTH_BITS			7
#define VDW_UNITS_BITS			7
#define VDW_ID_BITS			2

// VPR: Read from VPM into QPU.
// This operation always reads NUM full 16-element vectors.
#define VPR_V32_ADDR_X_POS		0
#define VPR_V32_ADDR_Y_POS		4
#define VPR_SIZE_POS			8
#define VPR_HORIZ_POS			11
#define VPR_STRIDE_POS			12
#define VPR_NUM_POS			20
#define VPR_V32_ADDR_X_BITS		4
#define VPR_V32_ADDR_Y_BITS		2
#define VPR_SIZE_BITS			2
#define VPR_HORIZ_BITS			1
#define VPR_STRIDE_BITS			6
#define VPR_NUM_BITS			4

// VPW: Write into VPM from QPU.
// This operation always writes one full 16-element vector.
#define VPW_V32_ADDR_X_POS		VPR_V32_ADDR_X_POS
#define VPW_V32_ADDR_Y_POS		VPR_V32_ADDR_Y_POS
#define VPW_SIZE_POS			VPR_SIZE_POS
#define VPW_HORIZ_POS			VPR_HORIZ_POS
#define VPW_STRIDE_POS			VPR_STRIDE_POS
#define VPW_V32_ADDR_X_BITS		VPR_V32_ADDR_X_BITS
#define VPW_V32_ADDR_Y_BITS		VPR_V32_ADDR_Y_BITS
#define VPW_SIZE_BITS			VPR_SIZE_BITS
#define VPW_HORIZ_BITS			VPR_HORIZ_BITS
#define VPW_STRIDE_BITS			VPR_STRIDE_BITS

int	v3d_run_prog(ba_t code_ba, ba_t unif_ba, size_t unif_size);
#endif
