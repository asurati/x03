// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

// r0 = curr thread, r1 = next thread
.section	.text, "ax", %progbits
.global		thread_switch
.align		2
.type		thread_switch, %function
thread_switch:
	// Save the stack pointer into the curr thread structure.
	mov	r2, sp
	str	r2, [r0], #4

	// Save into the curr thread structure
	str	r4, [r0], #4
	str	r5, [r0], #4
	str	r6, [r0], #4
	str	r7, [r0], #4
	str	r8, [r0], #4
	str	r10, [r0], #4
	str	r11, [r0], #4

	// Switch to the next thread.
	mov	r0, r1	// Also the return value.
	ldr	r2, [r0], #4
	mov	sp, r2

	// Restore registers.
	ldr	r4, [r0], #4
	ldr	r5, [r0], #4
	ldr	r6, [r0], #4
	ldr	r7, [r0], #4
	ldr	r8, [r0], #4
	ldr	r10, [r0], #4
	ldr	r11, [r0], #4

	// Assuming that thread_switch is called from a single location.
	// So the LR register doesn't need to be preserved.
	bx	lr
.size		thread_switch, . - thread_switch
