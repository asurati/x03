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

#define NUM_TILES_X			(FB_WIDTH / TILE_WIDTH)
#define NUM_TILES_Y			(FB_HEIGHT / TILE_HEIGHT + 1)

// The Shaded Coordinates Format is the format in which a coordinate shader
// outputs. Since we want a pass through coordinate shader, we set the input of
// the shader to be the same format as well. Similar siuation with the vertex
// shader.

struct vertex {
	float				xc;	// CS vertex starts here.
	float				yc;
	float				zc;
	float				wc;
	int16_t				xs;	// VS vertex starts here.
	int16_t				ys;
	float				zs;
	float				wcr;	// CS vertex ends here.
	float				r;
	float				g;
	float				b;	// VS vertex ends here.
} __attribute__((packed));

// The values are lifted up from d50.c.
static const struct vertex verts[] = {
	{0,	10/3.,	7/2.,	4,	0,	-3193,	0.94,0.25,1,0,0},
	{-5/2.,	-10/3.,	7/2.,	4,	-3195,	3193,	0.94,0.25,0,1,0},
	{5/2.,	-10/3.,	7/2.,	4,	3195,	3193,	0.94,0.25,0,0,1},
};

#if 0
static
void print_tile(int y, int x, uint32_t *arr)
{
	con_out("pt:[%d,%d], %x %x %x %x %x %x %x %x", y, x,
		arr[0], arr[1], arr[2], arr[3],
		arr[4], arr[5], arr[6], arr[7]);
}
#endif

