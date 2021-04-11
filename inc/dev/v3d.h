// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef DEV_V3D_H
#define DEV_V3D_H

#include <stddef.h>

#include <sys/mmu.h>

#define V3D_IDENT0			(void *)(IO_BASE + 0xc00000)
#define V3D_L2CACTL			(void *)(IO_BASE + 0xc00020)
#define V3D_SLCACTL			(void *)(IO_BASE + 0xc00024)

#define V3D_INTCTL			(void *)(IO_BASE + 0xc00030)
#define V3D_INTENA			(void *)(IO_BASE + 0xc00034)
#define V3D_INTDIS			(void *)(IO_BASE + 0xc00038)

#define V3D_CT0CS			(void *)(IO_BASE + 0xc00100)
#define V3D_CT1CS			(void *)(IO_BASE + 0xc00104)
#define V3D_CT0EA			(void *)(IO_BASE + 0xc00108)
#define V3D_CT1EA			(void *)(IO_BASE + 0xc0010c)
#define V3D_CT0CA			(void *)(IO_BASE + 0xc00110)
#define V3D_CT1CA			(void *)(IO_BASE + 0xc00114)
#define V3D_CT0RA			(void *)(IO_BASE + 0xc00118)
#define V3D_CT1RA			(void *)(IO_BASE + 0xc0011c)
#define V3D_CT0LC			(void *)(IO_BASE + 0xc00120)
#define V3D_CT1LC			(void *)(IO_BASE + 0xc00124)
#define V3D_CT0PC			(void *)(IO_BASE + 0xc00128)
#define V3D_CT1PC			(void *)(IO_BASE + 0xc0012c)
#define V3D_PCS				(void *)(IO_BASE + 0xc00130)
#define V3D_BFC				(void *)(IO_BASE + 0xc00134)
#define V3D_RFC				(void *)(IO_BASE + 0xc00138)

#define V3D_BPCA			(void *)(IO_BASE + 0xc00300)
#define V3D_BPCS			(void *)(IO_BASE + 0xc00304)
#define V3D_BPOA			(void *)(IO_BASE + 0xc00308)
#define V3D_BPOS			(void *)(IO_BASE + 0xc0030c)
#define V3D_BXCF			(void *)(IO_BASE + 0xc00310)

#define V3D_SQRSV1			(void *)(IO_BASE + 0xc00414)
#define V3D_SRQPC			(void *)(IO_BASE + 0xc00430)
#define V3D_SRQUA			(void *)(IO_BASE + 0xc00434)
#define V3D_SRQUL			(void *)(IO_BASE + 0xc00438)
#define V3D_SRQCS			(void *)(IO_BASE + 0xc0043c)
#define V3D_VPMBASE			(void *)(IO_BASE + 0xc00504)
#define V3D_DBCFG			(void *)(IO_BASE + 0xc00e00)
#define V3D_DBQITE			(void *)(IO_BASE + 0xc00e2c)
#define V3D_DBQITC			(void *)(IO_BASE + 0xc00e30)

#define V3D_DBGE			(void *)(IO_BASE + 0xc00f00)
#define V3D_ERRSTAT			(void *)(IO_BASE + 0xc00f20)

#define V3D_IDENT0_VAL			0x2443356

#define V3D_L2CACTL_CLEAR_POS		2
#define V3D_L2CACTL_CLEAR_BITS		1

#define V3D_SLCACTL_IC_CLEAR_POS	0
#define V3D_SLCACTL_UC_CLEAR_POS	8
#define V3D_SLCACTL_TMU0C_CLEAR_POS	16
#define V3D_SLCACTL_TMU1C_CLEAR_POS	24
#define V3D_SLCACTL_IC_CLEAR_BITS	4
#define V3D_SLCACTL_UC_CLEAR_BITS	4
#define V3D_SLCACTL_TMU0C_CLEAR_BITS	4
#define V3D_SLCACTL_TMU1C_CLEAR_BITS	4

