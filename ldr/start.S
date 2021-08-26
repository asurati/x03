// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

.section	.text.startup, "ax", %progbits
.global		_start
.align		2
.type		_start, %function

_start:
	b	.Lstart
.size		_start, . - _start

.section	.text, "ax", %progbits
.align		2
.type		.Lstart, %function
.Lstart:
	// Disable all interrupts, and switch to SVC mode
	cpsid	aif, #0x13

	ldr	r0, =.Lstack
	mov	sp, r0

	// Invalidate caches and TLBs.
	bl	invalidate

	// Set cache sizes to 16KB to avoid page colouring
	bl	set_cache_size

	// Load the sys.elf binary. Return its entry point.
	bl	load_sys
	mov	r5, r0	// Save

	bl	mmu_setup
	bl	mmu_enable

	bx	r5
.Lsink:
	wfi
	b	.Lsink
.size		.Lstart, . - .Lstart

// Allocate stack inside .data, as we want it to be resident inside the raw
// binary.
.section	.data, "aw", %progbits
.align		3
.size		.Lstack, .Lstack - .
.fill		0x800, 1, 0
.type		.Lstack, %object
.Lstack:
