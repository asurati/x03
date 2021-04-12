// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/stdio.h>
#include <lib/stdlib.h>

#include <sys/cpu.h>
#include <sys/err.h>
#include <sys/mmu.h>

#include <dev/uart.h>
#include <dev/v3d.h>

#define TIMEOUT_ITERS			1000

// Demo #2.

// For this demo, have the QPU copy paste a buffer via Vertex Pipe Memory
// (VPM).

// The input and output vectors are setup in the RAM.

// The QPU programs the Vertex Cache DMA (VCD) to load the input vector from
// the RAM into the VPM.

// Then it programs the VPM DMA Writer (VDW) to copy the vector back into the
// RAM at the given output location.

int d2()
{
	int i, prev_num_done, curr_num_done, err;
	unsigned int val;
	static char buf[512];
	pa_t pa;

	// No special alignment requirement for the in/out bufs.
	static unsigned int in_buf[16] = {0};
	static unsigned int out_buf[16] = {0};
	static pa_t uniforms[2] = {0};

	static const unsigned int code[] __attribute__((aligned(8))) = {
		0x83010800, 0xe0020c40,
		0x15800dc0, 0xd0020c80,
		0x15c80180, 0x100209c0,
		0x80900000, 0xe0021c40,
		0x15800dc0, 0xd0021c80,
		0x150321c0, 0x100209c0,
		0x15001fc0, 0xd0020980,
		0x0, 0x30000000,
		0x0, 0x10000000,
		0x0, 0x10000000,
	};

	// Reserve VPM for user programs.
	writel(V3D_VPMBASE, 16);	// 4 KB.

	// Read the num of user programs completed.
	val = readl(V3D_SRQCS);
	prev_num_done = bits_get(val, V3D_SRQCS_NUM_DONE);

	for (i = 0; i < 16; ++i)
		in_buf[i] = rand();

#if 0
	// 0: VDR_SETUP.

	// The input (and also the output) buffer in memory is stored as
	// 16 consecutively stored 32-bit integers. Since it is natural for
	// the QPU to work with 16-element vectors, the input can be thought of
	// as a 2D array of dimension 16x1 (16 wide and 1 high) - a single row
	// of 16 32-bit integers.

	// The size of the row is thus 64 bytes. The MPITCH value corresponding
	// to that row-size is 3, since the row-to-row pitch is 64 = 8*2^3.
	// Note that there isn't a second row, so the MPITCH value isn't
	// relevant to this demo, but for the sake of correctness, we set it
	// to represent the actual row-size of 64 bytes.

	// The WIDTH is set to 32-bit (=0) in MODEW.

	// The ROWLEN, in terms of WIDTH units, is 16(=0).

	// NROWS is 1.

	// Within VPM, we store the vector in a vertical orientation.
	// The row-to-row pitch, VPITCH, becomes 16(=0), since 16 should be
	// added to Y to move to the next vector in VPM. As with MPITCH, this
	// value isn't relevant as there's isn't another row to deal with.

	val = 0;
	val |= bits_set(VDR_ID, VDR_ID_VAL);
	val |= bits_set(VDR_MPITCH, 3);
	val |= bits_set(VDR_NROWS, 1);
	val |= bits_on(VDR_VERT);

	//
	// 2: VDW_SETUP.

	// The output is laid out in memory as a single row of 16 32-bit
	// integers.

	// The WIDTH is set to 32-bit (=0) in MODEW.
	// The UNITS is 1 row.
	// The DEPTH is 16, in units of width.

	val = 0;
	val |= bits_set(VDW_ID, VDW_ID_VAL);
	val |= bits_set(VDW_UNITS, 1);
	val |= bits_set(VDW_DEPTH, 16);
#endif

	uniforms[0] = pa_to_bus((pa_t)in_buf);
	uniforms[1] = pa_to_bus((pa_t)out_buf);

	dc_cvac(in_buf, sizeof(in_buf));
	dc_cvac(uniforms, sizeof(uniforms));
	dsb();

	writel(V3D_SRQUA, (pa_t)uniforms);
	writel(V3D_SRQUL, sizeof(uniforms));

	// Run code.
	pa = pa_to_bus((pa_t)code);
	writel(V3D_SRQPC, pa);

	for (i = 0; i < TIMEOUT_ITERS; ++i) {
		val = readl(V3D_SRQCS);
		curr_num_done = bits_get(val, V3D_SRQCS_NUM_DONE);
		if (curr_num_done > prev_num_done) {
			uart_send_string("d2: QPU done.\r\n");
			break;
		}
	}

	err = ERR_SUCCESS;
	if (i == TIMEOUT_ITERS) {
		uart_send_string("d2: Timed out waiting for QPU.\r\n");
		err = ERR_TIMEOUT;
		goto done;
	}

	// Invalidate the DCache entries for the output buffer, so that
	// the Arm's L1 cache doesn't contain any stale copies.

	dc_ivac(out_buf, sizeof(out_buf));
	dsb();

	for (i = 0; i < 16; ++i) {
		snprintf(buf, sizeof(buf), "d2: [%x] = %x %x\r\n", i,
			 in_buf[i], *(volatile unsigned int *)out_buf[i]);
		uart_send_string(buf);
	}

done:
	// Revoke the VPM reservation for user programs.
	writel(V3D_VPMBASE, 0);
	return err;
}

#if 0
li,vpmrds,-,0x83010800;
or,ldaddr,unif,0	-;
or,-,-,ldwait		-;

li,vpmwrs,-,0x80900000;
or,staddr,unif,0	-;
or,-,-,stwait		-;

or,hostint,1,1		-;
- - endp;
- -;
- -;
#endif
