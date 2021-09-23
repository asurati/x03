// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdlib.h>

#include <sys/cpu.h>

#include <dev/v3d.h>
#include <dev/tmr.h>
#include <dev/con.h>

// Transpose a 4x4 matrix. The matrices are stored in a row-major order.
int d4_run()
{
	int err, i;
	uint32_t *p32;
	pa_t code_ba, unif_ba;
	static uint32_t in_mat[4][4];
	static uint32_t unif[2];

	// out_buf must be cache-line aligned and its size an integral
	// multiple of the cache-line size. See d2.c.
	static uint32_t out_mat[4][4]
		__attribute__((aligned(CACHE_LINE_SIZE)));

	static const uint32_t code[] __attribute__((aligned(8))) = {
		0x80010000, 0xe0020c67,
		0x15800dc0, 0xd0020827,
		0x159e7000, 0x10020ca7,
		0x15ca7c00, 0x100209e7,
		0x00100a00, 0xe0020c67,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
		0x15c00dc0, 0xd0020827,
		0x00000050, 0xf0f807e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
		0x00000a00, 0xe0021c67,
		0x15767d80, 0x10020c27,
		0x80904000, 0xe0021c67,
		0x15800dc0, 0xd0020827,
		0x159e7000, 0x10021ca7,
		0x159f2e00, 0x100209e7,
		0x159c1fc0, 0xd00209a7,
		0x009e7000, 0x300009e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
		0x00000000, 0xe0020767,
		0x00010001, 0xe2020867,
		0x00000000, 0xe00208a7,
		0x149e7040, 0x100208e7,
		0x15767cc0, 0x10020767,
		0xc09fd002, 0xd00049e0,
		0xc09f100a, 0xd00049e1,
		0x149e7040, 0x100208e7,
		0x15767cc0, 0x10020767,
		0xc09fd002, 0xd00049e0,
		0xc09f100a, 0xd00049e1,
		0x149e7040, 0x100208e7,
		0x15767cc0, 0x10020767,
		0xc09fd002, 0xd00049e0,
		0xc09f100a, 0xd00049e1,
		0x149e7040, 0x100208e7,
		0x15767cc0, 0x10020767,
		0xc09fc002, 0xd00049e0,
		0xc09f100a, 0xd00049e1,
		0x149e7040, 0x100208e7,
		0x15767cc0, 0x10020767,
		0xc09fd002, 0xd00049e0,
		0xc09f100a, 0xd00049e1,
		0x149e7040, 0x100208e7,
		0x15767cc0, 0x10020767,
		0xc09fd002, 0xd00049e0,
		0xc09f100a, 0xd00049e1,
		0x149e7040, 0x100208e7,
		0x15767cc0, 0x10020767,
		0xc09fd002, 0xd00049e0,
		0xc09f100a, 0xd00049e1,
		0x149e7040, 0x100208e7,
		0x15767cc0, 0x10020767,
		0xc09fc002, 0xd00049e0,
		0xc09f100a, 0xd00049e1,
		0x149e7040, 0x100208e7,
		0x15767cc0, 0x10020767,
		0xc09fd002, 0xd00049e0,
		0xc09f100a, 0xd00049e1,
		0x149e7040, 0x100208e7,
		0x15767cc0, 0x10020767,
		0xc09fd002, 0xd00049e0,
		0xc09f100a, 0xd00049e1,
		0x149e7040, 0x100208e7,
		0x15767cc0, 0x10020767,
		0xc09fd002, 0xd00049e0,
		0xc09f100a, 0xd00049e1,
		0x149e7040, 0x100208e7,
		0x15767cc0, 0x10020767,
		0xc09fc002, 0xd00049e0,
		0xc09f100a, 0xd00049e1,
		0x149e7040, 0x100208e7,
		0x15767cc0, 0x10020767,
		0xc09fd002, 0xd00049e0,
		0xc09f100a, 0xd00049e1,
		0x149e7040, 0x100208e7,
		0x15767cc0, 0x10020767,
		0xc09fd002, 0xd00049e0,
		0xc09f100a, 0xd00049e1,
		0x149e7040, 0x100208e7,
		0x15767cc0, 0x10020767,
		0xc09fd002, 0xd00049e0,
		0xc09f100a, 0xd00049e1,
		0x00000000, 0xf0f7e9e7,
		0x149e7040, 0x100208e7,
		0x15767cc0, 0x10020767,
		0x15767d80, 0x10020827,
	};

	srand(tmr_get_ctr());

	p32 = &in_mat[0][0];
	for (i = 0; i < 4 * 4; ++i)
		p32[i] = rand();

	unif[0] = va_to_ba((va_t)in_mat);
	unif[1] = va_to_ba((va_t)out_mat);

	code_ba = va_to_ba((va_t)code);
	unif_ba = va_to_ba((va_t)unif);
	dc_cvac(in_mat, sizeof(in_mat));
	dc_cvac(unif, sizeof(unif));
	dsb();

	err = v3d_run_prog(code_ba, unif_ba, sizeof(unif));
	if (err)
		return err;
	dc_ivac(out_mat, sizeof(out_mat));
	dsb();

	p32 = &in_mat[0][0];
	for (i = 0; i < 4; ++i)
		con_out("d4: %x %x %x %x",
			p32[i * 4 + 0],
			p32[i * 4 + 1],
			p32[i * 4 + 2],
			p32[i * 4 + 3]);

	p32 = &out_mat[0][0];
	for (i = 0; i < 4; ++i)
		con_out("d4: %x %x %x %x",
			p32[i * 4 + 0],
			p32[i * 4 + 1],
			p32[i * 4 + 2],
			p32[i * 4 + 3]);
	return err;
}

