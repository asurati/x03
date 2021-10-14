// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef DEV_DDC_H
#define DEV_DDC_H

#include <sys/bits.h>

#define DDC_CTRL			(0x0 >> 2)
#define DDC_STS				(0x4 >> 2)
#define DDC_DLEN			(0x8 >> 2)
#define DDC_ADDR			(0xc >> 2)
#define DDC_FIFO			(0x10 >> 2)

#define DDC_CTRL_READ_POS		0
#define DDC_CTRL_START_POS		7
#define DDC_CTRL_I2C_EN_POS		15
#define DDC_CTRL_READ_BITS		1
#define DDC_CTRL_START_BITS		1
#define DDC_CTRL_I2C_EN_BITS		1

#define DDC_STS_DONE_POS		1
#define DDC_STS_TXW_POS			2
#define DDC_STS_RXR_POS			3
#define DDC_STS_TXD_POS			4
#define DDC_STS_RXD_POS			5
#define DDC_STS_DONE_BITS		1
#define DDC_STS_TXW_BITS		1
#define DDC_STS_RXR_BITS		1
#define DDC_STS_TXD_BITS		1
#define DDC_STS_RXD_BITS		1
#endif
