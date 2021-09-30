// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <stdint.h>

// Same as d52.fs.h, except for the write to TLB_Z.
static const uint32_t fs_code[] __attribute__((aligned(8))) = {
	0x203e303e, 0x100049e0, // fmul	r0, vary_rd, a15;
	0x019e7140, 0x10020827, // fadd	r0, r0, r5;

	0x203e303e, 0x100049e1, // fmul	r1, vary_rd, a15;
	0x019e7340, 0x10020867, // fadd	r1, r1, r5;

	0x203e303e, 0x100049e2, // fmul	r2, vary_rd, a15;
	0x019e7540, 0x100208a7, // fadd	r2, r2, r5;

	0x159cffc0, 0x10020b27, // or	tlb_z, b15, b15;

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
