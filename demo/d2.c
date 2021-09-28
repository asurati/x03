// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdlib.h>

#include <sys/cpu.h>

#include <dev/v3d.h>
#include <dev/tmr.h>
#include <dev/con.h>

// v0.0 (y=0,x=15)
// v0.1
// ...
// v0.15
//
// v1.0 (y=16,x=15)
// v1.1
// ...
// v1.15
//
// v2.0 (y=32,x=15)
// v2.1
// .
// v2.15
//
// v3.0 (y=48,x=15)
// v3.1
// .
// v3.15
//
// Total 64 words.

// Although VDR in vertical mode allows us to load 4 vectors in a single
// column (X=15 here), the behaviour of VDW in vertical mode is different.
// When asked to write 4 vectors starting at Y=0,X=15, it wrote vectors at
// Y=16,X=15, Y=16,X=0, Y=16,X=1 and Y=16,X=2. It moved horizontally, and when
// wrapped around X=15 to X=0, it incremented Y by 16.
//
// VDR has VPITCH which adds to the Y address, and does not touch the X
// address (at least in the 32-bit width mode). But VDW (in vertical mode)
// adds to the X address, and if the resultant X wraps around, only then adds
// 16 to the Y address.
//
// VDW thus only moves in left-to-right, then down-to-up,
//
// https://github.com/maazl/vc4asm/issues/27
// So if you choose vertical mode the elements in a memory row are vertically
// aligned in VPM. The next row in memory will be in the next column of VPM.

// The sample program that demonstrates the above analysis:

#if 0
li	vdr_setup, -, 0x8304080f;
ori	vdr_addr, uni_rd, 0;
or	-, vdr_wait, r0;

li	vdw_setup, -, 0x82100078;
ori	vdw_addr, uni_rd, 0;
or	-, vdw_wait, r0;

# Write VPM into RAM, for verification.
li	vdw_setup, -, 0x80104000;
ori	vdw_addr, uni_rd, 0;
or	-, vdw_wait, r0;
#endif

int d2_run()
{
	int err, i;
	pa_t code_ba, unif_ba;
	static uint32_t in_buf[64];
	static uint32_t unif[3];
	static uint32_t vpm[128][16];

	// Allocate out_buf such that it is aligned at the cache-line
	// boundary, so that it does not share cache-lines with any other
	// variable. This is needed because we must invalidate the out_buf
	// before reading from the memory, but if there are variables which
	// share the cache-line with out_buf, we may lose their most recent
	// values because of the invalidate.
	static uint32_t out_buf[64] __attribute__((aligned(CACHE_LINE_SIZE)));

	static const uint32_t code[] __attribute__((aligned(8))) = {
		0x8304080f, 0xe0020c67,
		0x15800dc0, 0xd0020ca7,
		0x15ca7c00, 0x100209e7,
		0x80900078, 0xe0020827,
		0x15800dc0, 0xd0020867,
		0x00000000, 0xe00208a7,
		0x0d9c45c0, 0xd00228e7,
		0x00000048, 0xf02809e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
		0x159e7000, 0x10021c67,
		0x159e7240, 0x10021ca7,
		0x159f2e00, 0x100209e7,
		0x00000800, 0xe00208e7,
		0x0c9e70c0, 0x10020827,
		0xffffff90, 0xf0f809e7,
		0x00000040, 0xe00208e7,
		0x0c9e72c0, 0x10020867,
		0x0c9c15c0, 0xd00208a7,
		0x80104000, 0xe0021c67,
		0x15800dc0, 0xd0021ca7,
		0x159f2e00, 0x100209e7,
		0x159c1fc0, 0xd00209a7,
		0x009e7000, 0x300009e7,
		0x009e7000, 0x100009e7,
		0x009e7000, 0x100009e7,
	};

	srand(tmr_get_ctr());

	for (i = 0; i < 64; ++i)
		in_buf[i] = rand();

	unif[0] = va_to_ba((va_t)in_buf);
	unif[1] = va_to_ba((va_t)out_buf);
	unif[2] = va_to_ba((va_t)vpm);

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
		con_out("d2: [%x]: %x, %x", i, in_buf[i], out_buf[i]);

	// Remove this return to dump the VPM.
	return err;

	dc_ivac(vpm, sizeof(vpm));
	dsb();
	for (i = 0; i < 128; ++i)
		con_out("[%x] %x %x %x %x, %x %x %x %x, %x %x %x %x, "
			"%x %x %x %x", i,
			vpm[i][0], vpm[i][1], vpm[i][2], vpm[i][3],
			vpm[i][4], vpm[i][5], vpm[i][6], vpm[i][7],
			vpm[i][8], vpm[i][9], vpm[i][10], vpm[i][11],
			vpm[i][12], vpm[i][13], vpm[i][14], vpm[i][15]);
	return err;
}

#if 0
li	vdr_setup, -, 0x8304080f;
ori	vdr_addr, uni_rd, 0;
or	-, vdr_wait, r0;

# r0 has the setup descriptor.
# r1 has the output address.
# r2 is the counter

li	r0, -, 0x80900078;
ori	r1, uni_rd, 0;
li	r2, -, 0;

loop:
subi	r3, r2, 4	sf;
b.z	done;
;;;
or	vdw_setup, r0, r0;
or	vdw_addr, r1, r1;
or	-, vdw_wait, r0;

li	r3, -, 0x800;
add	r0, r0, r3;	# Add 16 to Y coordinate: bits [13:7].
b	loop;
li	r3, -, 0x40;
add	r1, r1, r3;	# Add 64 to the output address.
addi	r2, r2, 1;	# Increment the counter.

done:
# Write VPM into RAM, for verification.
li	vdw_setup, -, 0x80104000;
ori	vdw_addr, uni_rd, 0;
or	-, vdw_wait, r0;

ori	host_int, 1, 1;
pe;;;
#endif
