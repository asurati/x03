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
	static uint32_t in_buf[64];
	static uint32_t unif[6];

	// out_buf must be cache-line aligned and its size an integral
	// multiple of the cache-line size. See d2.c.
	static uint32_t out_buf[64] __attribute__((aligned(CACHE_LINE_SIZE)));

	static const uint32_t code[] __attribute__((aligned(8))) = {
		0x15800dc0, 0xd0020827,
		0x15800dc0, 0xd0020867,
		0x159e7000, 0x10020c67,
		0x159e7240, 0x10020ca7,
		0x15ca7c00, 0x100209e7,
		0x15800dc0, 0xd0020827,
		0x159e7000, 0x10020c67,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
		0x15c00dc0, 0xd0020027,
		0x15c00dc0, 0xd0020067,
		0x15c00dc0, 0xd00200a7,
		0x15c00dc0, 0xd00200e7,
		0x17027d80, 0x10020027,
		0x17067d80, 0x10020067,
		0x170a7d80, 0x100200a7,
		0x170e7d80, 0x100200e7,
		0x15800dc0, 0xd0020827,
		0x159e7000, 0x10021c67,
		0x15027d80, 0x10020c27,
		0x15067d80, 0x10020c27,
		0x150a7d80, 0x10020c27,
		0x150e7d80, 0x10020c27,
		0x15800dc0, 0xd0020027,
		0x15800dc0, 0xd0020067,
		0x00000800, 0xe00248a7,
		0x00000040, 0xe00248e7,
		0x00000000, 0xe0024867,
		0x0d9c43c0, 0xd0022827,
		0x00000038, 0xf02809e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
		0x15027d80, 0x10021c67,
		0x15067d80, 0x10021ca7,
		0x159f2e00, 0x100209e7,
		0xffffffa0, 0xf0f809e7,
		0x0c9c13c0, 0xd0020867,
		0x0c027c80, 0x10020027,
		0x0c067cc0, 0x10020067,
		0x159c1fc0, 0xd00209a7,
		0x009e7000, 0x300009e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
	};

	srand(tmr_get_ctr());

	for (i = 0; i < 64; ++i)
		in_buf[i] = rand();

	// RAM->VPM the 4 vectors vertically starting at (y=0,x=15).
	vdr = 0;
	vdr |= bits_set(VDR_ADDR_X, 15);
	vdr |= bits_on(VDR_VERT);
	vdr |= bits_set(VDR_NROWS, 4);
	vdr |= bits_set(VDR_MPITCH, 3);
	vdr |= bits_on(VDR_ID);
	unif[0] = vdr;
	unif[1] = va_to_ba((va_t)in_buf);

	// VPM->QPU the vectors from (y=0,x=15)
	vpr = 0;
	vpr |= bits_set(VPR_V32_ADDR_X, 15);
	vpr |= bits_set(VPR_SIZE, 2);
	vpr |= bits_set(VPR_STRIDE, 16);	// Increments y
	vpr |= bits_set(VPR_NUM, 4);
	unif[2] = vpr;	// One vpr_setup for 4 reads.

	// NOT the vectors

	// QPU->VPM the vectors into (y=0,x=15)
	vpw = 0;
	vpw |= bits_set(VPW_V32_ADDR_X, 15);
	vpw |= bits_set(VPW_SIZE, 2);
	vpw |= bits_set(VPW_STRIDE, 16);	// Increments y
	unif[3] = vpw;	// One vpw_setup for 4 writes.

	// VPM->RAM the vector from (y=0,x=15)
	vdw = 0;
	vdw |= bits_set(VDW_ADDR_X, 15);
	vdw |= bits_set(VDW_DEPTH, 16);	// in units of vdw.WIDTH.
	vdw |= bits_set(VDW_UNITS, 1);	// # of rows.
	vdw |= bits_set(VDW_ID, 2);
	unif[4] = vdw;
	unif[5] = va_to_ba((va_t)&out_buf[0]);

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

	for (i = 0; i < 64; ++i)
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
;;;	# 3-instruction delay slot before reading.
ori	a0, vpr, 0;
ori	a1, vpr, 0;
ori	a2, vpr, 0;
ori	a3, vpr, 0;

not	a0, a0, a0;
not	a1, a1, a1;
not	a2, a2, a2;
not	a3, a3, a3;

ori	r0, uni_rd, 0;
or	vpw_setup, r0, r0;
or	vpw, a0, a0;
or	vpw, a1, a1;
or	vpw, a2, a2;
or	vpw, a3, a3;

ori	a0, uni_rd, 0;
ori	a1, uni_rd, 0;

# Increment the vdw.y by 16.
li	r2, -, 0x800;

# Increment the out_buf address by 16*4 = 64
li	r3, -, 64;

li	r1, -, 0;
loop:
subi	r0, r1, 4	sf;
b.z	done;
;;;
or	vdw_setup, a0, a0;
or	vdw_addr, a1, a1;
or	-, vdw_wait, r0;
b	loop;
addi	r1, r1, 1;
add	a0, a0, r2;
add	a1, a1, r3;
done:

ori	host_int, 1, 1;
pe;;;
#endif
