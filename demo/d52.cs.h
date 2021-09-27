// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <stdint.h>

static const uint32_t cs_code[] __attribute__((aligned(8))) = {
	0x00401a00, 0xe0020c67,
	0x009e7000, 0x100009e7,
	0x009e7000, 0x100009e7,
	0x009e7000, 0x100009e7,
	0x15c00dc0, 0xd0020027,
	0x15c00dc0, 0xd0020067,
	0x15c00dc0, 0xd00200a7,
	0x15c00dc0, 0xd00200e7,
	0x000001c0, 0xf0f807e7,
	0x009e7000, 0x100009e7,
	0x009e7000, 0x100009e7,
	0x009e7000, 0x100009e7,
	0x159e7000, 0x10020127,
	0x00000198, 0xf0f807e7,
	0x009e7000, 0x100009e7,
	0x009e7000, 0x100009e7,
	0x009e7000, 0x100009e7,
	0x159e7000, 0x10020167,
	0x00000170, 0xf0f807e7,
	0x009e7000, 0x100009e7,
	0x009e7000, 0x100009e7,
	0x009e7000, 0x100009e7,
	0x159e7000, 0x100201a7,
	0x00000148, 0xf0f807e7,
	0x009e7000, 0x100009e7,
	0x009e7000, 0x100009e7,
	0x009e7000, 0x100009e7,
	0x159e7000, 0x100201e7,
	0x159e7000, 0x10020d27,
	0x009e7000, 0x100009e7,
	0x009e7000, 0x100009e7,
	0x159c09c0, 0xd0020827,
	0x20127030, 0x100059c0,
	0x20167030, 0x100059c1,
	0x201a7030, 0x100059c2,
	0x159e7000, 0x100200e7,
	0x02040f80, 0xd0020067,
	0x0000013f, 0xe0020827,
	0x089e7000, 0x10020827,
	0x019ef1c0, 0xd0020827,
	0x20027030, 0x100059c0,
	0x000000ef, 0xe0020827,
	0x089e7000, 0x10020827,
	0x019ef1c0, 0xd0020827,
	0x20067030, 0x100059c1,
	0x010a0dc0, 0xd0020827,
	0x209ef007, 0xd00059c2,
	0x20064037, 0xd00059c1,
	0x20024037, 0xd00059c0,
	0x07067d80, 0x10020067,
	0x07027d80, 0x10020027,
	0x0000ffff, 0xe0020827,
	0x14027c00, 0x10020027,
	0x11048dc0, 0xd0020827,
	0x119c81c0, 0xd0020827,
	0x15027c00, 0x10020027,
	0x00001a00, 0xe0021c67,
	0x15127d80, 0x10020c27,
	0x15167d80, 0x10020c27,
	0x151a7d80, 0x10020c27,
	0x151e7d80, 0x10020c27,
	0x15027d80, 0x10020c27,
	0x150a7d80, 0x10020c27,
	0x150e7d80, 0x10020c27,
	0x159c1fc0, 0xd00209a7,
	0x009e7000, 0x300009e7,
	0x009e7000, 0x100009e7,
	0x009e7000, 0x100009e7,
	0x00000000, 0xe0020827,
	0x15800dc0, 0xd0020867,
	0x20027031, 0x100049e1,
	0x019e7040, 0x10020827,
	0x15800dc0, 0xd0020867,
	0x20067031, 0x100049e1,
	0x019e7040, 0x10020827,
	0x15800dc0, 0xd0020867,
	0x200a7031, 0x100049e1,
	0x019e7040, 0x10020827,
	0x00000000, 0xf0f7e9e7,
	0x15800dc0, 0xd0020867,
	0x200e7031, 0x100049e1,
	0x019e7040, 0x10020827,
};

// Coordinate Shader
#if 0
# Take m0, and multply x0.
# Then take m1, and multply y0, and add the result
# Then take m2, and multply z0, and add the result
# Then take m3, and multply w0, and add the result
# This result is the xc for all the vertices.
# Similarly, calculate yc, zc and wc
# Perform the perspective divide to get the NDC.
# Perform the viewport transform to get xs, ys and zs.
# Fill in the details.
# In the vertex shader, copy paste the colour.

li	vpr_setup, -, 0x401a00;	# 4 horizontal rows.
;;;	# 3 instruction delay
ori	a0, vpr, 0;	# xo
ori	a1, vpr, 0;	# yo
ori	a2, vpr, 0;	# zo
ori	a3, vpr, 0;	# wo

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

# Convert xs and ys into 12.4 fixed-point.
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

li	vpw_setup, -, 0x1a00;
or	vpw, a4, a4;
or	vpw, a5, a5;
or	vpw, a6, a6;
or	vpw, a7, a7;
or	vpw, a0, a0;
or	vpw, a2, a2;
or	vpw, a3, a3;

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




// Test shader.
#if 0
# Write back these values into RAM, for verification.
;;

