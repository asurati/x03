// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef L_DEV_DTB_H
#define L_DEV_DTB_H

#include <dev/dtb.h>

struct dtb_prop {
	uint32_t			len;
	uint32_t			off_name;
};

struct dtb_res_entry {
	uint64_t			addr;
	uint64_t			size;
};

#define DTB_MAGIC			0xd00dfeed
#define DTB_BEGIN_NODE			1
#define DTB_END_NODE			2
#define DTB_PROP			3
#define DTB_NOP				4
#define DTB_END				9
#endif
