// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef SYS_ELF_H
#define SYS_ELF_H

#include <stdint.h>

struct elf32_hdr {
	unsigned char			ident[16];
	uint16_t			type;
	uint16_t			machine;
	uint32_t			version;
	uint32_t			entry;
	uint32_t			phoff;
	uint32_t			shoff;
	uint32_t			flags;
	uint16_t			ehsize;
	uint16_t			phentsize;
	uint16_t			phnum;
	uint16_t			shentsize;
	uint16_t			shnum;
	uint16_t			shstrndx;
};

struct elf32_phdr {
	uint32_t			type;
	uint32_t			offset;
	uint32_t			vaddr;
	uint32_t			paddr;
	uint32_t			filesz;
	uint32_t			memsz;
	uint32_t			flags;
	uint32_t			align;
};

#define PT_LOAD				1
#define PF_X				(1 << 0)
#define PF_W				(1 << 1)
#define PF_R				(1 << 2)
#endif