# VPM loads the vertex data at Y=0x40, since we have starting 4K bytes of VPM
# reserved for UP. Even then, one can still read/write the data using Y=0 in
# CS. The read actually occurs from Y=40, and write actuals occurs at Y=44
# (if 1-4 vertex attributes), Y=48 (if 5-8 vertex attributes), etc., but
# with 4-columns swapped.
li	vpr_setup, -, 0x401a00;
;;;
ori	r0, vpr, 0;
ori	r1, vpr, 0;
ori	r2, vpr, 0;
ori	r3, vpr, 0;

fmuli	r0, r0, 2f;
fmuli	r1, r1, 2f;
fmuli	r2, r2, 2f;
fmuli	r3, r3, 2f;

# Data loaded at 0x40
# 1a44 writes at 0x48
# 1a48 writes at 0x48
# 1a00 writes at 0x44
# These writes swap columns in CS, but not in UP. Perhaps related to QPU
# scheduling.
li	vpw_setup, -, 0x1a00;
or	vpw, r0, r0;
or	vpw, r1, r1;
or	vpw, r2, r2;
or	vpw, r3, r3;

# Y address 13-7, X address 6-3
#li	vdw_setup, -, 0x82105c00;
#li	vdw_setup, -, 0x84106000;
li	vdw_setup, -, 0x80104000;
ori	vdw_addr, uni_rd, 0;
or	-, vdw_wait, r0;

usb;
ori	host_int, 1, 1;
pe;;;
#endif








// Coordinate Shader. Matrix in a register instead of in uniform.
#if 0
# m0 m1 m2 m3 m4 m5 m6 m7 m8 m9 ma mb mc md me mf
#  1
# Duplicate the element under the mask
# m0 m0 m0 m0 m0 m0 m0 m0 m0 m0 m0 m0 m0 m0 m0 m0
# Multiply the xo coordinates for all vertices.
# Then take m1, and multply y0, and add the result
# Then take m2, and multply z0, and add the result
# Then take m3, and multply w0, and add the result
# This result is the xc for all the vertices.
# Similarly, calculate yc, zc and wc
# Perform the perspective divide to get the NDC.
# Perform the viewport transform to get xs, ys and zs.
# Fill in the details.
# In the vertex shader, copy paste the colour.

li	vpr_setup, -, 0x401a00;	# 4 horizontal rows.
;;;	# 3 instruction delay
ori	a0, vpr, 0;	# xo
ori	a1, vpr, 0;	# yo
ori	a2, vpr, 0;	# zo
ori	a3, vpr, 0;	# wo
ori	a4, uni_rd, 0;	# Projection Matrix

#############################################################
li	a5, -, 0x10001;	# Mask 0xffffffff 0 .... 0
or	r0, a5, a5;	# Delay slot needed if not for this instruction.
and	r0, r0, a4;
bl	a31, func_duplicate;;;;
fmul	a6, a0, r0;	# m0*xo

li	r1, -, 0;	# Rotate the mask.
or	r0, a5, a5;
v8asrot1	a5, r0, r1;
or	r0, a5, a5;
and	r0, r0, a4;
bl	a31, func_duplicate;;;;
fmul	a7, a1, r0;	# m1*yo

li	r1, -, 0;	# Rotate the mask.
or	r0, a5, a5;
v8asrot1	a5, r0, r1;
or	r0, a5, a5;
and	r0, r0, a4;
bl	a31, func_duplicate;;;;
fmul	a8, a2, r0;	# m2*zo

li	r1, -, 0;	# Rotate the mask.
or	r0, a5, a5;
v8asrot1	a5, r0, r1;
or	r0, a5, a5;
and	r0, r0, a4;
bl	a31, func_duplicate;;;;
fmul	a9, a3, r0;	# m3*wo

or	r0, a6, a6;
fadd	r0, r0, a7;
fadd	r0, r0, a8;
fadd	a10, r0, a9;	# a10 has the xc for the given vertices.


#############################################################
li	r1, -, 0;	# Rotate the mask.
or	r0, a5, a5;
v8asrot1	a5, r0, r1;
or	r0, a5, a5;
and	r0, r0, a4;
bl	a31, func_duplicate;;;;
fmul	a6, a0, r0;	# m4*xo

li	r1, -, 0;	# Rotate the mask.
or	r0, a5, a5;
v8asrot1	a5, r0, r1;
or	r0, a5, a5;
and	r0, r0, a4;
bl	a31, func_duplicate;;;;
fmul	a7, a1, r0;	# m5*yo

li	r1, -, 0;	# Rotate the mask.
or	r0, a5, a5;
v8asrot1	a5, r0, r1;
or	r0, a5, a5;
and	r0, r0, a4;
bl	a31, func_duplicate;;;;
fmul	a8, a2, r0;	# m6*zo

li	r1, -, 0;	# Rotate the mask.
or	r0, a5, a5;
v8asrot1	a5, r0, r1;
or	r0, a5, a5;
and	r0, r0, a4;
bl	a31, func_duplicate;;;;
fmul	a9, a3, r0;	# m7*wo

or	r0, a6, a6;
fadd	r0, r0, a7;
fadd	r0, r0, a8;
fadd	a11, r0, a9;	# a11 has the yc for the given vertices.


