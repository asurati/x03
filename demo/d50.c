// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdlib.h>
#include <lib/string.h>

#include <sys/cpu.h>
#include <sys/err.h>

#include <dev/v3d.h>
#include <dev/con.h>

#define FB_WIDTH			640
#define FB_HEIGHT			480

#define TILE_WIDTH			64
#define TILE_HEIGHT			64

// Hope the GPU doesn't write beyond the FB, since the last row of the tiles
// is less than 64 pixels in height. The ClipWindow and ViewPort configurations
// should help in avoiding the buffer overflow.

#define NUM_TILES_X			(FB_WIDTH / TILE_WIDTH)
#define NUM_TILES_Y			(FB_HEIGHT / TILE_HEIGHT + 1)

struct vertex {
	int16_t				xs;
	int16_t				ys;
	float				zs;
	float				wcr;
	float				r;
	float				g;
	float				b;
} __attribute__((packed));

// Object Coordinates (xyzw):
// top		(0,	50,	-2,	1);
// left		(-50,	-50,	-2,	1);
// right	(50,	-50,	-2,	1);

// Camera:
// Position	(0,	0,	2);
// LookAt	(0,	0,	0);

// Eye/View Coordinates:
// top		(0,	50,	-4,	1);
// left		(-50,	-50,	-4,	1);
// right	(50,	-50,	-4,	1);

// http://www.songho.ca/opengl/gl_projectionmatrix.html
// Frustum (in Eye/View Coordinates):
// Near Plane at z = -1. n = 1.
// Far Plane at z = -5.	f = 5.

// 640x480, Aspect Ratio = 4:3
// Near Plane Dimensions: 40:30. r = 20, t = 15.

// The Perspective Projection Matrix:
//	1/20	0	0	0
//	0	1/15	0	0
//	0	0	-3/2	-5/2
//	0	0	-1	0

// Clip Coordinates:
// top		(0,	10/3,	7/2,	4);
// left		(-5/2,	-10/3,	7/2,	4);
// right	(5/2,	-10/3,	7/2,	4);

// NDC:
// top		(0,	10/12,	7/8);
// left		(-5/8,	-10/12,	7/8);
// right	(5/8,	-10/12,	7/8);

// ViewPort Centre Coordinates in Screen Space: (320, 240). The ViewPort
// Origin is at Bottom Left of the Screen.

// NDC to ViewPort Transformation X direction:
// NDC -1 is ViewPort -319.5 (Relative to the ViewPort Centre).
// NDC 1 is ViewPort 319.5 (Relative to the ViewPort Centre).
// (-1, -319.5) and (1, 319.5) are two points on a line.
// m = y2-y1 / x2-x1 = 319.5+319.5 / 2 = 319.5
// Xs = 319.5 * Xndc + d.
// 319.5 = 319.5 * 1 + d => d = 0.
// Xs = 319.5 * Xndc;

// NDC to ViewPort Transformation Y direction:
// Similar calculations as above:
// Ys = 239.5 * Yndc;

// NDC to ViewPort Transformation Z direction:
// glDepthRange(0, 1);
// NDC -1 is ViewPort 0.
// NDC 1 is ViewPort 1.
// Simlar calculations as above:
// Zs = 0.5 * (Zndc + 1);

// NDC: Flip the sign of the Y coordinates, so that the resulting image is
// right side up on render. This is because the GPU and the Framebuffer do not
// agree on the coordinate axes - for the GPU, the origin is at the
// bottom-left, but for the Framebuffer, it is at top-left.

// NDC Y-flipped:
// top		(0,	-10/12,	7/8);
// left		(-5/8,	10/12,	7/8);
// right	(5/8,	10/12,	7/8);

// Screen coordinates in float (Relative to the ViewPort Centre):
// top		(0,		-199.6,	0.94);
// left		(-199.7,	199.6,	0.94);
// right	(199.7,		199.6,	0.94);

// Screen coordinates (Xs, Ys) in 12.4 fixed point (float x 16.0)
// (Relative to the ViewPort Centre):
// top		(0,	-3193,	0.94);
// left		(-3195,	3193,	0.94);
// right	(3195,	3193,	0.94);

static const struct vertex verts[] = {
	{0,	-3193,	0.94,	0.25,	1, 0, 0},
	{-3195,	3193,	0.94,	0.25,	0, 1, 0},
	{3195,	3193,	0.94,	0.25,	0, 0, 1},
};

