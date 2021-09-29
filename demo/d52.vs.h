// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <stdint.h>

static const uint32_t vs_code[] __attribute__((aligned(8))) = {
	0x00701a00, 0xe0020c67, // li	vpr_setup, -, 0x701a00;
	0x009e7000, 0x100009e7, // ;
	0x009e7000, 0x100009e7, // ;
	0x009e7000, 0x100009e7, // ;
	0x15c00dc0, 0xd0020027, // ori	a0, vpr, 0;
	0x15c00dc0, 0xd0020067, // ori	a1, vpr, 0;
	0x15c00dc0, 0xd00200a7, // ori	a2, vpr, 0;
	0x15c00dc0, 0xd00200e7, // ori	a3, vpr, 0;
	0x15c00dc0, 0xd0021027, // ori	b0, vpr, 0;
	0x15c00dc0, 0xd0021067, // ori	b1, vpr, 0;
	0x15c00dc0, 0xd00210a7, // ori	b2, vpr, 0;
	0x000001b8, 0xf0f807e7, // bl	a31, func_row_wise_mult;
	0x009e7000, 0x100009e7, // ;
	0x009e7000, 0x100009e7, // ;
	0x009e7000, 0x100009e7, // ;
	0x159e7000, 0x10020127, // or	a4, r0, r0;
	0x00000190, 0xf0f807e7, // bl	a31, func_row_wise_mult;
	0x009e7000, 0x100009e7, // ;
	0x009e7000, 0x100009e7, // ;
	0x009e7000, 0x100009e7, // ;
	0x159e7000, 0x10020167, // or	a5, r0, r0;
	0x00000168, 0xf0f807e7, // bl	a31, func_row_wise_mult;
	0x009e7000, 0x100009e7, // ;
	0x009e7000, 0x100009e7, // ;
	0x009e7000, 0x100009e7, // ;
	0x159e7000, 0x100201a7, // or	a6, r0, r0;
	0x00000140, 0xf0f807e7, // bl	a31, func_row_wise_mult;
	0x009e7000, 0x100009e7, // ;
	0x009e7000, 0x100009e7, // ;
	0x009e7000, 0x100009e7, // ;
	0x159e7000, 0x100201e7, // or	a7, r0, r0;
	0x159e7000, 0x10020d27, // or	sfu_recip, r0, r0;
	0x009e7000, 0x100009e7, // ;
	0x009e7000, 0x100009e7, // ;
	0x159c09c0, 0xd0020827, // ori	r0, r4, 0;
	0x20127030, 0x100059c0, // fmul	a0, a4, r0;
	0x20167030, 0x100059c1, // fmul	a1, a5, r0;
	0x201a7030, 0x100059c2, // fmul	a2, a6, r0;
	0x159e7000, 0x100200e7, // or	a3, r0, r0;
	0x02040f80, 0xd0020067, // fsubi	a1, 0, a1;
	0x0000013f, 0xe0020827, // li	r0, -, 319;
	0x089e7000, 0x10020827, // itof	r0, r0, r0;
	0x019ef1c0, 0xd0020827, // faddi	r0, r0, i2f;
	0x20027030, 0x100059c0, // fmul	a0, a0, r0;
	0x000000ef, 0xe0020827, // li	r0, -, 239;
	0x089e7000, 0x10020827, // itof	r0, r0, r0;
	0x019ef1c0, 0xd0020827, // faddi	r0, r0, i2f;
	0x20067030, 0x100059c1, // fmul	a1, a1, r0;
	0x010a0dc0, 0xd0020827, // faddi	r0, a2, 1f;
	0x209ef007, 0xd00059c2, // fmuli	a2, r0, i2f;
	0x20064037, 0xd00059c1, // fmuli	a1, a1, 16f;
	0x20024037, 0xd00059c0, // fmuli	a0, a0, 16f;
	0x07067d80, 0x10020067, // ftoi	a1, a1, a1;
	0x07027d80, 0x10020027, // ftoi	a0, a0, a0;
	0x0000ffff, 0xe0020827, // li	r0, -, 0xffff;
	0x14027c00, 0x10020027, // and	a0, a0, r0;
	0x11048dc0, 0xd0020827, // shli	r0, a1, 8;
	0x119c81c0, 0xd0020827, // shli	r0, r0, 8;
	0x15027c00, 0x10020027, // or	a0, a0, r0;
	0x00001a00, 0xe0021c67, // li	vpw_setup, -, 0x1a00;
	0x15027d80, 0x10020c27, // or	vpw, a0, a0;
	0x150a7d80, 0x10020c27, // or	vpw, a2, a2;
	0x150e7d80, 0x10020c27, // or	vpw, a3, a3;
	0x159c0fc0, 0x10020c27, // or	vpw, b0, b0;
	0x159c1fc0, 0x10020c27, // or	vpw, b1, b1;
	0x159c2fc0, 0x10020c27, // or	vpw, b2, b2;
	0x159c1fc0, 0xd00209a7, // ori	host_int, 1, 1;
	0x009e7000, 0x300009e7, // pe;
	0x009e7000, 0x100009e7, // ;
	0x009e7000, 0x100009e7, // ;
	0x00000000, 0xe0020827, // func_row_wise_mult: li	r0, -, 0;
	0x15800dc0, 0xd0020867, // ori	r1, uni_rd, 0;
	0x20027031, 0x100049e1, // fmul	r1, a0, r1;
	0x019e7040, 0x10020827, // fadd	r0, r0, r1;
	0x15800dc0, 0xd0020867, // ori	r1, uni_rd, 0;
	0x20067031, 0x100049e1, // fmul	r1, a1, r1;
	0x019e7040, 0x10020827, // fadd	r0, r0, r1;
	0x15800dc0, 0xd0020867, // ori	r1, uni_rd, 0;
	0x200a7031, 0x100049e1, // fmul	r1, a2, r1;
	0x019e7040, 0x10020827, // fadd	r0, r0, r1;
	0x00000000, 0xf0f7e9e7, // b	a31;
	0x15800dc0, 0xd0020867, // ori	r1, uni_rd, 0;
	0x200e7031, 0x100049e1, // fmul	r1, a3, r1;
	0x019e7040, 0x10020827, // fadd	r0, r0, r1;
};

