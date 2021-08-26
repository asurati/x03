// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdlib.h>

#include <sys/cpu.h>

#include <dev/v3d.h>
#include <dev/tmr.h>
#include <dev/con.h>

// Store vertically, in two columns, with each column consisting of 4
// 16-element vectors.
// v0.0 (y=0,x=0)      v4 (y=0,x=1)
// v0.1
// ...
// v0.15
//
// v1.0 (y=16,x=0)     v5 (y=16,x=2)
// v1.1
// ...
// v1.15
//
// v2.0 (y=32,x=0)     v6
// v2.1
// .
// v2.15
//
// v3.0 (y=48,x=0)     v7
// v3.1
// .
// v3.15
//
// Total 128 words.

static
uint32_t d2_get_vdw_setup(int y, int x)
{
	uint32_t vdw;

	vdw = 0;
	vdw |= bits_set(VDW_ADDR_Y, y);
	vdw |= bits_set(VDW_ADDR_X, x);
	vdw |= bits_set(VDW_DEPTH, 64);
	vdw |= bits_set(VDW_UNITS, 1);
	vdw |= bits_set(VDW_ID, 2);
	return vdw;
}

int d2_run()
{
	int err, i, j;
	uint32_t vdr;
	pa_t code_ba, unif_ba;
	static uint32_t in_buf[64];
	static uint32_t unif[10];

	// Allocate out_buf such that it is aligned at the cache-line
	// boundary, so that it does not share cache-lines with any other
	// variable. This is needed because we must invalidate the out_buf
	// before reading from the memory, but if there are variables which
	// share the cache-line with out_buf, we may lose their most recent
	// values because of the invalidate.
	static uint32_t out_buf[64] __attribute__((aligned(32)));

	static const uint32_t code[] __attribute__((aligned(8))) = {
		0x15800dc0, 0xd0020827,
		0x15800dc0, 0xd0020867,
		0x159e7000, 0x10020c67,
		0x159e7240, 0x10020ca7,
		0x15ca7c00, 0x100209e7,

		0x15800dc0, 0xd0020827,
		0x15800dc0, 0xd0020867,
		0x159e7000, 0x10021c67,
		0x159e7240, 0x10021ca7,
		0x159f2e00, 0x100209e7,

		0x15800dc0, 0xd0020827,
		0x15800dc0, 0xd0020867,
		0x159e7000, 0x10021c67,
		0x159e7240, 0x10021ca7,
		0x159f2e00, 0x100209e7,

		0x15800dc0, 0xd0020827,
		0x15800dc0, 0xd0020867,
		0x159e7000, 0x10021c67,
		0x159e7240, 0x10021ca7,
		0x159f2e00, 0x100209e7,

		0x15800dc0, 0xd0020827,
		0x15800dc0, 0xd0020867,
		0x159e7000, 0x10021c67,
		0x159e7240, 0x10021ca7,
		0x159f2e00, 0x100209e7,

		0x159c1fc0, 0xd00209a7,
		0x009e7000, 0x300009e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
	};

	srand(tmr_get_ctr());

	for (i = 0; i < 64; ++i)
		in_buf[i] = rand();

	// RAM->VPM the first 64 words into (y=0,x=15).
	vdr = 0;
	vdr |= bits_set(VDR_ADDR_X, 15);
	vdr |= bits_on(VDR_VERT);
	vdr |= bits_set(VDR_NROWS, 4);
	vdr |= bits_set(VDR_MPITCH, 3);	// 8 * (1 << 3) = 64 bytes = 16 words.
	vdr |= bits_on(VDR_ID);
	unif[0] = vdr;
	unif[1] = va_to_ba((va_t)&in_buf[0]);

	for (i = 0, j = 2; i < 4; ++i) {
		// VPM->RAM vi (y=i*16,x=15).
		unif[j++] = d2_get_vdw_setup(i * 16, 15);
		unif[j++] = va_to_ba((va_t)&out_buf[i * 16]);
	}

	code_ba = va_to_ba((va_t)code);
	unif_ba = va_to_ba((va_t)unif);
	dc_cvac(in_buf, sizeof(in_buf));
	dc_cvac(unif, sizeof(unif));
	dc_civac(out_buf, sizeof(out_buf));
	dsb();

	err = v3d_run_prog(code_ba, unif_ba, sizeof(unif));
	if (err)
		return err;
	dc_ivac(out_buf, sizeof(out_buf));
	dsb();

	for (i = 0; i < 64; ++i)
		con_out("d2: [%d]: %x, %x", i, in_buf[i], out_buf[i]);
	return err;
}

#if 0
ori	r0, uni_rd, 0;
ori	r1, uni_rd, 0;
or	vpm_rd_setup, r0, r0;
or	vpm_ld_addr, r1, r1;
or	-, vpm_ld_wait, r0;

ori	r0, uni_rd, 0;
ori	r1, uni_rd, 0;
or	vpm_wr_setup, r0, r0;
or	vpm_st_addr, r1, r1;
or	-, vpm_st_wait, r0;

ori	r0, uni_rd, 0;
ori	r1, uni_rd, 0;
or	vpm_wr_setup, r0, r0;
or	vpm_st_addr, r1, r1;
or	-, vpm_st_wait, r0;

ori	r0, uni_rd, 0;
ori	r1, uni_rd, 0;
or	vpm_wr_setup, r0, r0;
or	vpm_st_addr, r1, r1;
or	-, vpm_st_wait, r0;

ori	r0, uni_rd, 0;
ori	r1, uni_rd, 0;
or	vpm_wr_setup, r0, r0;
or	vpm_st_addr, r1, r1;
or	-, vpm_st_wait, r0;

ori	host_int, 1, 1;
pe;;;
#endif
