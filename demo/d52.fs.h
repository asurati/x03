// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <stdint.h>

static const uint32_t fs_code[] __attribute__((aligned(8))) = {
	0x203e303e, 0x100049e0,
	0x019e7140, 0x10020827,
	0x203e303e, 0x100049e1,
	0x019e7340, 0x10020867,
	0x203e303e, 0x100049e2,
	0x019e7540, 0x100208a7,
	0x000000ff, 0xe00208e7,
	0x089e76c0, 0x100208e7,
	0x209e7003, 0x100049e0,
	0x209e700b, 0x100049e1,
	0x209e7013, 0x100049e2,
	0x079e7000, 0x10020827,
	0x079e7240, 0x10020867,
	0x079e7480, 0x100208a7,
	0x119c81c0, 0xd0020827,
	0x119c81c0, 0xd0020827,
	0x119c83c0, 0xd0020867,
	0x159e7040, 0x10020827,
	0x159e7080, 0x10020827,
	0xff000000, 0xe0020867,
	0x159e7040, 0x10020827,
	0x159e7000, 0x50020ba7,
	0x159c1fc0, 0xd00209a7,
	0x009e7000, 0x300009e7,
	0x009e7000, 0x100009e7,
	0x009e7000, 0x100009e7,
};
// The framebuffer format is BGRA8888, or 0xaarrggbb, or ARGB32.

#if 0
# RGB
fmul	r0, vary_rd, a15;	# a15 has W.
fadd	r0, r0, r5;

fmul	r1, vary_rd, a15;
fadd	r1, r1, r5;

fmul	r2, vary_rd, a15;
fadd	r2, r2, r5;

# TODO utilize mul output pack facility to convert floating point output into
# a 8-bit colour in the range [0, 255].
# x 255.0
li	r3, -, 255;
itof	r3, r3, r3;
fmul	r0, r0, r3;
fmul	r1, r1, r3;
fmul	r2, r2, r3;

# To integer colour
ftoi	r0, r0, r0;
ftoi	r1, r1, r1;
ftoi	r2, r2, r2;

# 0xaarrggbb
shli	r0, r0, 8;	# Red shifted by 16
shli	r0, r0, 8;
shli	r1, r1, 8;	# Green shifted by 8

or	r0, r0, r1;
or	r0, r0, r2;

li	r1, -, 0xff000000;
or	r0, r0, r1;

or	tlb_clr_all, r0, r0	usb;

ori	host_int, 1, 1;
pe;;;
#endif


// A Red Triangle Fragment Shader.
#if 0
# Wait for Scoreboard
;;	# WSB cannot occur in the first two instructions of FS.
wsb;
li	tlb_clr_all, -, 0xffff0000;
usb;
ori	host_int, 1, 1;
pe;;;
#endif
