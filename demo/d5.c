// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdlib.h>
#include <lib/string.h>

#include <sys/cpu.h>
#include <sys/err.h>

#include <dev/v3d.h>
#include <dev/con.h>

// Assuming a 640 x 480 framebuffer size.

#define NUM_TILES_X			10
#define NUM_TILES_Y			7

#define WIDTH				(NUM_TILES_X * 64)
#define HEIGHT				(NUM_TILES_Y * 64)

// The Shaded Coordinates Format is the format in which a coordinate shader
// outputs. Since we want an empty coordinate shader, we set the input of
// the shader to be the same format as well. Similar siuation with vertex
// shader.

struct vs_vertex {
	int16_t				xs;
	int16_t				ys;
	float				zs;
	float				wcr;
} __attribute__((packed));

struct cs_vertex {
	float				xc;
	float				yc;
	float				zc;
	float				wc;

	int16_t				xs;
	int16_t				ys;
	float				zs;
	float				wcr;
} __attribute__((packed));

#if 0
// In screen coordinates. x and y in 12.4 fixed point format.
static const struct vs_vertex vs_verts[] __attribute__((aligned(32))) = {
	{320 << 4, 292 << 4, 2.5, 0.5},
	{265 << 4, 187 << 4, 2.5, 0.5},
	{374 << 4, 187 << 4, 2.5, 0.5},
};

static const struct cs_vertex cs_verts[] __attribute__((aligned(32))) = {
	{0, 0.44, 1, 2, 320 << 4, 292 << 4, 2.5, 0.5},
	{-0.34, -0.44, 1, 2, 265 << 4, 187 << 4, 2.5, 0.5},
	{0.34, -0.44, 1, 2, 374 << 4, 187 << 4, 2.5, 0.5},
};
#endif
static const struct vs_vertex vs_verts[] __attribute__((aligned(32))) = {
	{320 << 4, (346 << 4) | 9, 1.0, 1.0},
	{(211 << 4) | 3, (134 << 4) | 6, 1.0, 1.0},
	{(428 << 4) | 11, (134 << 4) | 6, 1.0, 1.0},
};

static const struct cs_vertex cs_verts[] __attribute__((aligned(32))) = {
	{0.0, 0.44,	-1.0, 1.0,	320 << 4, (345 << 4) | 9, 1.0, 1.0},
	{-0.34, -0.44,	-1.0, 1.0,	(211 << 4) | 3, (134 << 4) | 6, 1.0, 1.0},
	{0.34, -0.44,	-1.0, 1.0,	(428 << 4) | 11, (134 << 4) | 6, 1.0, 1.0},
};

