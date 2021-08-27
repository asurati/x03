// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdlib.h>

#include <sys/cpu.h>

#include <dev/v3d.h>
#include <dev/tmr.h>
#include <dev/con.h>

int d3_run()
{
	int err, i;
	uint32_t vdr, vdw, vpr, vpw;
	pa_t code_ba, unif_ba;
	static uint32_t in_buf[16];
	static uint32_t unif[6];

	// out_buf must be cache-line aligned and its size an integral
	// multiple of the cache-line size. See d2.c.
	static uint32_t out_buf[16] __attribute__((aligned(32)));

	static const uint32_t code[] __attribute__((aligned(8))) = {
		0x15800dc0, 0xd0020827,
		0x15800dc0, 0xd0020867,
		0x159e7000, 0x10020c67,
		0x159e7240, 0x10020ca7,
		0x15ca7c00, 0x100209e7,

		0x15800dc0, 0xd0020827,
		0x159e7000, 0x10020c67,
		0x15c00dc0, 0xd0020867,

		0x179e7240, 0x10020867,

		0x15800dc0, 0xd0020827,
		0x159e7000, 0x10021c67,
		0x159e7240, 0x10020c27,

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

	for (i = 0; i < 16; ++i)
		in_buf[i] = rand();

	// RAM->VPM the vector into (y=48,x=15).
	vdr = 0;
	vdr |= bits_set(VDR_ADDR_Y, 48);
	vdr |= bits_set(VDR_ADDR_X, 15);
	vdr |= bits_on(VDR_VERT);
	vdr |= bits_set(VDR_NROWS, 1);
	vdr |= bits_on(VDR_ID);
	unif[0] = vdr;
	unif[1] = va_to_ba((va_t)in_buf);

	// VPM->QPU the vector from (y=3(*16),x=15)
	vpr = 0;
	vpr |= bits_set(VPR_V32_ADDR_Y, 3);
	vpr |= bits_set(VPR_V32_ADDR_X, 15);
	vpr |= bits_set(VPR_SIZE, 2);
	vpr |= bits_set(VPR_NUM, 1);
	unif[2] = vpr;

	// QPU->VPM the vector into (y=3(*16),x=15)
	vpw = 0;
	vpw |= bits_set(VPW_V32_ADDR_Y, 3);
	vpw |= bits_set(VPW_V32_ADDR_X, 15);
	vpw |= bits_set(VPW_SIZE, 2);
	unif[3] = vpw;

	// RAM->VPM the vector from (y=48,x=15)
	vdw = 0;
	vdw |= bits_set(VDW_ADDR_Y, 48);
	vdw |= bits_set(VDW_ADDR_X, 15);
	vdw |= bits_set(VDW_DEPTH, 64);
	vdw |= bits_set(VDW_UNITS, 1);
	vdw |= bits_set(VDW_ID, 2);
	unif[4] = vdw;
	unif[5] = va_to_ba((va_t)out_buf);

	code_ba = va_to_ba((va_t)code);
	unif_ba = va_to_ba((va_t)unif);
	dc_cvac(in_buf, sizeof(in_buf));
	dc_cvac(unif, sizeof(unif));
	dsb();

	err = v3d_run_prog(code_ba, unif_ba, sizeof(unif));
	if (err)
		return err;
	dc_ivac(out_buf, sizeof(out_buf));
	dsb();

	for (i = 0; i < 16; ++i)
		con_out("d3: [%d]: %x, %x, %x", i, in_buf[i], out_buf[i],
			in_buf[i] | out_buf[i]);
	return err;
}

#if 0
ori	r0, uni_rd, 0;
ori	r1, uni_rd, 0;
or	vdr_setup, r0, r0;
or	vdr_addr, r1, r1;
or	-, vdr_wait, r0;

ori	r0, uni_rd, 0;
or	vpr_setup, r0, r0;
ori	r1, vpr, 0;

not	r1, r1, r1;

ori	r0, uni_rd, 0;
or	vpw_setup, r0, r0;
or	vpw, r1, r1;

ori	r0, uni_rd, 0;
ori	r1, uni_rd, 0;
or	vdw_setup, r0, r0;
or	vdw_addr, r1, r1;
or	-, vdw_wait, r0;

ori	host_int, 1, 1;
pe;;;
#endif
