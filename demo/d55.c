// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdlib.h>
#include <lib/string.h>

#include <sys/cpu.h>
#include <sys/err.h>

#include <dev/v3d.h>
#include <dev/con.h>
#include <dev/hvs.h>
#include <dev/txp.h>

// Reuse from d52.
#include "d52.cs.h"
#include "d52.vs.h"
#include "d52.fs.h"

#define FB_WIDTH			640
#define FB_HEIGHT			480

#define SCL_FB_WIDTH			1440
#define SCL_FB_HEIGHT			1080

// MSAA 4x changes the tile width from 64x64 to 32x32
#define TILE_WIDTH			32
#define TILE_HEIGHT			32

#define NUM_TILES_X			(FB_WIDTH / TILE_WIDTH)
#define NUM_TILES_Y			(FB_HEIGHT / TILE_HEIGHT)

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

// The values are lifted up from d50.c.
static const struct vertex verts[] = {
	{0,	50,	-4, 1,	 1, 0, 0},
	{-50,	-50,	-4, 1,	 0, 1, 0},
	{50,	-50,	-4, 1,	 0, 0, 1},
};

// The perpsective projection matrix is passed as a uniform. Row-major order.
static const float proj_mat[4][4] = {
	{1./20,	0,	0,	0},
	{0,	1./15,	0,	0},
	{0,	0,	-3./2,	-5./2},
	{0,	0,	-1,	0},
};

int d55_run()
{
	int off, x, y;
	va_t tva;

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

	// Alignment enforced by GLSS/64.
	static struct v3dcr_gl_shader_state_rec ssr
		__attribute__((aligned(CACHE_LINE_SIZE)));

	static uint32_t unif[16];

	// Local Frame Buffer into which the V3D pipeline writes the image.
	static uint32_t l_fb[FB_WIDTH * FB_HEIGHT];
	void	d55_run_txp(const uint32_t *);

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
	tbmc->flags |= bits_on(V3DCR_TBMC_FLAGS_MSAA);

	tbs->id = 6;

	sem->id = 7;

	cw->id = 102;
	cw->width = FB_WIDTH;
	cw->height = FB_HEIGHT;

	cb->id = 96;
	cb->flags[0] |= bits_on(V3DCR_CFG_FWD_FACE_EN);
	cb->flags[0] |= bits_on(V3DCR_CFG_CLOCKWISE);
	cb->flags[0] |= bits_set(V3DCR_CFG_OVERSAMPLE, 1);

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
	va->num_verts = 3;

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

	trmc->id = 113;
	trmc->tb_base = va_to_ba((va_t)l_fb);
	trmc->width = FB_WIDTH;
	trmc->height = FB_HEIGHT;
	trmc->flags |= bits_set(V3DCR_TRMC_FLAGS_FBC_FMT, 1);	//RGBA8888
	trmc->flags |= bits_on(V3DCR_TRMC_FLAGS_MSAA);

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
	d55_run_txp(l_fb);
	return ERR_SUCCESS;
}
// The framebuffer format is BGRA8888, or 0xaarrggbb, or ARGB32.

// From the firmware. Mitchell-Netravali.
const uint32_t ppf_krnl[] = {
	0x7ebfc00, 0x7e3edf8, 0x4805fd, 0x1dca432,
	0x355769b, 0x1c6e3, 0x355769b, 0x1dca432,
	0x4805fd, 0x7e3edf8, 0x7ebfc00
};

#define HVS_DL_MEM_INDEX		0x800

