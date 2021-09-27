// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdlib.h>
#include <lib/string.h>

#include <sys/cpu.h>
#include <sys/err.h>

#include <dev/v3d.h>
#include <dev/con.h>

// Shaders
#include "d52.cs.h"
#include "d52.vs.h"
#include "d52.fs.h"

// Assuming a 640 x 480 framebuffer size.

#define NUM_TILES_X			10
#define NUM_TILES_Y			7

#define WIDTH				(NUM_TILES_X * 64)
#define HEIGHT				(NUM_TILES_Y * 64)

struct vertex {
	float				xo;
	float				yo;
	float				zo;
	float				wo;
	float				r;
	float				g;
	float				b;
} __attribute__((packed));

// For some ease, provide the eye/view coordinates as input to the cs and vs.
// The cs will read only the first 4 attributes. The GL shader state record
// must list the cs attribute as follows:
// Base = verts
// Number of Bytes = 16;
// Memory Stride = 28;	// Need to skip over the R,G,B fields.

static const struct vertex verts[] = {
	{0,	3,	-7, 1,	 1, 0, 0},
	{-3,	-1,	-7, 1,	 0, 1, 0},
	{3,	-1,	-7, 1,	 0, 0, 1},
};

// The cs receives these values as:
// 0	-3	3
// 3	-1	-1
// -7	-7	-7
// 1	1	1

// The perpsective projection matrix is passed as a uniform. Row-major order.
static const float proj_mat[4][4] = {
	{1.0/4,	0,	0,		0},
	{0,	1.0/3,	0,		0},
	{0,	0,	-11.0/9,	-20.0/9},
	{0,	0,	-1,		0},
};

// The v3d gpu takes ViewPort Centre (cx, cy) value, and the xs and ys are
// relative to that centre.
// For 640x480, the centre is at WC (320, 240).

// The NDC to ViewPort/WC transformation is as follows:
// The ViewPort origin is at bottom left.

// In the X direction:
// NDC -1 is WC -319.5 (relative to the ViewPort Centre X).
// NDC 1 is WC 319.5 (relative to the ViewPort Centre X).
// (-1, -319.5) and (1, 319.5) are two points on a line. The slope is
// m = y2-y1 / x2-x1
// 319.5+319.5 / 2 = 319.5
// xs = 319.5 * xndc + d.
// 319.5 = 319.5 * 1 + d => d = 0
// xs = 319.5 * xndc;

// In the Y direction:
// NDC -1 is WC -239.5 (relative to the ViewPort Centre X).
// NDC 1 is WC 239.5 (relative to the ViewPort Centre X).
// (-1, -239.5) and (1, 239.5) are two points on a line. The slope is
// m = y2-y1 / x2-x1
// 239.5+239.5 / 2 = 239.5
// ys = 239.5 * yndc + d.
// 239.5 = 239.5 * 1 + d. => d = 0
// ys = 239.5 * yndc.

// In the Z direction (glDepthRange is (0,1)):
// NDC -1 is WC 0.
// NDC 1 is WC 1.
// (-1, 0) and (1, 1) are two points on a line. The slope is
// m = y2-y1 / x2-x1
// 1-0 / 2 = 1/2
// zs = 0.5 * zndc + d.
// 1 = 0.5 + d => d = 0.5.
// zs = 0.5 * zndc + 0.5.

// Calculations:
// top		(0, 3, -2, 1);
// left		(-3, -1, -2, 1);
// right	(3, -1, -2, 1);
//
// Camera at	(0,0,5);
// The eye/view coordinates are thus:
// top		(0, 3, -7, 1);
// left		(-3, -1, -7, 1);
// right	(3, -1, -7, 1);
//
// Frustum:
// Near plane at -1, so n = 1.
// Far plane at -10, so f = 10.

// 640x480 aspect ratio = 4:3

// The near plane's dimensions are:
// Width = 8, Height = 6
// Thus, r = 4, t = 3.

// The perspective projection matrix is thus:

//	1/4	0	0	0
//	0	1/3	0	0
//	0	0	-11/9	-20/9
//	0	0	-1	0

// Clip coordinates:
// top		(0,	1,	57/9,	7)	wc = 7
// left		(-3/4,	-1/3,	57/9,	7)	wc = 7
// right	(3/4,	-1/3,	57/9,	7)	wc = 7

// NDC coordinates:
// top		(0,	1/7,	57/63)
// left		(-3/28,	-1/21,	57/63)
// right	(3/28,	-1/21,	57/63)

// ViewPort size is 640x480
// ViewPort Centre (320, 240)
// n = 0.0, f = 1.0 (glDepthRange)

// Flip the sign of the y coordinates, so that the image is right side up on
// render.

// top		(0,	-1/7,	57/63)
// left		(-3/28,	1/21,	57/63)
// right	(3/28,	1/21,	57/63)

// xs = 319.5 * xndc;
// ys = 239.5 * yndc.
// zs = 0.5 * zndc + 0.5.

// Screen/Window coordinates relative to the ViewPort Centre:
// top		(0,		-239.5/7,	60/63);
// left		(-958.5/28,	239.5/21,	60/63);
// right	(958.5/28,	239.5/21,	60/63);

// top		(0,		-34.2,		60/63);
// left		(-34.2,		11.4,		60/63);
// right	(34.2,		11.4,		60/63);
//
// Absolute Screen/Window coordinates relative to the ViewPort Centre:
// top		(320,		1440.5/7,	60/63);
// left		(8001.5/28,	5279.5/21,	60/63);
// right	(9918.5/28,	5279.5/21,	60/63);