#if 0
# Read the matrix into VPM.
li	vdr_setup, -, 0x80010000;	# Single Row.
ori	r0, uni_rd, 0;			# Address
or	vdr_addr, r0, r0;
or	-, vdr_wait, r0;

# Read it into QPU.
li	vpr_setup, -, 0x100a00;
;;;	# 3-instruction delay slot before reading.
ori	r0, vpr, 0; # matrix inside r0

bl	a31, transpose; #a31 is the link register
;;;


# Write into VPM.
li	vpw_setup, -, 0xa00;
or	vpw, a29, a29;

# Write into RAM.
li	vdw_setup, -, 0x80904000;
ori	r0, uni_rd, 0;			# Address
or	vdw_addr, r0, r0;
or	-, vdw_wait, r0;

ori	host_int, 1, 1;
pe;;;


transpose:
li	a29, -, 0;		# output
lis	r1, -, 0x10001;		# mask
li	r2, -, 0;

# m0 m1 m2 m3 m4 m5 m6 m7 m8 m9 ma mb mc md me mf
#m 1
and	r3, r0, r1;
or	a29, a29, r3;

# m3 m4 m5 m6 m7 m8 m9 ma mb mc md me mf m0 m1 m2
#m    1
v8asrot13	r0, r0, r2;	# Rotate input
v8asrot1	r1, r1, r2;	# Rotate mask
and	r3, r0, r1;
or	a29, a29, r3;

# m6 m7 m8 m9 ma mb mc md me mf m0 m1 m2 m3 m4 m5
#m       1
v8asrot13	r0, r0, r2;	# Rotate input
v8asrot1	r1, r1, r2;	# Rotate mask
and	r3, r0, r1;
or	a29, a29, r3;

# m9 ma mb mc md me mf m0 m1 m2 m3 m4 m5 m6 m7 m8
#m          1
v8asrot13	r0, r0, r2;	# Rotate input
v8asrot1	r1, r1, r2;	# Rotate mask
and	r3, r0, r1;
or	a29, a29, r3;

# md me mf m0 m1 m2 m3 m4 m5 m6 m7 m8 m9 ma mb mc
#m             1
v8asrot12	r0, r0, r2;	# Rotate input
v8asrot1	r1, r1, r2;	# Rotate mask
and	r3, r0, r1;
or	a29, a29, r3;

# m0 m1 m2 m3 m4 m5 m6 m7 m8 m9 ma mb mc md me mf
#m                1
v8asrot13	r0, r0, r2;	# Rotate input
v8asrot1	r1, r1, r2;	# Rotate mask
and	r3, r0, r1;
or	a29, a29, r3;

# m3 m4 m5 m6 m7 m8 m9 ma mb mc md me mf m0 m1 m2
#m                   1
v8asrot13	r0, r0, r2;	# Rotate input
v8asrot1	r1, r1, r2;	# Rotate mask
and	r3, r0, r1;
or	a29, a29, r3;

# m6 m7 m8 m9 ma mb mc md me mf m0 m1 m2 m3 m4 m5
#m                      1
v8asrot13	r0, r0, r2;	# Rotate input
v8asrot1	r1, r1, r2;	# Rotate mask
and	r3, r0, r1;
or	a29, a29, r3;

# ma mb mc md me mf m0 m1 m2 m3 m4 m5 m6 m7 m8 m9
#m                         1
v8asrot12	r0, r0, r2;	# Rotate input
v8asrot1	r1, r1, r2;	# Rotate mask
and	r3, r0, r1;
or	a29, a29, r3;

# md me mf m0 m1 m2 m3 m4 m5 m6 m7 m8 m9 ma mb mc
#m                            1
v8asrot13	r0, r0, r2;	# Rotate input
v8asrot1	r1, r1, r2;	# Rotate mask
and	r3, r0, r1;
or	a29, a29, r3;

# m0 m1 m2 m3 m4 m5 m6 m7 m8 m9 ma mb mc md me mf
#m                               1
v8asrot13	r0, r0, r2;	# Rotate input
v8asrot1	r1, r1, r2;	# Rotate mask
and	r3, r0, r1;
or	a29, a29, r3;

# m3 m4 m5 m6 m7 m8 m9 ma mb mc md me mf m0 m1 m2
#m                                  1
v8asrot13	r0, r0, r2;	# Rotate input
v8asrot1	r1, r1, r2;	# Rotate mask
and	r3, r0, r1;
or	a29, a29, r3;

# m7 m8 m9 ma mb mc md me mf m0 m1 m2 m3 m4 m5 m6
#m                                     1
v8asrot12	r0, r0, r2;	# Rotate input
v8asrot1	r1, r1, r2;	# Rotate mask
and	r3, r0, r1;
or	a29, a29, r3;

# ma mb mc md me mf m0 m1 m2 m3 m4 m5 m6 m7 m8 m9
#m                                        1
v8asrot13	r0, r0, r2;	# Rotate input
v8asrot1	r1, r1, r2;	# Rotate mask
and	r3, r0, r1;
or	a29, a29, r3;

# md me mf m0 m1 m2 m3 m4 m5 m6 m7 m8 m9 ma mb mc
#m                                           1
v8asrot13	r0, r0, r2;	# Rotate input
v8asrot1	r1, r1, r2;	# Rotate mask
and	r3, r0, r1;
or	a29, a29, r3;

# m0 m1 m2 m3 m4 m5 m6 m7 m8 m9 ma mb mc md me mf
#m                                              1
v8asrot13	r0, r0, r2;	# Rotate input
v8asrot1	r1, r1, r2;	# Rotate mask
b	a31;
and	r3, r0, r1;
or	a29, a29, r3;
or	r0, a29, a29;
#endif
