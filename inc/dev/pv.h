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
#define PV_CTRL_CLK_SEL_POS		2
#define PV_CTRL_WAIT_HSTART_POS		12
#define PV_CTRL_TRIG_UFLOW_POS		13
#define PV_CTRL_CLR_AT_START_POS	14
#define PV_CTRL_FIFO_LEVEL_POS		15
#define PV_CTRL_EN_BITS			1
#define PV_CTRL_FIFO_CLR_BITS		1
#define PV_CTRL_CLK_SEL_BITS		2
#define PV_CTRL_WAIT_HSTART_BITS	1
#define PV_CTRL_TRIG_UFLOW_BITS		1
#define PV_CTRL_CLR_AT_START_BITS	1
#define PV_CTRL_FIFO_LEVEL_BITS		6

#define PV_VID_CTRL_EN_POS		0
#define PV_VID_CTRL_CONTINUOUS_POS	1
#define PV_VID_CTRL_EN_BITS		1
#define PV_VID_CTRL_CONTINUOUS_BITS	1

#define PV_HORZA_HSW_POS		0
#define PV_HORZA_HBP_POS		16
#define PV_HORZA_HSW_BITS		16
#define PV_HORZA_HBP_BITS		16

#define PV_HORZB_HAP_POS		0
#define PV_HORZB_HFP_POS		16
#define PV_HORZB_HAP_BITS		16
#define PV_HORZB_HFP_BITS		16

#define PV_VERTA_VSW_POS		0
#define PV_VERTA_VBP_POS		16
#define PV_VERTA_VSW_BITS		16
#define PV_VERTA_VBP_BITS		16

#define PV_VERTB_VAL_POS		0
#define PV_VERTB_VFP_POS		16
#define PV_VERTB_VAL_BITS		16
#define PV_VERTB_VFP_BITS		16

#endif
