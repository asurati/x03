// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

.macro		def_handler, entry
// .global		\entry
.align		2
.type		\entry, %function
\entry:
	b	.
.size		\entry, . - \entry
.endm

.section	.text, "ax", %progbits
// VBAR LS 5bits SBZ.
.align		5
.global		excptn_vector
.type		excptn_vector, %function
excptn_vector:
	ldr	pc, =.Lexcptn_rst
	ldr	pc, =.Lexcptn_ui
	ldr	pc, =.Lexcptn_swi
	ldr	pc, =.Lexcptn_ia
	ldr	pc, =.Lexcptn_da
	b	.
	ldr	pc, =.Lexcptn_irq
	b	.
.size		excptn_vector, . - excptn_vector

def_handler	.Lexcptn_rst
def_handler	.Lexcptn_ui
def_handler	.Lexcptn_swi
def_handler	.Lexcptn_ia
def_handler	.Lexcptn_da

.align		2
.type		.Lexcptn_irq, %function
.Lexcptn_irq:
	sub	lr, lr, #4	// Adjust the return address.

	srsdb	sp!, #0x13	// Save spsr_irq and lr_irq onto SVC stack.
	cps	#0x13		// Switch to SVC mode
	push	{r0-r3, r12, lr}	// Save regs
	bl	cpu_hw_irq_handler
	pop	{r0-r3, r12, lr}
	rfeia	sp!			// Undo srsdb
.size		.Lexcptn_irq, . - .Lexcptn_irq