#############################################################
li	r1, -, 0;	# Rotate the mask.
or	r0, a5, a5;
v8asrot1	a5, r0, r1;
or	r0, a5, a5;
and	r0, r0, a4;
bl	a31, func_duplicate;;;;
fmul	a6, a0, r0;	# m8*xo

li	r1, -, 0;	# Rotate the mask.
or	r0, a5, a5;
v8asrot1	a5, r0, r1;
or	r0, a5, a5;
and	r0, r0, a4;
bl	a31, func_duplicate;;;;
fmul	a7, a1, r0;	# m9*yo

li	r1, -, 0;	# Rotate the mask.
or	r0, a5, a5;
v8asrot1	a5, r0, r1;
or	r0, a5, a5;
and	r0, r0, a4;
bl	a31, func_duplicate;;;;
fmul	a8, a2, r0;	# ma*zo

li	r1, -, 0;	# Rotate the mask.
or	r0, a5, a5;
v8asrot1	a5, r0, r1;
or	r0, a5, a5;
and	r0, r0, a4;
bl	a31, func_duplicate;;;;
fmul	a9, a3, r0;	# mb*wo

or	r0, a6, a6;
fadd	r0, r0, a7;
fadd	r0, r0, a8;
fadd	a12, r0, a9;	# a12 has the zc for the given vertices.


#############################################################
li	r1, -, 0;	# Rotate the mask.
or	r0, a5, a5;
v8asrot1	a5, r0, r1;
or	r0, a5, a5;
and	r0, r0, a4;
bl	a31, func_duplicate;;;;
fmul	a6, a0, r0;	# mc*xo

li	r1, -, 0;	# Rotate the mask.
or	r0, a5, a5;
v8asrot1	a5, r0, r1;
or	r0, a5, a5;
and	r0, r0, a4;
bl	a31, func_duplicate;;;;
fmul	a7, a1, r0;	# md*yo

li	r1, -, 0;	# Rotate the mask.
or	r0, a5, a5;
v8asrot1	a5, r0, r1;
or	r0, a5, a5;
and	r0, r0, a4;
bl	a31, func_duplicate;;;;
fmul	a8, a2, r0;	# me*zo

li	r1, -, 0;	# Rotate the mask.
or	r0, a5, a5;
v8asrot1	a5, r0, r1;
or	r0, a5, a5;
and	r0, r0, a4;
bl	a31, func_duplicate;;;;
fmul	a9, a3, r0;	# mf*wo

or	r0, a6, a6;
fadd	r0, r0, a7;
fadd	r0, r0, a8;
fadd	a13, r0, a9;	# a13 has the wc for the given vertices.


#############################################################
ori	sfu_recip, a13, a13; # wcr
;;	# Delay slot for SFU

fmul	a15, a10, r4;	# xc -> xndc
fmul	a16, a11, r4;	# yc -> yndc
fmul	a17, a12, r4;	# zc -> zndc

or	a14, r4, r4;

# Invert the sign of yndc
fsubi	a16, 0, a16;

#############################################################
# Perform ViewPort Transform. Hardcoded for 640x480 for now.

# xs = 319.5 * xndc;
# ys = 239.5 * yndc.
# zs = 0.5 * zndc + 0.5.

li	r0, -, 319;
itof	r0, r0, r0;
faddi	r0, r0, i2f;
fmul	a15, a15, r0;	# xndc -> xs

li	r0, -, 239;
itof	r0, r0, r0;
faddi	r0, r0, i2f;
fmul	a16, a16, r0;	# yndc -> ys

faddi	a17, a17, 1f;
fmuli	a17, a17, i2f;	# zndc -> zs

# Convert xs and ys into 12.4 fixed-point. For now, with zero
# fractional component.
ftoi	a15, a15, a15;
ftoi	a16, a16, a16;
shli	a15, a15, 4;
shli	a16, a16, 4;

# Place them within the same 32-bit word
shli	a16, a16, 8;
shli	a16, a16, 8;
or	r0, a15, a15;
or	r0, r0, a16;
or	a15, r0, r0;


#############################################################
# a10 xc
# a11 yc
# a12 zc
# a13 wc
# a14 wcr
# a15 ys | xs
# a17 zs

li	vpw_setup, -, 0x1a00;
or	vpw, a10, a10;
or	vpw, a11, a11;
or	vpw, a12, a12;
or	vpw, a13, a13;
or	vpw, a15, a15;
or	vpw, a17, a17;
or	vpw, a14, a14;

ori	host_int, 1, 1;
pe;;;













# r0 = input with a single element selected.
func_duplicate:
li	r3, -, 0;	# Initialize output.
li	r1, -, 0;	# Initialize the counter.
loop_func_duplicate:
subi	r2, r1, 8;
subi	r2, r2, 8	sf;	# Done yet?
b.z	done_func_duplicate;
;;;
or	r3, r0, r0;
b	loop_func_duplicate;
li	r2, -, 0;
v8asrot1	r0, r0, r2;
addi	r1, r1, 1;	# Increment the counter.

done_func_duplicate:
b	a31;
or	r0, r3, r3;	# Return value in r0
;;
#endif
