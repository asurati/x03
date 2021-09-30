// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdlib.h>
#include <lib/string.h>

#include <sys/cpu.h>
#include <sys/err.h>

#include <dev/v3d.h>
#include <dev/con.h>

// Reuse the d52 CS and VS.
#include "d52.cs.h"
#include "d52.vs.h"

#include "d53.fs.h"

#define FB_WIDTH			640
#define FB_HEIGHT			480

#define TILE_WIDTH			64
#define TILE_HEIGHT			64

// Hope the GPU doesn't write beyond the FB, since the last row of the tiles
// is less than 64 pixels in height. The ClipWindow and ViewPort configurations
// should help in avoiding the buffer overflow.

#define NUM_TILES_X			(FB_WIDTH / TILE_WIDTH)
#define NUM_TILES_Y			(FB_HEIGHT / TILE_HEIGHT + 1)

// For some ease, provide the eye/view coordinates as input to the cs and vs,
// instead of the object coordinates.
struct vertex {
	float				xe;
	float				ye;
	float				ze;
	float				we;
	float				r;
	float				g;
	float				b;
} __attribute__((packed));

// The smaller, white triangle is behind the first, larger triangle. Even so,
// the white triangle is rendered. With Z-testing disabled, the order of the
// triangle dictates the drawing. Here, the larger triangle is drawn first,
// and then the smaller, so the smaller is rendered on the screen even
// though it is behind the larger triangle.
//
// With the Z-testing enabled, the rendering is correct, regardless of the
// order in which the triangles are presented here.

// The values lifted up from d50.
static const struct vertex verts[] = {
	{0,	50,	-4, 1,	 1, 0, 0},
	{-50,	-50,	-4, 1,	 0, 1, 0},
	{50,	-50,	-4, 1,	 0, 0, 1},

	{0,	30,	-3.5,	1, 1, 1, 1},
	{-30,	-30,	-3.5,	1, 1, 1, 1},
	{30,	-30,	-3.5,	1, 1, 1, 1},
};

// The perpsective projection matrix is passed as a uniform. Row-major order.
static const float proj_mat[4][4] = {
	{1./20,	0,	0,	0},
	{0,	1./15,	0,	0},
	{0,	0,	-3./2,	-5./2},
	{0,	0,	-1,	0},
};

int d53_run()
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

	memcpy(unif, proj_mat, sizeof(proj_mat));

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
	tbmc->flags |= bits_on(V3DCR_TBMC_FLAGS_INIT_TSDA);	// Necessary.

	tbs->id = 6;

	sem->id = 7;

	// ClipWindow excludes the last row of the tiles.
	cw->id = 102;
	cw->width = FB_WIDTH;
	cw->height = FB_HEIGHT;

	cb->id = 96;
	cb->flags[0] |= bits_on(V3DCR_CFG_FWD_FACE_EN);
	cb->flags[0] |= bits_on(V3DCR_CFG_CLOCKWISE);
	cb->flags[1] |= bits_on(V3DCR_CFG_Z_UPDATE_EN);
	cb->flags[1] |= bits_set(V3DCR_CFG_Z_TEST_FN, 1);	// LessThan.

	// The viewport offset coordinates are in signed 12.4 fixed point
	// format.
	vo->id = 103;
	vo->x = (FB_WIDTH / 2) << 4;	// Centre coordinates.
	vo->y = (FB_HEIGHT / 2) << 4;

	cxy->id = 105;
	cxy->half_width = (FB_WIDTH / 2) * 16.0;
	cxy->half_height = (FB_HEIGHT / 2) * 16.0;

	ss->id = 64;
	ss->ssr_base = va_to_ba((va_t)&ssr);
	ss->ssr_base |= 2;

	va->id = 33;
	va->mode = V3DCR_VERT_ARR_MODE_TRI;
	va->num_verts = 6;

	f->id = 4;

	memset(&ssr, 0, sizeof(ssr));
	ssr.flags |= bits_on(V3DCR_SSR_FLAGS_FS_STHRD);
	ssr.flags |= bits_on(V3DCR_SSR_FLAGS_CLIP_EN);

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
	cc->mask = 0xffffff;		// Z value == 1.0

	trmc->id = 113;
	trmc->tb_base = pa_to_ba(fb_get_pa());
	trmc->width = FB_WIDTH;
	trmc->height = FB_HEIGHT;
	trmc->flags |= bits_set(V3DCR_TRMC_FLAGS_FBC_FMT, 1);	//RGBA8888

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