// These indices are randomly selected, and may trample upon the firmware's
// display lists and parameters.
#define HVS_DL_INDEX			0x400
#define HVS_DL_PPFK_INDEX		0x410
void d55_run_txp(const uint32_t *fb)
{
	int i;
	uint32_t val;
	extern volatile uint32_t *g_hvs_regs;
	extern volatile uint32_t *g_txp_regs;
	volatile uint32_t *dl, *ppfk;
	static uint32_t out_fb[1440 * 1080];
	void	rle_dump(volatile uint32_t *, int, int);

	dl = &g_hvs_regs[HVS_DL_MEM_INDEX + HVS_DL_INDEX];
	ppfk = &g_hvs_regs[HVS_DL_MEM_INDEX + HVS_DL_PPFK_INDEX];

	for (i = 0; i < 11; ++i)
		ppfk[i] = ppf_krnl[i];

	// The input FB (fb) has the format BGRA8888, or  ARGB32.
	// The output FB (out_fb) is expected to be in the same format.
	val = 0;
	val |= bits_set(HVS_DLW_CTL0_PIXEL_FMT, 7);	// RGBA8888
	val |= bits_set(HVS_DLW_CTL0_PIXEL_ORDER, 3);
	val |= bits_set(HVS_DLW_CTL0_RGBA_EXPAND, 3);	// Round??
	val |= bits_on(HVS_DLW_CTL0_VALID);
	val |= bits_set(HVS_DLW_CTL0_NUM_WORDS, 14);	// 14 + End.
	dl[0] = val;

	dl[1] = bits_on(HVS_DLW_POS0_FIXED_ALPHA);	// 0xff

	val = 0;
	val |= bits_set(HVS_DLW_POS1_SCL_WIDTH, SCL_FB_WIDTH);
	val |= bits_set(HVS_DLW_POS1_SCL_HEIGHT, SCL_FB_HEIGHT);
	dl[2] = val;

	val = 0;
	val |= bits_set(HVS_DLW_POS2_SRC_WIDTH, FB_WIDTH);
	val |= bits_set(HVS_DLW_POS2_SRC_HEIGHT, FB_HEIGHT);
	val |= bits_set(HVS_DLW_POS2_ALPHA_MODE, 1);	// Fixed Alpha.
	dl[3] = val;
	dl[4] = 0;

	dl[5] = va_to_ba((va_t)fb);
	dl[6] = 0;
	dl[7] = FB_WIDTH * 4;	// Source Pitch, for Linear Tiling.
	dl[8] = 0x2000;		// LBM base address. Random atm.

	val = 0;
	val |= bits_set(HVS_DLW_PPF_IPHASE, 0x60);	// From firmware.
	val |= bits_set(HVS_DLW_PPF_SCALE,
			(1ul << 16) * FB_WIDTH / SCL_FB_WIDTH);
	val |= bits_on(HVS_DLW_PPF_AGC);
	dl[9] = val;

	val = 0;
	val |= bits_set(HVS_DLW_PPF_IPHASE, 0x60);	// From firmware.
	val |= bits_set(HVS_DLW_PPF_SCALE,
			(1ul << 16) * FB_HEIGHT / SCL_FB_HEIGHT);
	val |= bits_on(HVS_DLW_PPF_AGC);
	dl[10] = val;
	dl[11] = 0;

	dl[12] = HVS_DL_PPFK_INDEX;// H Scaling kernel for Plane0 (RGB, or Y)
	dl[13] = HVS_DL_PPFK_INDEX;// V Scaling kernel for Plane0 (RGB, or Y)
	dl[14] = bits_on(HVS_DLW_CTL0_END);

	g_hvs_regs[HVS_DL2] = HVS_DL_INDEX;

	val = 0;
	val |= bits_on(HVS_DL_CTRL_EN);
	val |= bits_on(HVS_DL_CTRL_ONESHOT);
	val |= bits_set(HVS_DL_CTRL_HEIGHT, SCL_FB_HEIGHT);
	val |= bits_set(HVS_DL_CTRL_WIDTH, SCL_FB_WIDTH);
	val |= bits_set(HVS_DL_CTRL_WIDTH, SCL_FB_WIDTH);
	g_hvs_regs[HVS_DL2_CTRL] = val;

	g_txp_regs[TXP_DST_POINTER] = va_to_ba((va_t)out_fb);
	g_txp_regs[TXP_DST_PITCH] = SCL_FB_WIDTH * 4;

	val = 0;
	val |= bits_set(TXP_DIM_WIDTH, SCL_FB_WIDTH);
	val |= bits_set(TXP_DIM_HEIGHT, SCL_FB_HEIGHT);
	g_txp_regs[TXP_DIM] = val;

	val = 0;
	val |= bits_on(TXP_DST_CTRL_GO);
	val |= bits_set(TXP_DST_CTRL_FMT, 13);
	val |= bits_on(TXP_DST_CTRL_BYTE_EN);
	val |= bits_on(TXP_DST_CTRL_ALPHA_EN);
	val |= bits_set(TXP_DST_CTRL_VERSION, 1);
	val |= bits_set(TXP_DST_CTRL_PILOT, 0x54);
	g_txp_regs[TXP_DST_CTRL] = val;

	while (1) {
		if (!bits_get(g_txp_regs[TXP_DST_CTRL], TXP_DST_CTRL_BUSY))
			break;
	}
	// Remove this return to print the out_fb.
	return;
	dc_ivac(out_fb, sizeof(out_fb));
	rle_dump(out_fb, 1440, 1080);
}
