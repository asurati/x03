// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef DEV_HDMI_H
#define DEV_HDMI_H

#include <stdint.h>

#include <sys/bits.h>

// HD register space.
#define HDMI_MAI_CTRL			(0x14 >> 2)
#define HDMI_VID_CTRL			(0x38 >> 2)
#define HDMI_CSC_CTRL			(0x40 >> 2)

// HDMI register space.
#define HDMI_FIFO_CTRL			(0x5c >> 2)
#define HDMI_RAM_PKT_CFG		(0xa0 >> 2)
#define HDMI_RAM_PKT_STS		(0xa4 >> 2)
#define HDMI_SCHED_CTRL			(0xc0 >> 2)
#define HDMI_HORZA			(0xc4 >> 2)
#define HDMI_HORZB			(0xc8 >> 2)
#define HDMI_VERTA0			(0xcc >> 2)
#define HDMI_VERTB0			(0xd0 >> 2)
#define HDMI_VERTA1			(0xd4 >> 2)
#define HDMI_VERTB1			(0xd8 >> 2)
#define HDMI_TX_PHY_RESET_CTRL		(0x2c0 >> 2)

#define HDMI_VID_CTRL_BLANK_POS		18
#define HDMI_VID_CTRL_CLR_SYNC_POS	24
#define HDMI_VID_CTRL_CLR_RGB_POS	23
#define HDMI_VID_CTRL_NHSYNC_POS	27
#define HDMI_VID_CTRL_NVSYNC_POS	28
#define HDMI_VID_CTRL_EN_POS		31
#define HDMI_VID_CTRL_BLANK_BITS	1
#define HDMI_VID_CTRL_CLR_SYNC_BITS	1
#define HDMI_VID_CTRL_CLR_RGB_BITS	1
#define HDMI_VID_CTRL_NHSYNC_BITS	1
#define HDMI_VID_CTRL_NVSYNC_BITS	1
#define HDMI_VID_CTRL_EN_BITS		1

#define HDMI_SCHED_CTRL_IGNORE_VP_POS	5
#define HDMI_SCHED_CTRL_MANUAL_FMT_POS	15
#define HDMI_SCHED_CTRL_IGNORE_VP_BITS	1
#define HDMI_SCHED_CTRL_MANUAL_FMT_BITS	1

struct hdmi_avi_infoframe {
	uint8_t				type;
	uint8_t				version;
	uint8_t				data_len;
	uint8_t				chksum;
	uint8_t				colour_scan;
	uint8_t				colour_aspect;
	uint8_t				ext_colour_quant;
	uint8_t				vic;
	uint8_t				repeat;
	uint16_t			top;
	uint16_t			btm;
	uint16_t			left;
	uint16_t			right;
} __attribute__((packed));
#endif
