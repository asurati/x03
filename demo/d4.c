// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdlib.h>
#include <lib/string.h>

#include <sys/cpu.h>
#include <sys/err.h>

#include <dev/v3d.h>
#include <dev/con.h>

struct vertex {
	uint16_t			xs;
	uint16_t			ys;
	float				zs;
	float				wcr;
	float				r;
	float				g;
	float				b;
} __attribute__((packed));

static const struct vertex verts[] __attribute__((aligned(32))) = {
	{0,		0,		0.5,	1.0, 1.0, 0.0, 0.0},
	{60 << 4,	0,		0.5,	1.0, 0.0, 1.0, 0.0},
	{0,		30 << 4,	0.5,	1.0, 0.0, 0.0, 1.0},
};

int d4_run()
{
	int err, off, i, x, y;
	uint32_t val;
	volatile uint32_t *fb;
	volatile uint32_t *fb_get();

	struct v3dcr_tile_binning_mode		*tbmc;
	struct v3dcr_tile_binning_start		*tbs;
	struct v3dcr_clip_window		*cw;
	struct v3dcr_config_bits		*cb;
	struct v3dcr_viewport_offset		*vo;
	struct v3dcr_clipper_xy_scale		*cxy;
	struct v3dcr_nv_shader_state		*ss;
	struct v3dcr_vert_array			*va;
	struct v3dcr_flush			*f;
	struct v3dcr_nv_shader_state_rec	*ssr;

	struct v3dcr_tile_rendering_mode	*trmc;
	struct v3dcr_tile_coords		*tc;
	struct v3dcr_branch			*br;
	struct v3dcr_store_mstcb_eof		*eof;

	static char v3dcr[512] __attribute__((aligned(32)));

	// Alignment enforced by TBMC/112.
	static uint32_t tsda[64 >> 2] __attribute__((aligned(32)));

	// Alignment seems to be enfored by the max. tile allocation block
	// size of 256 bytes. This buffer is the binning memory pool
	// BPCA/BPCS.
	static uint32_t ta[PAGE_SIZE >> 2] __attribute__((aligned(256)));

	static uint32_t tb[64 * 64] __attribute__((aligned(32)));

	// Alignment enforced by NVSS/65.
	static char ssr_buf[32] __attribute__((aligned(32)));

	static const uint32_t code[] __attribute__((aligned(8))) = {
		0x158c0dc0, 0xd0020800,
		0x1000140, 0x10020800,
		0x158c0dc0, 0xd0020840,
		0x1000340, 0x10020840,
		0x158c0dc0, 0xd0020880,
		0x1000540, 0x10020880,

		0x437f0000, 0xe00208c0,

		0x20000003, 0x10004020,
		0x2000000b, 0x10004021,
		0x20000013, 0x10004022,

		0x70001c0, 0xd0020800,
		0x70003c0, 0xd0020840,
		0x70005c0, 0xd0020880,

		0x110085c0, 0xd0020880,
		0x110085c0, 0xd0020880,
		0x110083c0, 0xd0020840,

		0x15000040, 0x10020800,
		0x15000080, 0x10020800,

		0xff000000, 0xe0020840,
		0x15000040, 0x10020800,

		0x150001c0, 0xd0020b80,
		0x0, 0x50000000,

		0x15001fc0, 0xd0020980,
		0x0, 0x30000000,
		0x0, 0x10000000,
		0x0, 0x10000000,
#if 0
		0x159c1fc0, 0xd00209a7,
		0x009e7000, 0x300009e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
#endif
	};

	memset(ssr_buf, 0, sizeof(ssr_buf));

	off = 0;
	memset(v3dcr, 0, sizeof(v3dcr));

	tbmc = (struct v3dcr_tile_binning_mode *)&v3dcr[off];
	off += sizeof(*tbmc);

	tbs = (struct v3dcr_tile_binning_start *)&v3dcr[off];
	off += sizeof(*tbs);

	cw = (struct v3dcr_clip_window *)&v3dcr[off];
	off += sizeof(*cw);

	cb = (struct v3dcr_config_bits *)&v3dcr[off];
	off += sizeof(*cb);

	vo = (struct v3dcr_viewport_offset *)&v3dcr[off];
	off += sizeof(*vo);

	cxy = (struct v3dcr_clipper_xy_scale *)&v3dcr[off];
	off += sizeof(*cxy);

	ss = (struct v3dcr_nv_shader_state *)&v3dcr[off];
	off += sizeof(*ss);

	va = (struct v3dcr_vert_array *)&v3dcr[off];
	off += sizeof(*va);

	f = (struct v3dcr_flush *)&v3dcr[off];
	off += sizeof(*f);

	tbmc->id = 112;
	tbmc->ta_base = va_to_ba((va_t)ta);
	tbmc->ta_size = sizeof(ta);
	tbmc->tsda_base = va_to_ba((va_t)tsda);
	tbmc->width = tbmc->height = 1;
	tbmc->flags = 4;	// auto-init tsda. Necessary.

	tbs->id = 6;

	cw->id = 102;
	cw->width = cw->height = 64;

	cb->id = 96;
	cb->flags[0] = 3;

	vo->id = 103;

	cxy->id = 105;
	cxy->half_width = cxy->half_height = 32 * 16.0;

	ss->id = 65;
	ss->ssr_base = va_to_ba((va_t)ssr_buf);

	va->id = 33;
	va->mode = 4;
	va->num_verts = 3;

	f->id = 4;

	ssr = (struct v3dcr_nv_shader_state_rec *)ssr_buf;
	ssr->flags = 1;
	ssr->stride = sizeof(struct vertex);
	ssr->num_vary = 3;
	ssr->code_addr = va_to_ba((va_t)code);
	ssr->verts_addr = va_to_ba((va_t)verts);

	dc_cvac(v3dcr, off);
	dc_cvac(ssr, sizeof(*ssr));
	dsb();

	v3d_run_binner(va_to_ba((va_t)v3dcr), off);

	dc_ivac(tsda, sizeof(tsda));
	dc_ivac(ta, sizeof(ta));
	dsb();

#if 0
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

	trmc = (struct v3dcr_tile_rendering_mode *)&v3dcr[off];
	off += sizeof(*trmc);

	tc = (struct v3dcr_tile_coords *)&v3dcr[off];
	off += sizeof(*tc);

	br = (struct v3dcr_branch *)&v3dcr[off];
	off += sizeof(*br);

	eof = (struct v3dcr_store_mstcb_eof *)&v3dcr[off];
	off += sizeof(*eof);

	trmc->id = 113;
	trmc->tb_base = va_to_ba((va_t)tb);
	trmc->width = trmc->height = 64;
	trmc->flags = 4;

	tc->id = 115;

	br->id = 17;
	br->addr = va_to_ba((va_t)ta);

	eof->id = 25;

	dc_cvac(v3dcr, off);
	dsb();

	v3d_run_renderer(va_to_ba((va_t)v3dcr), off);

	dc_ivac(tb, sizeof(tb));
	dsb();

	fb = fb_get();

	for (y = 0, i = 0; y < 64; ++y) {
		for (x = 0; x < 64; ++x) {
			val = *(volatile uint32_t *)&tb[i++];
			fb[y * 1280 + x] = val;
		}
		dc_cvac((void *)&fb[y * 1280], 64*4);
	}
	err = ERR_SUCCESS;
	return err;
}
