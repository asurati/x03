// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

.macro		barriers
	mcr	p15, 0,	r0, c7, c10, 4	// DSB
	mcr	p15, 0,	r0, c7, c5, 4	// ISB
	mcr	p15, 0,	r0, c7, c5, 6
.endm

.section	.text, "ax", %progbits
.global		mmu_real_switch
.align		2
.type		mmu_real_switch, %function
// r0 = ttbr0 val
mmu_real_switch:
	mov	r3, r0

	mrc	p15, 0, r0, c1, c0, 0	// Read the control register
	bic	r0, r0, #0x1000	// Clear M, C, I
	bic	r0, r0, #5
	mcr	p15, 0, r0, c1, c0, 0

	mov	r0, #0

	// Clean and Invalidate DC
	mcr	p15, 0, r0, c7, c14, 0
	barriers

	// Invalidate IC
	mcr	p15, 0, r0, c7, c5, 0
	barriers

	// Invalidate TLBs
	mcr	p15, 0, r0, c8, c5, 0
	mcr	p15, 0, r0, c8, c6, 0
	barriers

	// Setup TTBR0
	mcr	p15, 0, r3, c2, c0, 0

	// Enable M,C,I
	mrc	p15, 0, r0, c1, c0, 0	// Read the control register
	orr	r0, r0, #0x1000	// Set M, C, I
	orr	r0, r0, #5
	mcr	p15, 0, r0, c1, c0, 0

	bx	lr
.size		mmu_real_switch, . - mmu_real_switch
