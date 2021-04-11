// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

.align		2
.global		thread_get_current
.type		thread_get_current, %function
thread_get_current:
	mov	r0, sp
	bic	r0, r0, #0xff0
	bic	r0, r0, #0xf
	add	r0, r0, #0x1000	// stack size 4KB.
	sub	r0, r0, #0x18	// sizeof(struct thread)
	bx	lr
.size		thread_get_current, . - thread_get_current

.align		2
.global		_thread_switch
.type		_thread_switch, %function

// r0 = &curr->context, r1 = &next->context.
_thread_switch:
	push	{r0-r12, lr}
	mov	r2, #0	// padding.
	mrs	r3, cpsr
	push	{r2, r3}
	str	sp, [r0]

	ldr	sp, [r1]
	pop	{r2, r3}
	msr	cpsr, r3
	pop	{r0-r12, pc}
.size		_thread_switch, . - _thread_switch