int d5_run()
{
	int err, off, x, y;
	va_t tva;
	pa_t	fb_get_pa();

	struct v3dcr_tile_binning_mode		*tbmc;
	struct v3dcr_tile_binning_start		*tbs;
	struct v3dcr_clip_window		*cw;
	struct v3dcr_config_bits		*cb;
	struct v3dcr_viewport_offset		*vo;
	struct v3dcr_clipper_xy_scale		*cxy;
	struct v3dcr_shader_state		*ss;
	struct v3dcr_vert_array			*va;
	struct v3dcr_flush			*f;
	struct v3dcr_sema			*sem;

	struct v3dcr_clear_colours		*cc;
	struct v3dcr_tile_rendering_mode	*trmc;
	struct v3dcr_tile_coords		*tc;
	struct v3dcr_branch			*br;
	struct v3dcr_store_mstcb		*str;
	struct v3dcr_store_tb_gen		*stg;

	static char v3dcr[PAGE_SIZE] __attribute__((aligned(32)));

	// Alignment enforced by TBMC/112.
	static uint32_t tsda[(NUM_TILES_Y * NUM_TILES_X * 48) >> 2]
		__attribute__((aligned(32)));

	// Alignment seems to be enfored by the max. tile allocation block
	// size of 256 bytes. This buffer is the binning memory pool
	// BPCA/BPCS.
	static uint32_t ta[PAGE_SIZE >> 2] __attribute__((aligned(256)));

	// Alignment enforced by NVSS/65.
	static struct v3dcr_gl_shader_state_rec	ssr
		__attribute__((aligned(32)));

	static const uint32_t cs_code[] __attribute__((aligned(8))) = {
		0x00701a00, 0xe0020c67,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
		0x15c00dc0, 0xd0020027,
		0x15c00dc0, 0xd0020067,
		0x15c00dc0, 0xd00200a7,
		0x15c00dc0, 0xd00200e7,
		0x15c00dc0, 0xd0020127,
		0x15c00dc0, 0xd0020167,
		0x15c00dc0, 0xd00201a7,
		0x00001a00, 0xe0021c67,
		0x15027d80, 0x10020c27,
		0x15067d80, 0x10020c27,
		0x150a7d80, 0x10020c27,
		0x150e7d80, 0x10020c27,
		0x15127d80, 0x10020c27,
		0x15167d80, 0x10020c27,
		0x151a7d80, 0x10020c27,
		0x159c1fc0, 0xd00209a7,
		0x009e7000, 0x300009e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
	};

	static const uint32_t vs_code[] __attribute__((aligned(8))) = {
		0x00301a00, 0xe0020c67,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
		0x15c00dc0, 0xd0020027,
		0x15c00dc0, 0xd0020067,
		0x15c00dc0, 0xd00200a7,
		0x00001a00, 0xe0021c67,
		0x15027d80, 0x10020c27,
		0x15067d80, 0x10020c27,
		0x150a7d80, 0x10020c27,
		0x159c1fc0, 0xd00209a7,
		0x009e7000, 0x300009e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
	};

	static const uint32_t fs_code[] __attribute__((aligned(8))) = {
		0x009e7000, 0x400009e7,
		0xffff0000, 0xe0020ba7,
		0x009e7000, 0x500009e7,
		0x159c1fc0, 0xd00209a7,
		0x009e7000, 0x300009e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
#if 0
		0x018c0dc0, 0xd0020827,
		0x019e7140, 0x10020827,
		0x018c0dc0, 0xd0020867,
		0x019e7340, 0x10020867,
		0x018c0dc0, 0xd00208a7,
		0x019e7540, 0x100208a7,

		0x000000ff, 0xe00248e7,
		0x089e76c0, 0x100208e7,
		0x209e7003, 0x100049e0,
		0x209e700b, 0x100049e1,
		0x209e7013, 0x100049e2,

		0x079e7000, 0x10020827,
		0x079e7240, 0x10020867,
		0x079e7480, 0x100208a7,

		0x119c81c0, 0xd0020827,
		0x119c81c0, 0xd0020827,
		0x119c83c0, 0xd0020867,

		0x159e7040, 0x10020827,
		0x159e7080, 0x10020827,

		0xff000000, 0xe0024867,
		0x159e7040, 0x10020827,
		0x159e7000, 0x50020ba7,

		0x159c1fc0, 0xd00209a7,
		0x009e7000, 0x300009e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
#endif
	};

	off = 0;
	memset(v3dcr, 0, sizeof(v3dcr));

	tbmc = (struct v3dcr_tile_binning_mode *)&v3dcr[off];
	off += sizeof(*tbmc);

	tbs = (struct v3dcr_tile_binning_start *)&v3dcr[off];
	off += sizeof(*tbs);

	sem = (struct v3dcr_sema *)&v3dcr[off];
	off += sizeof(*sem);

	cw = (struct v3dcr_clip_window *)&v3dcr[off];
	off += sizeof(*cw);

	cb = (struct v3dcr_config_bits *)&v3dcr[off];
	off += sizeof(*cb);

	vo = (struct v3dcr_viewport_offset *)&v3dcr[off];
	off += sizeof(*vo);

	cxy = (struct v3dcr_clipper_xy_scale *)&v3dcr[off];
	off += sizeof(*cxy);

	ss = (struct v3dcr_shader_state *)&v3dcr[off];
	off += sizeof(*ss);

	va = (struct v3dcr_vert_array *)&v3dcr[off];
	off += sizeof(*va);

	f = (struct v3dcr_flush *)&v3dcr[off];
	off += sizeof(*f);

	tbmc->id = 112;
	tbmc->ta_base = va_to_ba((va_t)ta);
	tbmc->ta_size = sizeof(ta);
	tbmc->tsda_base = va_to_ba((va_t)tsda);
	tbmc->width = NUM_TILES_X;
	tbmc->height = NUM_TILES_Y;
	tbmc->flags = 4;	// Auto-init tsda. Necessary.

	tbs->id = 6;

	sem->id = 7;

	cw->id = 102;
	cw->width = WIDTH;
	cw->height = HEIGHT;

	cb->id = 96;
	cb->flags[0] = 3;

	// The viewport offset coordinates are in signed 12.4 fixed point
	// format.
	vo->id = 103;

	cxy->id = 105;
	cxy->half_width = (WIDTH / 2) * 16.0;
	cxy->half_height = (HEIGHT / 2) * 16.0;

	ss->id = 64;
	ss->ssr_base = va_to_ba((va_t)&ssr);
	ss->ssr_base |= 2;	// # of attribute arrays.

	va->id = 33;
	va->mode = 4;
	va->num_verts = 3;

	f->id = 4;

	memset(&ssr, 0, sizeof(ssr));
	ssr.flags = 5;
	ssr.fs_code_addr = va_to_ba((va_t)fs_code);
	ssr.cs_attr_sel = 1;
	ssr.cs_attr_size = sizeof(struct cs_vertex);
	ssr.cs_code_addr = va_to_ba((va_t)cs_code);
	ssr.vs_attr_sel = 2;
	ssr.vs_attr_size = sizeof(struct vs_vertex);
	ssr.vs_code_addr = va_to_ba((va_t)vs_code);
	ssr.attr[0].base = va_to_ba((va_t)cs_verts);
	ssr.attr[0].num_bytes = sizeof(struct cs_vertex) - 1;
	ssr.attr[0].stride = sizeof(struct cs_vertex);
	ssr.attr[1].base = va_to_ba((va_t)vs_verts);
	ssr.attr[1].num_bytes = sizeof(struct vs_vertex) - 1;
	ssr.attr[1].stride = sizeof(struct vs_vertex);

	dc_cvac(v3dcr, off);
	dc_cvac(&ssr, sizeof(ssr));
	dsb();

	v3d_run_binner(va_to_ba((va_t)v3dcr), off);

#if 0
	dc_ivac(tsda, sizeof(tsda));
	dc_ivac(ta, sizeof(ta));
	dsb();

	for (i = 0; i < (int)(sizeof(tsda) >> 2); ++i) {
		val = *(volatile uint32_t *)&tsda[i];
		if (val)
			con_out("tsda[%x] = %x", i, val);
	}

	for (i = 0; i < (int)(sizeof(ta) >> 2); ++i) {
		val = *(volatile uint32_t *)&ta[i];
		if (val)
			con_out("ta[%x] = %x", i, val);
	}
#endif

	off = 0;
	memset(v3dcr, 0, sizeof(v3dcr));

	sem = (struct v3dcr_sema *)&v3dcr[off];
	off += sizeof(*sem);

	cc = (struct v3dcr_clear_colours *)&v3dcr[off];
	off += sizeof(*cc);

	trmc = (struct v3dcr_tile_rendering_mode *)&v3dcr[off];
	off += sizeof(*trmc);

	tc = (struct v3dcr_tile_coords *)&v3dcr[off];
	off += sizeof(*tc);

	stg = (struct v3dcr_store_tb_gen *)&v3dcr[off];
	off += sizeof(*stg);

	sem->id = 8;

	cc->id = 114;
	// even and odd??
	cc->colour[0] = 0xffffff00;	// ARGB32
	cc->colour[1] = 0xffffff00;	// ARGB32

	trmc->id = 113;
	trmc->tb_base = pa_to_ba(fb_get_pa());
	trmc->width = WIDTH;
	trmc->height = HEIGHT;
	trmc->flags = 4;

	// Clear Colours needs an empty write.
	tc->id = 115;
	stg->id = 28;

	// Tile (x, y) writes at fb[(y * NUM_TILES_X + x) * 64]
	// The viewport transform assumes origin at bottom left, but the
	// fb assumes it top left. Flip the y's.
	for (y = 0; y < NUM_TILES_Y; ++y) {
		for (x = 0; x < NUM_TILES_X; ++x) {
			tc = (struct v3dcr_tile_coords *)&v3dcr[off];
			off += sizeof(*tc);

			br = (struct v3dcr_branch *)&v3dcr[off];
			off += sizeof(*br);

			str = (struct v3dcr_store_mstcb *)&v3dcr[off];
			off += sizeof(*str);

			tc->id = 115;
			tc->col = x;
			tc->row = y;

			tva = (va_t)ta;
			tva += (y * NUM_TILES_X + x) * 32;

			br->id = 17;
			br->addr = va_to_ba(tva);

			str->id = 24;
		}
	}

	// Last tile signals the EOF.
	str->id = 25;

	dc_cvac(v3dcr, off);
	dsb();

	v3d_run_renderer(va_to_ba((va_t)v3dcr), off);

	err = ERR_SUCCESS;
	return err;
}

