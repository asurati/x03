// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

.section	.text, "ax", %progbits
.align		5
.global		excptn_vector
.type		excptn_vector, %function
excptn_vector:
	ldr	pc, .Lexcptn_dummy_addr
	ldr	pc, .Lexcptn_dummy_addr
	ldr	pc, .Lexcptn_dummy_addr
	ldr	pc, .Lexcptn_dummy_addr
	ldr	pc, .Lexcptn_dummy_addr
	ldr	pc, .Lexcptn_dummy_addr
	ldr	pc, .Lexcptn_irq_addr
	ldr	pc, .Lexcptn_dummy_addr
.Lexcptn_dummy_addr:
	.word	cpu_irq_handler_dummy
.Lexcptn_irq_addr:
	.word	.Lexcptn_irq
.size		excptn_vector, . - excptn_vector


// Keep sp_irq 8-byte aligned.
.align		2
.type		.Lexcptn_irq, %function
.Lexcptn_irq:
	sub	lr, lr, #4		// Adjust the return address.

	srsdb	sp!, #19		// Save spsr_irq and lr_irq
					// into the SVC stack
	cps	#19			// Switch to SVC mode
	push	{r0-r3, r12, lr}	// Save aapcs regs + lr_svc

	bl	cpu_irq_handler

	pop	{r0-r3, r12, lr}
	rfeia	sp!			// Undo srsdb
.size		.Lexcptn_irq, . - .Lexcptn_irq
