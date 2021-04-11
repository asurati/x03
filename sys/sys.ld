// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <sys/mmu.h>

ENTRY(_start)

PHDRS
{
	all	PT_LOAD PHDRS FILEHDR FLAGS(7);	// rwx
}

SECTIONS
{
	. = VA_BASE + SIZEOF_HEADERS;
	.text : {
		*(.text.startup);
		*(.text);
	} :all

	. = ALIGN(8);
	.rodata : {
		*(.rodata);
		*(.rodata.*);
	}

	. = ALIGN(8);
	.data : {
		*(.data);
	}

	. = ALIGN(8);
	.bss : {
		_bss_start = .;
		*(.bss);
		_bss_end = .;
	}
	_krnl_end = .;
}