int d50_run()
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

	static char v3dcr[PAGE_SIZE] __attribute__((aligned(32)));

	// 16-byte Alignment enforced by TBMC/112. Provide a CACHE_LINE_SIZE
	// alignment.
	static uint32_t tsda[(NUM_TILES_Y * NUM_TILES_X * 48) >> 2]
		__attribute__((aligned(CACHE_LINE_SIZE)));

	// Alignment seems to be enfored by the max. tile allocation block
	// size of 256 bytes. This buffer is the binning memory pool
	// BPCA/BPCS.
	static uint32_t ta[PAGE_SIZE >> 2] __attribute__((aligned(256)));

	// Alignment enforced by NVSS/65.
	static struct v3dcr_nv_shader_state_rec ssr
	       	__attribute__((aligned(CACHE_LINE_SIZE)));

	static const uint32_t code[] __attribute__((aligned(8))) = {
		0x203e303e, 0x100049e0, // fmul	r0, vary_rd, a15;
		0x019e7140, 0x10020827, // fadd	r0, r0, r5;

		0x203e303e, 0x100049e1, // fmul	r1, vary_rd, a15;
		0x019e7340, 0x10020867, // fadd	r1, r1, r5;

		0x203e303e, 0x100049e2, // fmul	r2, vary_rd, a15;
		0x019e7540, 0x100208a7, // fadd	r2, r2, r5;

		0xff000000, 0xe00208e7, // li	r3, -, 0xff000000;

		0x209e0007, 0xd16049e3, // fmuli	r3, r0, 1f	pm8c;
		0x209e000f, 0xd15049e3, // fmuli	r3, r1, 1f	pm8b;
		0x209e0017, 0xd14049e3, // fmuli	r3, r2, 1f	pm8a;

		0x159e76c0, 0x50020ba7, // or	tlb_clr_all, r3, r3	usb;

		0x159c1fc0, 0xd00209a7, // ori	host_int, 1, 1;
		0x009e7000, 0x300009e7, // pe;
		0x009e7000, 0x100009e7, // ;
		0x009e7000, 0x100009e7, // ;
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
	tbmc->flags |= bits_on(V3DCR_TBMC_FLAGS_INIT_TSDA);	// Necessary.

	tbs->id = 6;

	sem->id = 7;

	cw->id = 102;
	cw->width = FB_WIDTH;
	cw->height = FB_HEIGHT;

	// By default, the GPU considers any triangle seen with an
	// anti-clockwise winding order (CCW) as front facing, but because the
	// GPU also considers the first byte of the framebuffer as belonging to
	// the bottom of the screen instead of the top, it has become necessary
	// to flip the sign of the Y-coordinate in order to render the scene
	// right side up. That is done within the CS/VS by flipping the sign of
	// the Y-coordinate within NDC. But then the winding order is opposite
	// of what was intended. To fix that, tell the GPU that CW is the
	// front facing winding order, and then enable the front facing
	// primitives.
	cb->id = 96;
	cb->flags[0] |= bits_on(V3DCR_CFG_FWD_FACE_EN);
	cb->flags[0] |= bits_on(V3DCR_CFG_CLOCKWISE);

	// The viewport offset coordinates are in signed 12.4 fixed point
	// format.
	vo->id = 103;
	vo->x = (FB_WIDTH / 2) << 4;	// Centre coordinates.
	vo->y = (FB_HEIGHT / 2) << 4;

	cxy->id = 105;
	cxy->half_width = (FB_WIDTH / 2) * 16.0;
	cxy->half_height = (FB_HEIGHT / 2) * 16.0;

	ss->id = 65;
	ss->ssr_base = va_to_ba((va_t)&ssr);

	va->id = 33;
	va->mode = V3DCR_VERT_ARR_MODE_TRI;
	va->num_verts = 3;

	f->id = 4;

	memset(&ssr, 0, sizeof(ssr));
	ssr.flags |= bits_on(V3DCR_SSR_FLAGS_FS_STHRD);
	ssr.stride = sizeof(struct vertex);
	ssr.num_vary = 3;
	ssr.code_addr = va_to_ba((va_t)code);
	ssr.verts_addr = va_to_ba((va_t)verts);

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

#if 0
# RGB
fmul	r0, vary_rd, a15;	# a15 has W.
fadd	r0, r0, r5;

fmul	r1, vary_rd, a15;
fadd	r1, r1, r5;

fmul	r2, vary_rd, a15;
fadd	r2, r2, r5;

li	r3, -, 0xff000000;	# alpha (= 8d)

# Utilize MUL-pack facility to convert colour components from float to
# byte with saturation, and place them at appropriate locations depending on
# the framebuffer format. The format is 0x8d8c8b8a, corresponding to
# 0xaarrggbb.
fmuli	r3, r0, 1f	pm8c;
fmuli	r3, r1, 1f	pm8b;
fmuli	r3, r2, 1f	pm8a;

or	tlb_clr_all, r3, r3	usb;

ori	host_int, 1, 1;
pe;;;
#endif
