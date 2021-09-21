// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <sys/mmu.S.h>		// For ASM_LDR_BASE
ENTRY(_start)

SECTIONS
{
	. = ASM_LDR_BASE;
	.text : {
		*(.text.startup);
		*(.text);
		. = ALIGN(16);
	}
	.got : {
		*(.got);
		*(.toc);
	}
	.data : {
		*(.data);
		. = ALIGN(16);
	}
	/DISCARD/ : {
		*(.bss);
	}
	_end = .;
}
