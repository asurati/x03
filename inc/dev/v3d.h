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

#define V3D_CT0CS			(0x100 >> 2)
#define V3D_CT0EA			(0x108 >> 2)
#define V3D_CT0CA			(0x110 >> 2)
#define V3D_CT0LC			(0x120 >> 2)
#define V3D_CT0PC			(0x128 >> 2)

#define V3D_CT1CS			(0x104 >> 2)
#define V3D_CT1EA			(0x10c >> 2)
#define V3D_CT1CA			(0x114 >> 2)
#define V3D_CT1LC			(0x124 >> 2)
#define V3D_CT1PC			(0x12c >> 2)

#define V3D_PCS				(0x130 >> 2)
#define V3D_BFC				(0x134 >> 2)
#define V3D_RFC				(0x138 >> 2)

#define V3D_BPCA			(0x300 >> 2)
#define V3D_BPCS			(0x304 >> 2)
#define V3D_BPOA			(0x308 >> 2)
#define V3D_BPOS			(0x30c >> 2)

#define V3D_SQRSV0			(0x410 >> 2)
#define V3D_SQRSV1			(0x414 >> 2)
#define V3D_SRQPC			(0x430 >> 2)
#define V3D_SRQUA			(0x434 >> 2)
#define V3D_SRQUL			(0x438 >> 2)
#define V3D_SRQCS			(0x43c >> 2)

#define V3D_VPACNTL			(0x500 >> 2)
#define V3D_VPMBASE			(0x504 >> 2)

#define V3D_DBCFG			(0xe00 >> 2)
#define V3D_DBQITE			(0xe2c >> 2)
#define V3D_DBQITC			(0xe30 >> 2)

#define V3D_DBGE			(0xf00 >> 2)
#define V3D_ERRSTAT			(0xf20 >> 2)
#define V3D_ERRSTAT			(0xf20 >> 2)
#define V3D_ERRSTAT			(0xf20 >> 2)
#define V3D_ERRSTAT			(0xf20 >> 2)
#define V3D_ERRSTAT			(0xf20 >> 2)
#define V3D_ERRSTAT			(0xf20 >> 2)
#define V3D_ERRSTAT			(0xf20 >> 2)

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
// A single setup works for multiple writes because of the STRIDE property.
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

struct v3dcr_tile_binning_mode {
	uint8_t				id;		// 112
	uint32_t			ta_base;
	uint32_t			ta_size;
	uint32_t			tsda_base;
	uint8_t				width;
	uint8_t				height;
	uint8_t				flags;
} __attribute__((packed));

struct v3dcr_tile_binning_start {
	uint8_t				id;		// 6
} __attribute__((packed));

struct v3dcr_clip_window {
	uint8_t				id;		// 102
	uint16_t			left;
	uint16_t			bottom;
	uint16_t			width;
	uint16_t			height;
} __attribute__((packed));

struct v3dcr_config_bits {
	uint8_t				id;		// 96
	uint8_t				flags[3];
} __attribute__((packed));

struct v3dcr_viewport_offset {
	uint8_t				id;		// 103
	int16_t				x;
	int16_t				y;
} __attribute__((packed));

struct v3dcr_clipper_xy_scale {
	uint8_t				id;		// 105
	float				half_width;
	float				half_height;
} __attribute__((packed));

struct v3dcr_shader_state {
	uint8_t				id;		// gl=64, nv=65
	uint32_t			ssr_base;
} __attribute__((packed));

struct v3dcr_vert_array {
	uint8_t				id;		// 33
	uint8_t				mode;
	uint32_t			num_verts;
	uint32_t			index;
} __attribute__((packed));

struct v3dcr_flush {
	uint8_t				id;		// 4
} __attribute__((packed));

struct v3dcr_sema {
	uint8_t				id;		// inc=7, wt=8
} __attribute__((packed));

struct v3dcr_gl_attr {
	uint32_t			base;
	uint8_t				num_bytes;
	uint8_t				stride;
	uint8_t				vs_vpm_off;
	uint8_t				cs_vpm_off;
} __attribute__((packed));

struct v3dcr_gl_shader_state_rec {
	uint16_t			flags;

	uint8_t				fs_num_unif;
	uint8_t				fs_num_vary;
	uint32_t			fs_code_addr;
	uint32_t			fs_unif_addr;

	uint16_t			vs_num_unif;
	uint8_t				vs_attr_sel;
	uint8_t				vs_attr_size;
	uint32_t			vs_code_addr;
	uint32_t			vs_unif_addr;

	uint16_t			cs_num_unif;
	uint8_t				cs_attr_sel;
	uint8_t				cs_attr_size;
	uint32_t			cs_code_addr;
	uint32_t			cs_unif_addr;

	struct v3dcr_gl_attr		attr[8];
} __attribute__((packed));

struct v3dcr_nv_shader_state_rec {
	uint8_t				flags;
	uint8_t				stride;
	uint8_t				num_unif;
	uint8_t				num_vary;
	uint32_t			code_addr;
	uint32_t			unif_addr;
	uint32_t			verts_addr;
} __attribute__((packed));

struct v3dcr_clear_colours {
	uint8_t				id;		// 114
	uint32_t			colour[2];
	uint32_t			mask;
	uint8_t				stencil;
} __attribute__((packed));

struct v3dcr_tile_rendering_mode {
	uint8_t				id;		// 113
	uint32_t			tb_base;
	uint16_t			width;
	uint16_t			height;
	uint16_t			flags;
} __attribute__((packed));

struct v3dcr_tile_coords {
	uint8_t				id;		// 115
	uint8_t				col;
	uint8_t				row;
} __attribute__((packed));

struct v3dcr_branch {
	uint8_t				id;		// 17
	uint32_t			addr;
} __attribute__((packed));

struct v3dcr_store_tb_gen {
	uint8_t				id;		// 28
	uint8_t				flags[2];
	uint32_t			base;
} __attribute__((packed));

struct v3dcr_store_mstcb {
	uint8_t				id;		// 24; 25 with EOF
} __attribute__((packed));

int	v3d_run_prog(ba_t code_ba, ba_t unif_ba, size_t unif_size);
void	v3d_run_binner(ba_t cr, size_t size);
void	v3d_run_renderer(ba_t cr, size_t size);
#endif
