// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef DEV_DTB_H
#define DEV_DTB_H

#include <stddef.h>
#include <stdint.h>

struct dtb_header {
	uint32_t			magic;
	uint32_t			size;
	uint32_t			off_struct;
	uint32_t			off_strings;
	uint32_t			off_mem_res_map;
	uint32_t			version;
	uint32_t			last_comp_version;
	uint32_t			boot_cpuid;
	uint32_t			size_strings;
	uint32_t			size_struct;
};

struct dtb {
	const uint32_t			*buf;
	struct dtb_header		hdr;
	int				num_words;
};

int	dtb_init(struct dtb *dtb, const void *buf);
int	dtb_size(const struct dtb *dtb);
int	dtb_node(const struct dtb *dtb, const char *path);
int	dtbn_child(const struct dtb *dtb, int node, const char *name, int len);
int	dtbn_prop_read(const struct dtb *dtb, int node, const char *name,
		       void *buf, size_t size);
int	dtb_mem_res_read(const struct dtb *dtb, uint64_t *addr, uint64_t *size,
			 int len);
#endif
