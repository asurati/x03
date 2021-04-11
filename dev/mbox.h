// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef L_DEV_MBOX_H
#define L_DEV_MBOX_H

#include <dev/mbox.h>

#define MBOX_TAG_GET_FIRMWARE_REVISION	1u
#define MBOX_TAG_GET_BOARD_REVISION	0x10002u
#define MBOX_TAG_GET_VC_MEMORY		0x10006u
#define MBOX_TAG_GET_CLOCK_RATE		0x30002u
#define MBOX_TAG_ENABLE_QPU		0x30012u
#define MBOX_TAG_SET_DOMAIN_STATE	0x38030u

struct mbox_msg {
	unsigned int			size;
	unsigned int			code;
};

struct mbox_msg_tag {
	unsigned int			id;
	unsigned int			buf_size;
	unsigned int			data_size;
};

struct mbox_msg_get_clock_rate {
	struct mbox_msg			msg;
	struct mbox_msg_tag		msg_tag;
	struct {
		unsigned int		clock_id;
		unsigned int		clock_rate;
	} buf;
	unsigned int			end_tag;
};

struct mbox_msg_get_firmware_revision {
	struct mbox_msg			msg;
	struct mbox_msg_tag		msg_tag;
	struct {
		unsigned int		rev;
	} buf;
	unsigned int			end_tag;
};

struct mbox_msg_get_vc_memory {
	struct mbox_msg			msg;
	struct mbox_msg_tag		msg_tag;
	struct {
		unsigned int		base;
		unsigned int		size;
	} buf;
	unsigned int			end_tag;
};

struct mbox_msg_enable_qpu {
	struct mbox_msg			msg;
	struct mbox_msg_tag		msg_tag;
	struct {
		unsigned int		is_enable;
	} buf;
	unsigned int			end_tag;
};

struct mbox_msg_set_domain_state {
	struct mbox_msg			msg;
	struct mbox_msg_tag		msg_tag;
	struct {
		unsigned int		domain;
		unsigned int		on;
	} buf;
	unsigned int			end_tag;
};

int	mbox_init();
#endif