#define V3D_SRQCS_QLEN_POS		0
#define V3D_SRQCS_QERR_POS		7
#define V3D_SRQCS_NUM_MADE_POS		8
#define V3D_SRQCS_NUM_DONE_POS		16
#define V3D_SRQCS_QLEN_BITS		6
#define V3D_SRQCS_QERR_BITS		1
#define V3D_SRQCS_NUM_MADE_BITS		8
#define V3D_SRQCS_NUM_DONE_BITS		8

#define V3D_DBCFG_QITENA_POS		0
#define V3D_DBCFG_QITENA_BITS		1

#define V3D_DBQITE_ALL_POS		0
#define V3D_DBQITE_ALL_BITS		16

#define V3D_DBQITC_ALL_POS		0
#define V3D_DBQITC_ALL_BITS		16

#define V3D_VPMBASE_URSV_POS		0
#define V3D_VPMBASE_URSV_BITS		5

// Table 32: VPMVCD_WR_SETUP with write from QPU into VPM
// Table 33: VPMVCD_RD_SETUP with read from VPM into QPU
#define VPM_ADDR_POS			0
#define VPM_SIZE_POS			8
#define VPM_LANED_POS			10
#define VPM_HORIZ_POS			11
#define VPM_STRIDE_POS			12
#define VPM_NUM_POS			20
#define VPM_ID_POS			30

#define VPM_ADDR_BITS			8
#define VPM_SIZE_BITS			2
#define VPM_LANED_BITS			1
#define VPM_HORIZ_BITS			1
#define VPM_STRIDE_BITS			6
#define VPM_NUM_BITS			4
#define VPM_ID_BITS			2

// Table 36: VPMVCD_RD_SETUP with VPM DMA Load
#define VDR_ADDRX_POS			0
#define VDR_ADDRY_POS			4
#define VDR_VERT_POS			11
#define VDR_VPITCH_POS			12
#define VDR_NROWS_POS			16
#define VDR_ROWLEN_POS			20
#define VDR_MPITCH_POS			24
#define VDR_MODEW_POS			28
#define VDR_ID_POS			31

#define VDR_ADDRX_BITS			4
#define VDR_ADDRY_BITS			7
#define VDR_VERT_BITS			1
#define VDR_VPITCH_BITS			4
#define VDR_NROWS_BITS			4
#define VDR_ROWLEN_BITS			4
#define VDR_MPITCH_BITS			4
#define VDR_MODEW_BITS			3
#define VDR_ID_BITS			1

#define VDR_ID_VAL			1

// Table 37: VDR extended memory stride setup format.
#define VDRE_MPITCHB_POS		0
#define VDRE_ID_POS			28
#define VDRE_MPITCHB_BITS		13
#define VDRE_ID_BITS			4

#define VDRE_ID_VAL			9

#define VDW_MODEW_POS			0
#define VDW_VPMBASE_POS			3
#define VDW_HORIZ_POS			14
#define VDW_DEPTH_POS			16
#define VDW_UNITS_POS			23
#define VDW_ID_POS			30

#define VDW_MODEW_BITS			3
#define VDW_VPMBASE_BITS		11
#define VDW_HORIZ_BITS			1
#define VDW_DEPTH_BITS			7
#define VDW_UNITS_BITS			7
#define VDW_ID_BITS			2

#define VDW_ID_VAL			2

#define V3D_CTCS_CTRUN_POS		5
#define V3D_CTCS_CTRSTA_POS		15
#define V3D_CTCS_CTRUN_BITS		1
#define V3D_CTCS_CTRSTA_BITS		1

