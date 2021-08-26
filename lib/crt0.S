// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

.section	.text, "ax", %progbits
.global		_start
.align		2
_start:
	// Select the stack.
	ldr	r0, =.Lstack
	mov	sp, r0

	bl	cpu_init
	bl	thread_idle_thread
.Lsink:
	wfi
	b	.Lsink
.size		_start, . - _start

// Idle thread stack.
.section	.bss, "aw", %nobits
.align		4
.size		.Lstack, .Lstack - .
.fill		0x10000, 1, 0
.type		.Lstack, %object
.Lstack:
