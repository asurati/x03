// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef DEV_TXP_H
#define DEV_TXP_H

#include <sys/bits.h>

#define TXP_DST_POINTER			(0x0 >> 2)
#define TXP_DST_PITCH			(0x4 >> 2)
#define TXP_DIM				(0x8 >> 2)
#define TXP_DST_CTRL			(0xc >> 2)
#define TXP_DST_PROGRESS		(0x10 >> 2)

#define TXP_DIM_WIDTH_POS		0
#define TXP_DIM_HEIGHT_POS		16
#define TXP_DIM_WIDTH_BITS		16
#define TXP_DIM_HEIGHT_BITS		16

#define TXP_DST_CTRL_GO_POS		0
#define TXP_DST_CTRL_BUSY_POS		1
#define TXP_DST_CTRL_FMT_POS		8
#define TXP_DST_CTRL_BYTE_EN_POS	16
#define TXP_DST_CTRL_ALPHA_EN_POS	20
#define TXP_DST_CTRL_VERSION_POS	22
#define TXP_DST_CTRL_PILOT_POS		24
#define TXP_DST_CTRL_GO_BITS		1
#define TXP_DST_CTRL_BUSY_BITS		1
#define TXP_DST_CTRL_FMT_BITS		4
#define TXP_DST_CTRL_BYTE_EN_BITS	4
#define TXP_DST_CTRL_ALPHA_EN_BITS	1
#define TXP_DST_CTRL_VERSION_BITS	2
#define TXP_DST_CTRL_PILOT_BITS		8
#endif