#define CRID_HALT			0
#define CRID_FLUSH			4
#define CRID_START_TILE_BINNING		6
#define CRID_BRANCH_SUBLIST		17
#define CRID_STORE_COLOR		24
#define CRID_STORE_COLOR_EOF		25
#define CRID_STORE_TILE_BUF_GEN		28
#define CRID_INDEXED_PRIMITIVE_LIST	32
#define CRID_VERTEX_ARRAY_PRIMITIVES	33
#define CRID_PRIMITIVE_LIST_FORMAT	56
#define CRID_GL_SHADER_STATE		64
#define CRID_NV_SHADER_STATE		65
#define CRID_CONFIG_BITS		96
#define CRID_CLIP_WINDOW		102
#define CRID_VIEWPORT_OFFSET		103
#define CRID_CLIPPER_XY_SCALING		105
#define CRID_TILE_BINNING_MODE_CONFIG	112
#define CRID_TILE_RENDERING_MODE_CONFIG	113
#define CRID_CLEAR_COLORS		114
#define CRID_TILE_COORDINATES		115

struct cr_branch_sublist {
	unsigned char			id;
	unsigned int			br_addr;
} __attribute__((packed));

struct cr_store_tile_buf_gen {
	unsigned char			id;
	unsigned short			flags;
	unsigned int			tile_buf_addr;
} __attribute__((packed));

struct cr_indexed_primitive_list {
	unsigned char			id;
	unsigned char			flags;
	unsigned int			num_indices;
	unsigned int			indices_addr;
	unsigned int			max_index;
} __attribute__((packed));

struct cr_vertex_array_primitives {
	unsigned char			id;
	unsigned char			mode;
	unsigned int			num_vertices;
	unsigned int			index_first_vertex;
} __attribute__((packed));

struct cr_primitive_list_format {
	unsigned char			id;
	unsigned char			flags;
} __attribute__((packed));

struct cr_gl_shader_state {
	unsigned char			id;
	unsigned int			shader_rec_addr;
} __attribute__((packed));

struct cr_nv_shader_state {
	unsigned char			id;
	unsigned int			shader_rec_addr;
} __attribute__((packed));

struct nv_shader_state_record {
	unsigned char			flags;
	unsigned char			stride;
	unsigned char			num_uniforms;
	unsigned char			num_varyings;
	unsigned int			code_addr;
	unsigned int			uniforms_addr;
	unsigned int			vertices_addr;
} __attribute__((packed));

struct cr_config_bits {
	unsigned char			id;
	unsigned char			flags[3];
} __attribute__((packed));

struct cr_clip_window {
	unsigned char			id;

	// In pixel units.
	unsigned short			left;
	unsigned short			bottom;
	unsigned short			width;
	unsigned short			height;
} __attribute__((packed));

struct cr_viewport_offset {
	unsigned char			id;

	// In pixel units.
	short				center_x;
	short				center_y;
} __attribute__((packed));

struct cr_clipping_xy_scaling {
	unsigned char			id;

	// In units of pixel/16.
	float				half_width;
	float				half_height;
} __attribute__((packed));

struct cr_tile_binning_mode_config {
	unsigned char			id;

	// PTB working memory. Consumes 8KB. If auto-init TSDA is set in the
	// flags, consumes an additional 4KB. The start of the buffer contains
	// the rendering control list created by the PTB - 32 bytes per tile.

	unsigned int			tile_alloc_addr;
	unsigned int			tile_alloc_size;

	// Contains the tile state for each tile. Filled by PTB. 48 bytes per
	// tile.

	unsigned int			tile_state_addr;

	// Width and Height in the units of tiles.
	unsigned char			width;
	unsigned char			height;

	// Keep default as 0 for now.
	unsigned char			flags;
} __attribute__((packed));

struct cr_tile_rendering_mode_config {
	unsigned char			id;

	unsigned int			tile_buf_addr;

	// In units of pixels.
	unsigned short			width;
	unsigned short			height;

	unsigned short			flags;
} __attribute__((packed));

struct cr_clear_colors {
	unsigned char			id;

	unsigned int			clear_color[2];
	unsigned char			clear_zs[3];
	unsigned char			clear_vg_mask;
	unsigned char			clear_stencil;
} __attribute__((packed));

struct cr_tile_coordinates {
	unsigned char			id;

	unsigned char			tile_row;
	unsigned char			tile_col;
} __attribute__((packed));
#endif