// top		(320,		205.8,		60/63);
// left		(285.8		251.4,		60/63);
// right	(354.2,		251.4,		60/63);

int d52_run()
{
	int off, x, y;
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

	static char v3dcr[PAGE_SIZE];

	// 16-byte Alignment enforced by TBMC/112. Provide a CACHE_LINE_SIZE
	// alignment.
	static uint32_t tsda[(NUM_TILES_Y * NUM_TILES_X * 48) >> 2]
		__attribute__((aligned(CACHE_LINE_SIZE)));

	// Alignment seems to be enfored by the max. tile allocation block
	// size of 256 bytes. This buffer is the binning memory pool
	// BPCA/BPCS.
	static uint32_t ta[PAGE_SIZE >> 2] __attribute__((aligned(256)));

	// Alignment enforced by NVSS/64.
	static struct v3dcr_gl_shader_state_rec ssr
		__attribute__((aligned(CACHE_LINE_SIZE)));

	static uint32_t unif[17];
	static uint32_t cs_out[128][16] __attribute__((aligned(CACHE_LINE_SIZE)));

	memcpy(unif, proj_mat, sizeof(proj_mat));
	unif[16] = va_to_ba((va_t)cs_out);

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
	vo->x = 320 << 4;	// Centre coordinates.
	vo->y = 240 << 4;

	cxy->id = 105;
	cxy->half_width = (WIDTH / 2) * 16.0;
	cxy->half_height = (HEIGHT / 2) * 16.0;

	ss->id = 64;
	ss->ssr_base = va_to_ba((va_t)&ssr);
	ss->ssr_base |= 2;

	va->id = 33;
	va->mode = 4;
	va->num_verts = 3;

	f->id = 4;

	memset(&ssr, 0, sizeof(ssr));
	ssr.flags = 5;
	ssr.fs_num_vary = 3;
	ssr.fs_code_addr = va_to_ba((va_t)fs_code);

	ssr.cs_attr_sel = 1;
	ssr.cs_attr_size = 4 * sizeof(float);
	ssr.cs_code_addr = va_to_ba((va_t)cs_code);
	ssr.cs_unif_addr = va_to_ba((va_t)unif);
	ssr.cs_num_unif = 16;
	ssr.attr[0].base = va_to_ba((va_t)verts);
	ssr.attr[0].num_bytes = 4 * sizeof(float) - 1;
	ssr.attr[0].stride = 7 * sizeof(float);
	// 4 -> skips 1 row in VPM.
	// 8 -> skips 2 rows in VPM.
	// ssr.attr[0].cs_vpm_off = 8;

	// VS is part of the rendering pipeline.
	ssr.vs_attr_sel = 2;
	ssr.vs_attr_size = 7 * sizeof(float);
	ssr.vs_code_addr = va_to_ba((va_t)vs_code);
	ssr.vs_unif_addr = va_to_ba((va_t)unif);
	ssr.vs_num_unif = 16;
	ssr.attr[1].base = va_to_ba((va_t)verts);
	ssr.attr[1].num_bytes = 7 * sizeof(float)- 1;
	ssr.attr[1].stride = 7 * sizeof(float);

	dc_cvac(v3dcr, off);
	dc_cvac(&ssr, sizeof(ssr));
	dc_cvac(unif, sizeof(unif));
	dsb();

	v3d_run_binner(va_to_ba((va_t)v3dcr), off);
#if 0
	dc_ivac(cs_out, sizeof(cs_out));
	dsb();
	for (i = 0; i < 128; ++i) {
		con_out("%x: %x %x %x %x, %x %x %x %x, %x %x %x %x, %x %x %x %x",
			i,
			cs_out[i][0], cs_out[i][1], cs_out[i][2], cs_out[i][3],
			cs_out[i][4], cs_out[i][5], cs_out[i][6], cs_out[i][7],
			cs_out[i][8], cs_out[i][9], cs_out[i][10], cs_out[i][11],
			cs_out[i][12], cs_out[i][13], cs_out[i][14], cs_out[i][15]);
	}
	return ERR_SUCCESS;
#endif

	off = 0;
	memset(v3dcr, 0, sizeof(v3dcr));

	cc = (struct v3dcr_clear_colours *)&v3dcr[off];
	off += sizeof(*cc);

	trmc = (struct v3dcr_tile_rendering_mode *)&v3dcr[off];
	off += sizeof(*trmc);

	tc = (struct v3dcr_tile_coords *)&v3dcr[off];
	off += sizeof(*tc);

	stg = (struct v3dcr_store_tb_gen *)&v3dcr[off];
	off += sizeof(*stg);

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

	for (y = 0; y < NUM_TILES_Y; ++y) {
		for (x = 0; x < NUM_TILES_X; ++x) {
			tc = (struct v3dcr_tile_coords *)&v3dcr[off];
			off += sizeof(*tc);

			if (x == 0 && y == 0) {
				sem = (struct v3dcr_sema *)&v3dcr[off];
				off += sizeof(*sem);
			}

			br = (struct v3dcr_branch *)&v3dcr[off];
			off += sizeof(*br);

			str = (struct v3dcr_store_mstcb *)&v3dcr[off];
			off += sizeof(*str);

			tc->id = 115;
			tc->col = x;
			tc->row = y;

			if (x == 0 && y == 0)
				sem->id = 8;

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
	return ERR_SUCCESS;
}
// The framebuffer format is BGRA8888, or 0xaarrggbb, or ARGB32.
