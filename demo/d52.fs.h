// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <stdint.h>

static const uint32_t fs_code[] __attribute__((aligned(8))) = {
	0x203e303e, 0x100049e0, // fmul	r0, vary_rd, a15;
	0x019e7140, 0x10020827, // fadd	r0, r0, r5;
	0x203e303e, 0x100049e1, // fmul	r1, vary_rd, a15;
	0x019e7340, 0x10020867, // fadd	r1, r1, r5;
	0x203e303e, 0x100049e2, // fmul	r2, vary_rd, a15;
	0x019e7540, 0x100208a7, // fadd	r2, r2, r5;
	0x000000ff, 0xe00208e7, // li	r3, -, 0xff;
	0x209e0007, 0xd16049e0, // fmuli	r0, r0, 1f	pm8c;
	0x209e000f, 0xd15049e0, // fmuli	r0, r1, 1f	pm8b;
	0x209e0017, 0xd14049e0, // fmuli	r0, r2, 1f	pm8a;
	0x209e001f, 0xd17049e0, // fmuli	r0, r3, 1f	pm8d;
	0x159e7000, 0x50020ba7, // or	tlb_clr_all, r0, r0	usb;
	0x159c1fc0, 0xd00209a7, // ori	host_int, 1, 1;
	0x009e7000, 0x300009e7, // pe;
	0x009e7000, 0x100009e7, // ;
	0x009e7000, 0x100009e7, // ;
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

li	r3, -, 0xff;	# alpha

# Utilize MUL-pack facility to convert colour components from float to
# byte with saturation, and place them at appropriate locations depending on
# the framebuffer format. The format is 0x8d8c8b8a, corresponding to
# 0xaarrggbb.
fmuli	r0, r0, 1f	pm8c;
fmuli	r0, r1, 1f	pm8b;
fmuli	r0, r2, 1f	pm8a;
fmuli	r0, r3, 1f	pm8d;

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
