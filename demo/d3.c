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
	pa_t code_ba, unif_ba;
	static uint32_t in_buf[64];
	static uint32_t unif[2];

	// out_buf must be cache-line aligned and its size an integral
	// multiple of the cache-line size. See d2.c.
	static uint32_t out_buf[64] __attribute__((aligned(CACHE_LINE_SIZE)));

	static const uint32_t code[] __attribute__((aligned(8))) = {
		0x8304080f, 0xe0020c67,
		0x15800dc0, 0xd0020ca7,
		0x15ca7c00, 0x100209e7,
		0x0041020f, 0xe0020c67,
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
		0x0001020f, 0xe0021c67,
		0x15027d80, 0x10020c27,
		0x15067d80, 0x10020c27,
		0x150a7d80, 0x10020c27,
		0x150e7d80, 0x10020c27,
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
		con_out("d3: [%x]: %x, %x, %x", i, in_buf[i], out_buf[i],
			in_buf[i] | out_buf[i]);
	return err;
}

#if 0
li	vdr_setup, -, 0x8304080f;
ori	vdr_addr, uni_rd, 0;
or	-, vdr_wait, r0;

li	vpr_setup, -, 0x41020f;
;;;	# 3-instruction delay slot before reading.
ori	a0, vpr, 0;
ori	a1, vpr, 0;
ori	a2, vpr, 0;
ori	a3, vpr, 0;

not	a0, a0, a0;
not	a1, a1, a1;
not	a2, a2, a2;
not	a3, a3, a3;

li	vpw_setup, -, 0x1020f;
or	vpw, a0, a0;
or	vpw, a1, a1;
or	vpw, a2, a2;
or	vpw, a3, a3;

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
#li	vdw_setup, -, 0x80104000;
#ori	vdw_addr, uni_rd, 0;
#or	-, vdw_wait, r0;

ori	host_int, 1, 1;
pe;;;
#endif
