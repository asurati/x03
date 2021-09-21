// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <sys/mmu.S.h>	// For ASM_SYS_BASE, PAGE_SIZE_BITS

ENTRY(_start)

PHDRS
{
	text	PT_LOAD PHDRS FILEHDR FLAGS(5); // r-x
	rodata	PT_LOAD	FLAGS(4);		// r--
	data	PT_LOAD FLAGS(6);		// rw-
}

SECTIONS
{
	. = ASM_SYS_BASE + SIZEOF_HEADERS;

	. = ALIGN(0x8);
	.text : {
		*(.text);
		*(.excptn);
	} :text

	. = ALIGN(1 << PAGE_SIZE_BITS);
	.rodata : {
		*(.rodata);
		*(.rodata.*);
		*(.eh_frame);
	} :rodata

	. = ALIGN(16);	// Move got inside rodata.
	.got : {
		*(.got);
		*(.toc);
	}

	. = ALIGN(1 << PAGE_SIZE_BITS);
	.data : {
		*(.data);
	} :data

	.bss : {
		*(.bss);
	}
	_sys_end = .;
}