// Coordinate Shader.
#if 0
li	vpr_setup, -, 0x701a00;
;;;
ori	a0, vpr, 0;
ori	a1, vpr, 0;
ori	a2, vpr, 0;
ori	a3, vpr, 0;
ori	a4, vpr, 0;
ori	a5, vpr, 0;
ori	a6, vpr, 0;

li	vpw_setup, -, 0x1a00;
or	vpw, a0, a0;
or	vpw, a1, a1;
or	vpw, a2, a2;
or	vpw, a3, a3;
or	vpw, a4, a4;
or	vpw, a5, a5;
or	vpw, a6, a6;

ori	host_int, 1, 1;
pe;;;
#endif

// Vertex Shader.
#if 0
li	vpr_setup, -, 0x301a00;
;;;
ori	a0, vpr, 0;
ori	a1, vpr, 0;
ori	a2, vpr, 0;

li	vpw_setup, -, 0x1a00;
or	vpw, a0, a0;
or	vpw, a1, a1;
or	vpw, a2, a2;

ori	host_int, 1, 1;
pe;;;
#endif


#if 0
# Wait for Scoreboard
wsb;
li	tlb_clr_all, -, 0xffff0000;
usb;
ori	host_int, 1, 1;
pe;;;
#endif


// The framebuffer format is BGRA8888, or 0xaarrggbb, or ARGB32.
#if 0
# RGB
faddi	r0, vary_rd, 0;
fadd	r0, r0, r5;

faddi	r1, vary_rd, 0;
fadd	r1, r1, r5;

faddi	r2, vary_rd, 0;
fadd	r2, r2, r5;

# x 255.0
li	r3, -, 255;
itof	r3, r3, r3;
fmul	r0, r0, r3;
fmul	r1, r1, r3;
fmul	r2, r2, r3;

# To integer colour
ftoi	r0, r0, r0;
ftoi	r1, r1, r1;
ftoi	r2, r2, r2;

# 0xaarrggbb
shli	r0, r0, 8;	# Red shifted by 16
shli	r0, r0, 8;
shli	r1, r1, 8;	# Green shifted by 8

or	r0, r0, r1;
or	r0, r0, r2;

li	r1, -, 0xff000000;
or	r0, r0, r1;

or	tlb_clr_all, r0, r0	usb;

ori	host_int, 1, 1;
pe;;;
#endif
