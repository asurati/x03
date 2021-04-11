// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <sys/mmu.h>

// Under qemu, the dtb is at RAM_BASE, since we pass an elf to the -kernel
// option.

// From Peter Maydell:
// "As an oddball exception, for Arm if you pass an ELF file to -kernel,
// we'll assume that it is not a Linux kernel, and will boot it by just
// starting at the ELF entrypoint. We provide the DTB blob at the base of RAM,
// but only if it wouldn't overlap with the loaded ELF file."

.section	.text.startup, "ax", %progbits
.align		2
.global		_start
.type		_start, %function
_start:
	b	start
.size		_start, . - _start

.section	.text, "ax", %progbits
.align		2
.global		start
.type		start, %function
start:
	// Save the dtb pointer passed by uboot.
	mov	r4, r2

	// Disable IRQs, FIQs and enter SVC.
	cpsid	if, #19

	// Invalidate I/D caches.
	mcr	p15, 0, r0, c7, c7, 0

	// Set the size of the caches to 16KB to avoid page colouring.
	mrc	p15, 0, r3, c1, c0, 1
	orr	r3, #(1 << 6)	// AUX_CTRL_CZ
	mcr	p15, 0, r3, c1, c0, 1

	// Enable ICache. This causes the instruction accesses to be cacheable
	// instead of strongly ordered. See "Behavior with MMU disabled".

	mrc	p15, 0, r3, c1, c0, 0
	orr	r3, #(1 << 12)	// CTRL_I
	mcr	p15, 0, r3, c1, c0, 0

	mcr	p15, 0, r0, c7, c10, 4	// dsb.
	mcr	p15, 0, r0, c7, c5, 4	// isb.
	mcr	p15, 0, r0, c7, c5, 6

	// Zero the .bss.
	mov	r0, #0
	ldr	r1, =_bss_start
	ldr	r2, =_bss_end
zero_bss:
	cmp	r1, r2
	beq	zero_bss_done
	strb	r0, [r1]
	add	r1, r1, #1
	b	zero_bss

zero_bss_done:
	// Setup stack. This thread will eventually turn into the idle thread.
	ldr	r0, =g_idle_thread_stack
	add	r0, r0, #0x1000	// stack size 4KB
	sub	r0, r0, #0x18	// sizeof(struct thread)
	mov	sp, r0

	// Call into main to initialize. Pass the dtb pointer.
	mov	r0, r4
	bl	main

	// Call into idle thread routine.
	bl	thread_idle
.Lsink:
	wfi
	b	.Lsink
.size		start, . - start