// Vertex Shader
#if 0
li	vpr_setup, -, 0x701a00;	# 7 horizontal rows.
;;;	# 3 instruction delay
ori	a0, vpr, 0;	# xo
ori	a1, vpr, 0;	# yo
ori	a2, vpr, 0;	# zo
ori	a3, vpr, 0;	# wo
ori	b0, vpr, 0;	# r
ori	b1, vpr, 0;	# g
ori	b2, vpr, 0;	# b

#############################################################
bl	a31, func_row_wise_mult;;;;
or	a4, r0, r0;	# a4 has the xc for the given vertices.

bl	a31, func_row_wise_mult;;;;
or	a5, r0, r0;	# a5 has the yc for the given vertices.

bl	a31, func_row_wise_mult;;;;
or	a6, r0, r0;	# a6 has the zc for the given vertices.

bl	a31, func_row_wise_mult;;;;
or	a7, r0, r0;	# a7 has the wc for the given vertices.


#############################################################
or	sfu_recip, r0, r0; # r0 has wc
;;	# Delay slot for SFU

ori	r0, r4, 0;	# wcr

fmul	a0, a4, r0;	# xc -> xndc
fmul	a1, a5, r0;	# yc -> yndc
fmul	a2, a6, r0;	# zc -> zndc

or	a3, r0, r0;	# wcr

# Invert the sign of yndc
fsubi	a1, 0, a1;

#############################################################
# Perform ViewPort Transform. Hardcoded for 640x480 for now.

# xs = 319.5 * xndc;
# ys = 239.5 * yndc.
# zs = 0.5 * zndc + 0.5.

li	r0, -, 319;
itof	r0, r0, r0;
faddi	r0, r0, i2f;
fmul	a0, a0, r0;	# xndc -> xs

li	r0, -, 239;
itof	r0, r0, r0;
faddi	r0, r0, i2f;
fmul	a1, a1, r0;	# yndc -> ys

faddi	r0, a2, 1f;
fmuli	a2, r0, i2f;	# zndc -> zs

# Convert xs and ys into 12.4 fixed-point. For now, with zero
# fractional component.
fmuli	a1, a1, 16f;
fmuli	a0, a0, 16f;

ftoi	a1, a1, a1;
ftoi	a0, a0, a0;

# Place them within the same 32-bit word
li	r0, -, 0xffff;
and	a0, a0, r0;	# Zero the upper 16-bits of xs.

shli	r0, a1, 8;	# Shift Left ys by 16 bits.
shli	r0, r0, 8;
or	a0, a0, r0;


#############################################################
# a4 xc
# a5 yc
# a6 zc
# a7 wc
# a0 ys | xs
# a2 zs
# a3 wcr
# b0 r
# b1 g
# b2 g

li	vpw_setup, -, 0x1a00;
or	vpw, a0, a0;
or	vpw, a2, a2;
or	vpw, a3, a3;
or	vpw, b0, b0;
or	vpw, b1, b1;
or	vpw, b2, b2;

# Write VPM into RAM, for verification.
#li	vdw_setup, -, 0x80104000;
#ori	vdw_addr, uni_rd, 0;
#or	-, vdw_wait, r0;

ori	host_int, 1, 1;
pe;;;


#############################################################
func_row_wise_mult:
li	r0, -, 0;	# Output

ori	r1, uni_rd, 0;	# m0
fmul	r1, a0, r1;	# m0*xo
fadd	r0, r0, r1;

ori	r1, uni_rd, 0;	# m1
fmul	r1, a1, r1;	# m1*yo
fadd	r0, r0, r1;

ori	r1, uni_rd, 0;	# m2
fmul	r1, a2, r1;	# m2*zo
fadd	r0, r0, r1;

b	a31;
ori	r1, uni_rd, 0;	# m2
fmul	r1, a3, r1;	# m3*wo
fadd	r0, r0, r1;
#endif
