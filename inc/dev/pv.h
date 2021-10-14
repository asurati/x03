// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#ifndef DEV_PV_H
#define DEV_PV_H

#include <sys/bits.h>

#define PV_CTRL				(0 >> 2)
#define PV_VID_CTRL			(0x4 >> 2)
#define PV_HORZA			(0xc >> 2)
#define PV_HORZB			(0x10 >> 2)
#define PV_VERTA			(0x14 >> 2)
#define PV_VERTB			(0x18 >> 2)
#define PV_INT_EN			(0x24 >> 2)
#define PV_INT_STAT			(0x28 >> 2)
#define PV_STAT				(0x2c >> 2)

#define PV_CTRL_EN_POS			0
#define PV_CTRL_FIFO_CLR_POS		1
#define PV_CTRL_WAIT_HSTART_POS		12
#define PV_CTRL_TRIG_UFLOW_POS		13
#define PV_CTRL_CLR_AT_START_POS	14
#define PV_CTRL_EN_BITS			1
#define PV_CTRL_FIFO_CLR_BITS		1
#define PV_CTRL_WAIT_HSTART_BITS	1
#define PV_CTRL_TRIG_UFLOW_BITS		1
#define PV_CTRL_CLR_AT_START_BITS	1

#define PV_VID_CTRL_EN_POS		0
#define PV_VID_CTRL_CONTINUOUS_POS	1
#define PV_VID_CTRL_EN_BITS		1
#define PV_VID_CTRL_CONTINUOUS_BITS	1
#endif