int d51_run()
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

	static char v3dcr[PAGE_SIZE] __attribute__((aligned(CACHE_LINE_SIZE)));

	// Alignment enforced by TBMC/112.
	static uint32_t tsda[(NUM_TILES_Y * NUM_TILES_X * 48) >> 2]
		__attribute__((aligned(CACHE_LINE_SIZE)));

	// Alignment seems to be enfored by the max. tile allocation block
	// size of 256 bytes. This buffer is the binning memory pool
	// BPCA/BPCS.
	static uint32_t ta[PAGE_SIZE >> 2] __attribute__((aligned(256)));

	// Alignment enforced by NVSS/65.
	static struct v3dcr_gl_shader_state_rec	ssr
		__attribute__((aligned(CACHE_LINE_SIZE)));

	static const uint32_t cs_code[] __attribute__((aligned(8))) = {
		0x00701a00, 0xe0020c67, // li   vpr_setup, -, 0x701a00;
		0x009e7000, 0x100009e7, // ;
		0x009e7000, 0x100009e7, // ;
		0x009e7000, 0x100009e7, // ;
		0x15c00dc0, 0xd0020027, // ori  a0, vpr, 0;
		0x15c00dc0, 0xd0020067, // ori  a1, vpr, 0;
		0x15c00dc0, 0xd00200a7, // ori  a2, vpr, 0;
		0x15c00dc0, 0xd00200e7, // ori  a3, vpr, 0;
		0x15c00dc0, 0xd0020127, // ori  a4, vpr, 0;
		0x15c00dc0, 0xd0020167, // ori  a5, vpr, 0;
		0x15c00dc0, 0xd00201a7, // ori  a6, vpr, 0;

		0x00001a00, 0xe0021c67, // li   vpw_setup, -, 0x1a00;
		0x15027d80, 0x10020c27, // or   vpw, a0, a0;
		0x15067d80, 0x10020c27, // or   vpw, a1, a1;
		0x150a7d80, 0x10020c27, // or   vpw, a2, a2;
		0x150e7d80, 0x10020c27, // or   vpw, a3, a3;
		0x15127d80, 0x10020c27, // or   vpw, a4, a4;
		0x15167d80, 0x10020c27, // or   vpw, a5, a5;
		0x151a7d80, 0x10020c27, // or   vpw, a6, a6;

		0x159c1fc0, 0xd00209a7, // ori  host_int, 1, 1;
		0x009e7000, 0x300009e7, // pe;
		0x009e7000, 0x100009e7, // ;
		0x009e7000, 0x100009e7, // ;
	};

	static const uint32_t vs_code[] __attribute__((aligned(8))) = {
		0x00601a00, 0xe0020c67, // li	vpr_setup, -, 0x301a00;
		0x009e7000, 0x100009e7, // ;
		0x009e7000, 0x100009e7, // ;
		0x009e7000, 0x100009e7, // ;
		0x15c00dc0, 0xd0020027, // ori	a0, vpr, 0;
		0x15c00dc0, 0xd0020067, // ori	a1, vpr, 0;
		0x15c00dc0, 0xd00200a7, // ori	a2, vpr, 0;
		0x15c00dc0, 0xd00200e7, // ori	a3, vpr, 0;
		0x15c00dc0, 0xd0020127, // ori	a4, vpr, 0;
		0x15c00dc0, 0xd0020167, // ori	a5, vpr, 0;

		0x00001a00, 0xe0021c67, // li	vpw_setup, -, 0x1a00;
		0x15027d80, 0x10020c27, // or	vpw, a0, a0;
		0x15067d80, 0x10020c27, // or	vpw, a1, a1;
		0x150a7d80, 0x10020c27, // or	vpw, a2, a2;
		0x150e7d80, 0x10020c27, // or	vpw, a3, a3;
		0x15127d80, 0x10020c27, // or	vpw, a4, a4;
		0x15167d80, 0x10020c27, // or	vpw, a5, a5;

		0x159c1fc0, 0xd00209a7, // ori	host_int, 1, 1;
		0x009e7000, 0x300009e7, // pe;
		0x009e7000, 0x100009e7, // ;
		0x009e7000, 0x100009e7, // ;
	};

	static const uint32_t fs_code[] __attribute__((aligned(8))) = {
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

	ss->id = 64;
	ss->ssr_base = va_to_ba((va_t)&ssr);
	ss->ssr_base |= 2;	// # of attribute arrays.

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
	ssr.cs_attr_size = 28;
	ssr.cs_code_addr = va_to_ba((va_t)cs_code);
	ssr.attr[0].base = va_to_ba((va_t)verts);
	ssr.attr[0].num_bytes = 28 - 1;
	ssr.attr[0].stride = 40;

	ssr.vs_attr_sel = 2;
	ssr.vs_attr_size = 24;
	ssr.vs_code_addr = va_to_ba((va_t)vs_code);
	ssr.attr[1].base = va_to_ba((va_t)&verts[0].xs);
	ssr.attr[1].num_bytes = 24 - 1;
	ssr.attr[1].stride = 40;

	dc_cvac(v3dcr, off);
	dc_cvac(&ssr, sizeof(ssr));
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
	trmc->tb_base = pa_to_ba(fb_get_pa());
	trmc->width = FB_WIDTH;
	trmc->height = FB_HEIGHT;
	trmc->flags |= bits_set(V3DCR_TRMC_FLAGS_FBC_FMT, 1);	//RGBA8888

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
			// print_tile(y, x, (uint32_t *)tva);
		}
	}

	// Last tile signals the EOF.
	str->id = 25;

	dc_cvac(v3dcr, off);
	dsb();

	v3d_run_renderer(va_to_ba((va_t)v3dcr), off);
	return ERR_SUCCESS;
}

// The QPU automatically redirects the reads and writes from within the
// CS and VS to the appropriate locations reserved for this QPU in the VPM.

// Although it seems the shaders are reading and writing from Y=0, the actual
// rows are likely to be different. Not only that, the shaders are also not
// able to overwrite the rows containing the input vertices; the shaded
// vertices get written in the rows following those containing the input
// vertices.
//
// If the shaded vertices are not stored/written, the actual rows where the
// PTB/PSE expects the shaded vertices to be present contain previous/garbage
// data, which perhaps results in a render with just the clear colour and
// nothing else.

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
li	vpr_setup, -, 0x601a00;
;;;
ori	a0, vpr, 0;
ori	a1, vpr, 0;
ori	a2, vpr, 0;
ori	a3, vpr, 0;
ori	a4, vpr, 0;
ori	a5, vpr, 0;

li	vpw_setup, -, 0x1a00;
or	vpw, a0, a0;
or	vpw, a1, a1;
or	vpw, a2, a2;
or	vpw, a3, a3;
or	vpw, a4, a4;
or	vpw, a5, a5;

ori	host_int, 1, 1;
pe;;;
#endif

// Fragment Shader. The framebuffer format is BGRA8888, or 0xaarrggbb, or
// ARGB32.
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
