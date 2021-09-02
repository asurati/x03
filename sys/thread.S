// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

// r0 = curr thread, r1 = next thread
.section	.text, "ax", %progbits
.global		thread_switch
.align		2
.type		thread_switch, %function
thread_switch:
	// Save the stack pointer into the curr thread structure.
	str	sp, [r0], #4

	// Save the LR into the curr thread structure.
	str	lr, [r0], #4

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
	ldr	sp, [r0], #4
	ldr	lr, [r0], #4

	// Restore registers.
	ldr	r4, [r0], #4
	ldr	r5, [r0], #4
	ldr	r6, [r0], #4
	ldr	r7, [r0], #4
	ldr	r8, [r0], #4
	ldr	r10, [r0], #4
	ldr	r11, [r0], #4

	bx	lr
.size		thread_switch, . - thread_switch

.global		thread_enter
.type		thread_enter, %function
thread_enter:
	mov	lr, r4
	mov	r0, r5
	bx	lr
.Lsink:
	wfi
	b	.Lsink
.size		thread_enter, . - thread_enter
